// enve - 2D animations software
// Copyright (C) 2016-2020 Maurycy Liebner

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "property.h"
#include "Animators/complexanimator.h"
#include "undoredo.h"
#include "Animators/transformanimator.h"
#include "typemenu.h"
#include "Private/document.h"
#include "canvas.h"

Property::Property(const QString& name) :
    prp_mName(name) {
    connect(this, &Property::prp_ancestorChanged, this, [this]() {
        mParentScene = mParent_k ? mParent_k->mParentScene : nullptr;
        emit prp_pathChanged();
    });
}

void Property::prp_updateCanvasProps() {
    if(mParent_k) mParent_k->prp_updateCanvasProps();
}

void Property::prp_drawCanvasControls(
        SkCanvas * const canvas, const CanvasMode mode,
        const float invScale, const bool ctrlPressed) {
    if(mPointsHandler) {
        bool key;
        if(SWT_isAnimator()) {
            const auto asAnim = static_cast<Animator*>(this);
            key = asAnim->anim_getKeyOnCurrentFrame();
        } else key = false;
        mPointsHandler->drawPoints(canvas, invScale, key, mode, ctrlPressed);
    }
}

void Property::prp_setupTreeViewMenu(PropertyMenu * const menu) {
    const auto clipboard = Document::sInstance->getPropertyClipboard();
    const bool compat = clipboard && clipboard->compatibleTarget(this);
    menu->addPlainAction("Paste", [this, clipboard]() {
        clipboard->paste(this);
    })->setEnabled(compat);
    menu->addPlainAction("Copy", [this]() {
        const auto clipboard = enve::make_shared<PropertyClipboard>(this);
        Document::sInstance->replaceClipboard(clipboard);
    });
}

void Property::prp_afterChangedAbsRange(const FrameRange &range,
                                        const bool clip) {
    prp_afterChangedCurrent(UpdateReason::userChange);
    emit prp_absFrameRangeChanged(range, clip);
}

void Property::prp_afterWholeInfluenceRangeChanged() {
    prp_afterChangedAbsRange(prp_absInfluenceRange());
}

void Property::prp_afterChangedRelRange(const FrameRange &range, const bool clip) {
    const auto absRange = prp_relRangeToAbsRange(range);
    prp_afterChangedAbsRange(absRange, clip);
}

FrameRange Property::prp_getIdenticalAbsRange(const int relFrame) const {
    return prp_relRangeToAbsRange(prp_getIdenticalRelRange(relFrame));
}

const QString &Property::prp_getName() const {
    return prp_mName;
}

int Property::prp_getTotalFrameShift() const {
    return prp_getRelFrameShift() + prp_getInheritedFrameShift();
}

int Property::prp_getInheritedFrameShift() const {
    return prp_mInheritedFrameShift;
}

void Property::prp_setInheritedFrameShift(const int shift,
                                          ComplexAnimator *parentAnimator) {
    Q_UNUSED(parentAnimator)
    if(prp_mInheritedFrameShift == shift) return;
    const auto oldRange = prp_absInfluenceRange();
    prp_mInheritedFrameShift = shift;
    const auto newRange = prp_absInfluenceRange();
    prp_afterFrameShiftChanged(oldRange, newRange);
}

void Property::prp_afterFrameShiftChanged(const FrameRange &oldAbsRange,
                                          const FrameRange &newAbsRange) {
    prp_afterChangedAbsRange((newAbsRange + oldAbsRange).adjusted(-1, 1), false);
}

BasicTransformAnimator *Property::getTransformAnimator() const {
    if(mParent_k) return mParent_k->getTransformAnimator();
    return nullptr;
}

QMatrix Property::getTransform() const {
    const auto trans = getTransformAnimator();
    if(trans) return trans->getTotalTransform();
    return QMatrix();
}

void Property::prp_setSelected(const bool selected) {
    if(prp_mSelected == selected) return;
    prp_mSelected = selected;
    emit prp_selectionChanged(selected, QPrivateSignal());
}

FrameRange Property::prp_relRangeToAbsRange(const FrameRange& range) const {
    return {prp_relFrameToAbsFrame(range.fMin),
                prp_relFrameToAbsFrame(range.fMax)};
}

FrameRange Property::prp_absRangeToRelRange(const FrameRange& range) const {
    return {prp_absFrameToRelFrame(range.fMin),
            prp_absFrameToRelFrame(range.fMax)};
}

int Property::prp_absFrameToRelFrame(const int absFrame) const {
    if(absFrame == FrameRange::EMIN) return FrameRange::EMIN;
    if(absFrame == FrameRange::EMAX) return FrameRange::EMAX;
    return absFrame - prp_getTotalFrameShift();
}

qreal Property::prp_absFrameToRelFrameF(const qreal absFrame) const {
    if(qRound(absFrame) == FrameRange::EMIN) return FrameRange::EMIN;
    if(qRound(absFrame) == FrameRange::EMAX) return FrameRange::EMAX;
    return absFrame - prp_getTotalFrameShift();
}

int Property::prp_relFrameToAbsFrame(const int relFrame) const {
    if(relFrame == FrameRange::EMIN) return FrameRange::EMIN;
    if(relFrame == FrameRange::EMAX) return FrameRange::EMAX;
    return relFrame + prp_getTotalFrameShift();
}

qreal Property::prp_relFrameToAbsFrameF(const qreal relFrame) const {
    if(qRound(relFrame) == FrameRange::EMIN) return FrameRange::EMIN;
    if(qRound(relFrame) == FrameRange::EMAX) return FrameRange::EMAX;
    return relFrame + prp_getTotalFrameShift();
}

void Property::prp_setNameAction(const QString &newName) {
    if(newName == prp_mName) return;
    {
        prp_pushUndoRedoName("Rename");
        UndoRedo ur;
        const auto oldValue = prp_mName;
        const auto newValue = newName;
        ur.fUndo = [this, oldValue]() {
            prp_setName(oldValue);
        };
        ur.fRedo = [this, newValue]() {
            prp_setName(newValue);
        };
        prp_addUndoRedo(ur);
    }
    prp_setName(newName);
}

void Property::prp_setName(const QString &newName) {
    if(newName == prp_mName) return;
    prp_mName = newName;
    emit prp_nameChanged(newName, QPrivateSignal());
    emit prp_pathChanged();
}

bool Property::prp_differencesBetweenRelFrames(
        const int frame1, const int frame2) const {
    return !prp_getIdenticalRelRange(frame1).inRange(frame2);
}

FrameRange Property::prp_absInfluenceRange() const {
    return prp_relRangeToAbsRange(prp_relInfluenceRange());
}

void Property::prp_enabledDrawingOnCanvas() {
    prp_setDrawingOnCanvasEnabled(true);
}

void Property::prp_disableDrawingOnCanvas() {
    prp_setDrawingOnCanvasEnabled(false);
}

void Property::prp_setDrawingOnCanvasEnabled(const bool enabled) {
    mDrawOnCanvas = enabled;
    if(mParent_k) mParent_k->prp_updateCanvasProps();
}

void Property::prp_getFullPath(QStringList& names) const {
    if(mParent_k) mParent_k->prp_getFullPath(names);
    names.append(prp_getName());
}

QString Property::prp_sFixName(const QString &name) {
    QString result = name.trimmed();

    result.remove(QRegExp("[^A-Za-z0-9 _]"));
    while(!result.isEmpty() &&
          (result.front() == ' ' ||
           result.front().isDigit())) {
        result.remove(0, 1);
    }
    if(result.isEmpty()) return "Object";
    return result;
}

bool Property::prp_sValidateName(const QString &name,
                                 QString *error) {
    if(name.isEmpty()) {
        *error = "Name cannot be empty";
        return false;
    }
    if(!name.front().isLetter()) {
        *error = "Name has to start with a letter";
        return false;
    }
    if(name.back() == ' ') {
        *error = "Name cannot end with a space";
        return false;
    }
    const int nValid = name.count(QRegExp("[A-Za-z0-9_ ]"));
    if(nValid != name.count()) {
        *error = "Invalid characters used";
        return false;
    }
    return true;
}

void Property::setPointsHandler(const stdsptr<PointsHandler> &handler) {
    mPointsHandler = handler;
    if(mPointsHandler) {
        prp_enabledDrawingOnCanvas();
        mPointsHandler->setTransform(getTransformAnimator());
    }
}

void Property::prp_addUndoRedo(const UndoRedo& undoRedo) {
    const auto parentScene = getParentScene();
    if(!parentScene) return;
    qptr<Property> thisQPtr = this;
    auto undo = undoRedo.fUndo;
    auto redo = undoRedo.fRedo;
    undo = [thisQPtr, undo]() { if(thisQPtr) undo(); };
    redo = [thisQPtr, redo]() { if(thisQPtr) redo(); };
    parentScene->addUndoRedo(prp_getName() + " Change", undo, redo);
}

void Property::prp_pushUndoRedoName(const QString& name) {
    const auto parentScene = getParentScene();
    if(!parentScene) return;
    parentScene->pushUndoRedoName(name);
}

void Property::setParent(ComplexAnimator * const parent) {
    if(mParent_k == parent) return;
    auto& conn = mParent_k.assign(parent);
    if(parent) {
        conn << connect(mParent_k, &Property::prp_ancestorChanged,
                        this, &Property::prp_ancestorChanged);
        conn << connect(mParent_k, &Property::prp_pathChanged,
                        this, &Property::prp_pathChanged);
    }
    if(mPointsHandler) mPointsHandler->setTransform(getTransformAnimator());
    emit prp_parentChanged(parent, QPrivateSignal());
    emit prp_ancestorChanged(QPrivateSignal());
}

bool Property::prp_isParentBoxSelected() const {
    const auto pBox = getFirstAncestor<eBoxOrSound>();
    if(pBox) return pBox->isSelected();
    return false;
}

#include "canvas.h"
void Property::prp_selectionChangeTriggered(const bool shiftPressed) {
    if(!mParentScene) return;
    if(shiftPressed) {
        if(prp_mSelected) {
            mParentScene->removeFromSelectedProps(this);
        } else {
            mParentScene->addToSelectedProps(this);
        }
    } else {
        mParentScene->clearSelectedProps();
        mParentScene->addToSelectedProps(this);
    }
}
