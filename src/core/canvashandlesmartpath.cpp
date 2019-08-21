#include "canvas.h"
#include "MovablePoints/smartnodepoint.h"
#include "Boxes/smartvectorpath.h"

void Canvas::clearCurrentSmartEndPoint() {
    setCurrentSmartEndPoint(nullptr);
}

void Canvas::setCurrentSmartEndPoint(SmartNodePoint * const point) {
    if(mLastEndPoint) mLastEndPoint->deselect();
    if(point) point->select();
    mLastEndPoint = point;
}
#include "MovablePoints/pathpointshandler.h"
#include "Animators/SmartPath/smartpathcollection.h"
#include "Animators/transformanimator.h"

void Canvas::handleAddSmartPointMousePress(const MouseEvent &e) {
    if(mLastEndPoint ? mLastEndPoint->isHidden(mCurrentMode) : false) {
        clearCurrentSmartEndPoint();
    }
    qptr<BoundingBox> test;

    auto nodePointUnderMouse = static_cast<SmartNodePoint*>(mPressedPoint.data());
    if(nodePointUnderMouse ? !nodePointUnderMouse->isEndPoint() : false) {
        nodePointUnderMouse = nullptr;
    }
    if(nodePointUnderMouse == mLastEndPoint &&
            nodePointUnderMouse) return;
    if(!mLastEndPoint && !nodePointUnderMouse) {
        const auto newPath = enve::make_shared<SmartVectorPath>();
        newPath->planCenterPivotPosition();
        mCurrentContainer->addContained(newPath);
        clearBoxesSelection();
        addBoxToSelection(newPath.get());
        const auto relPos = newPath->mapAbsPosToRel(e.fPos);
        newPath->getBoxTransformAnimator()->setPosition(relPos.x(), relPos.y());
        const auto newHandler = newPath->getPathAnimator();
        const auto node = newHandler->createNewSubPathAtRelPos({0, 0});
        setCurrentSmartEndPoint(node);
    } else {
        if(!nodePointUnderMouse) {
            const auto newPoint = mLastEndPoint->actionAddPointAbsPos(e.fPos);
            //newPoint->startTransform();
            setCurrentSmartEndPoint(newPoint);
        } else if(!mLastEndPoint) {
            setCurrentSmartEndPoint(nodePointUnderMouse);
        } else { // mCurrentSmartEndPoint
            const auto targetNode = nodePointUnderMouse->getTargetNode();
            const auto handler = nodePointUnderMouse->getHandler();
            const bool success = nodePointUnderMouse->isEndPoint() &&
                    mLastEndPoint->actionConnectToNormalPoint(
                        nodePointUnderMouse);
            if(success) {
                const int newTargetId = targetNode->getNodeId();
                const auto sel = handler->getPointWithId<SmartNodePoint>(newTargetId);
                setCurrentSmartEndPoint(sel);
            }
        }
    } // pats is not null
}


void Canvas::handleAddSmartPointMouseMove(const MouseEvent &e) {
    if(!mLastEndPoint) return;
    if(mFirstMouseMove) mLastEndPoint->startTransform();
    if(mLastEndPoint->hasNextNormalPoint() &&
       mLastEndPoint->hasPrevNormalPoint()) {
        mLastEndPoint->setCtrlsMode(CtrlsMode::CTRLS_CORNER);
        mLastEndPoint->setC0Enabled(true);
        mLastEndPoint->moveC0ToAbsPos(e.fPos);
    } else {
        if(!mLastEndPoint->hasNextNormalPoint() &&
           !mLastEndPoint->hasPrevNormalPoint()) {            
            mLastEndPoint->setCtrlsMode(CtrlsMode::CTRLS_CORNER);
            mLastEndPoint->setC2Enabled(true);
        } else {
            mLastEndPoint->setCtrlsMode(CtrlsMode::CTRLS_SYMMETRIC);
        }
        if(mLastEndPoint->hasNextNormalPoint()) {
            mLastEndPoint->moveC0ToAbsPos(e.fPos);
        } else {
            mLastEndPoint->moveC2ToAbsPos(e.fPos);
        }
    }
}

void Canvas::handleAddSmartPointMouseRelease(const MouseEvent &e) {
    Q_UNUSED(e);
    if(mLastEndPoint) {
        if(!mFirstMouseMove) mLastEndPoint->finishTransform();
        //mCurrentSmartEndPoint->prp_afterWholeInfluenceRangeChanged();
        if(!mLastEndPoint->isEndPoint())
            clearCurrentSmartEndPoint();
    }
}