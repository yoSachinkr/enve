#include "patheffectanimators.h"
#include "patheffect.h"
#include "Boxes/pathbox.h"
#include "Boxes/boxesgroup.h"
#include <QDebug>

PathEffectAnimators::PathEffectAnimators(const bool &isOutline,
                                         const bool &isFill,
                                         BoundingBox *parentPath) :
    ComplexAnimator() {
    mIsOutline = isOutline;
    mIsFill = isFill;
    mParentPath = parentPath;
}

void PathEffectAnimators::addEffect(PathEffect *effect) {
    if(mParentPath->SWT_isPathBox()) {
        PathBox *pathBox = (PathBox*)mParentPath;
        if(mIsOutline) {
            pathBox->addOutlinePathEffect(effect);
        } else if(mIsFill) {
            pathBox->addFillPathEffect(effect);
        } else {
            pathBox->addPathEffect(effect);
        }
    } else if(mParentPath->SWT_isPathBox()) {
        BoxesGroup *groupBox = (BoxesGroup*)mParentPath;
        if(mIsOutline) {
            groupBox->addOutlinePathEffect(effect);
        } else if(mIsFill) {
            groupBox->addFillPathEffect(effect);
        } else {
            groupBox->addPathEffect(effect);
        }
    }
}

void PathEffectAnimators::removeEffect(PathEffect *effect) {
    if(mParentPath->SWT_isPathBox()) {
        PathBox *pathBox = (PathBox*)mParentPath;
        if(mIsOutline) {
            pathBox->removeOutlinePathEffect(effect);
        } else if(mIsFill) {
            pathBox->removeFillPathEffect(effect);
        } else {
            pathBox->removePathEffect(effect);
        }
    } else if(mParentPath->SWT_isPathBox()) {
        BoxesGroup *groupBox = (BoxesGroup*)mParentPath;
        if(mIsOutline) {
            groupBox->removeOutlinePathEffect(effect);
        } else if(mIsFill) {
            groupBox->removeFillPathEffect(effect);
        } else {
            groupBox->removePathEffect(effect);
        }
    }
}

bool PathEffectAnimators::hasEffects() {
    return !ca_mChildAnimators.isEmpty();
}

void PathEffectAnimators::filterPathForRelFrameBeforeThickness(
                                                const int &relFrame,
                                                SkPath *srcDstPath) {
    SkPath dstPath = *srcDstPath;
    Q_FOREACH(const QSharedPointer<Property> &effect, ca_mChildAnimators) {
        PathEffect *effectT = (PathEffect*)effect.data();
        if(effectT->applyBeforeThickness() && effectT->isVisible()) {
            SkPath srcPath = dstPath;
            effectT->filterPathForRelFrame(relFrame,
                                           srcPath,
                                           &dstPath);
        }
    }
    *srcDstPath = dstPath;
}

void PathEffectAnimators::filterPathForRelFrame(const int &relFrame,
                                                SkPath *srcDstPath) {
    SkPath dstPath = *srcDstPath;
    Q_FOREACH(const QSharedPointer<Property> &effect, ca_mChildAnimators) {
        PathEffect *effectT = (PathEffect*)effect.data();
        if(effectT->applyBeforeThickness() || !effectT->isVisible()) {
            continue;
        }
        SkPath srcPath = dstPath;
        effectT->filterPathForRelFrame(relFrame,
                                       srcPath,
                                       &dstPath);
    }
    *srcDstPath = dstPath;
}
