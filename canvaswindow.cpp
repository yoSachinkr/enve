#include "canvaswindow.h"
#include "canvas.h"
#include <QComboBox>
#include "mainwindow.h"
#include "BoxesList/boxscrollwidgetvisiblepart.h"
#include "BoxesList/OptimalScrollArea/singlewidgetabstraction.h"
#include "paintcontroler.h"
#include "renderoutputwidget.h"
#include "Sound/soundcomposition.h"
#include "global.h"
#include "canvaswidget.h"
#include "RenderWidget/renderinstancesettings.h"
#include "newcanvasdialog.h"
#include "Boxes/videobox.h"
#include "Boxes/imagebox.h"
#include "Sound/singlesound.h"

CanvasWindow::CanvasWindow(QWidget *parent) {
    //setAttribute(Qt::WA_OpaquePaintEvent, true);
    int numberThreads = 1;//qMax(1, QThread::idealThreadCount() - 1);
    for(int i = 0; i < numberThreads; i++) {
        QThread *paintControlerThread = new QThread(this);
        PaintControler *paintControler = new PaintControler(i);
        paintControler->moveToThread(paintControlerThread);
        connect(paintControler, SIGNAL(finishedUpdating(int, Updatable*)),
                this, SLOT(sendNextUpdatableForUpdate(int, Updatable*)) );
        connect(this, SIGNAL(updateUpdatable(Updatable*, int)),
                paintControler, SLOT(updateUpdatable(Updatable*, int)) );

        paintControlerThread->start();

        mPaintControlers << paintControler;
        mPaintControlerThreads << paintControlerThread;

        mFreeThreads << i;
    }

    mPreviewFPSTimer = new QTimer(this);

    initializeAudio();

    mCanvasWidget = QWidget::createWindowContainer(this, parent);
    //mCanvasWidget = new CanvasWidget(this, parent);
    mCanvasWidget->setAcceptDrops(true);
    mCanvasWidget->setMinimumSize(MIN_WIDGET_HEIGHT*10,
                                  MIN_WIDGET_HEIGHT*10);
    mCanvasWidget->setSizePolicy(QSizePolicy::Minimum,
                                 QSizePolicy::Minimum);
    mCanvasWidget->setMouseTracking(true);
}

CanvasWindow::~CanvasWindow() {
    foreach(QThread *thread, mPaintControlerThreads) {
        thread->quit();
        thread->wait();
        delete thread;
    }
    foreach(PaintControler *paintControler, mPaintControlers) {
        delete paintControler;
    }
}

Canvas *CanvasWindow::getCurrentCanvas() {
    return mCurrentCanvas;
}

SingleWidgetAbstraction* CanvasWindow::SWT_getAbstractionForWidget(
            ScrollWidgetVisiblePart *visiblePartWidget) {
    Q_FOREACH(SingleWidgetAbstraction *abs, mSWT_allAbstractions) {
        if(abs->getParentVisiblePartWidget() == visiblePartWidget) {
            return abs;
        }
    }
    SingleWidgetAbstraction *abs = SWT_createAbstraction(visiblePartWidget);
    return abs;
}

void CanvasWindow::SWT_addChildrenAbstractions(
        SingleWidgetAbstraction *abstraction,
        ScrollWidgetVisiblePart *visiblePartWidget) {
    Q_FOREACH(Canvas *child, mCanvasList) {
        abstraction->addChildAbstraction(
                    child->SWT_getAbstractionForWidget(visiblePartWidget));
    }
}

void CanvasWindow::setCurrentCanvas(const int &id) {
    if(id < 0 || id >= mCanvasList.count()) {
        setCurrentCanvas((Canvas*)NULL);
    } else {
        setCurrentCanvas(mCanvasList.at(id));
    }
}

void CanvasWindow::setCurrentCanvas(Canvas *canvas) {
    if(mCurrentCanvas != NULL) {
        mCurrentCanvas->setIsCurrentCanvas(false);
        disconnect(mPreviewFPSTimer, SIGNAL(timeout()),
                   mCurrentCanvas, SLOT(nextPreviewFrame()) );
    }
    mCurrentCanvas = canvas;
    if(mCurrentCanvas == NULL) {
        mCurrentSoundComposition = NULL;
    } else {
        mCurrentSoundComposition = mCurrentCanvas->getSoundComposition();
        connect(mPreviewFPSTimer, SIGNAL(timeout()),
                mCurrentCanvas, SLOT(nextPreviewFrame()) );

        mCurrentCanvas->setIsCurrentCanvas(true);

        setCanvasMode(mCurrentCanvas->getCurrentCanvasMode());

        emit changeFrameRange(0, getMaxFrame());
        emit changeCurrentFrame(getCurrentFrame());
    }
    SWT_scheduleWidgetsContentUpdateWithTarget(
                mCurrentCanvas,
                SWT_CurrentCanvas);
    updateDisplayedFillStrokeSettings();
    MainWindow::getInstance()->updateSettingsForCurrentCanvas();
    callUpdateSchedulers();
}

void CanvasWindow::addCanvasToList(Canvas *canvas) {
    mCanvasList << canvas;
    SWT_addChildAbstractionForTargetToAll(canvas);
}

void CanvasWindow::removeCanvas(const int &id) {
    Canvas *canvas = mCanvasList.takeAt(id);
    SWT_removeChildAbstractionForTargetFromAll(canvas);
    if(mCanvasList.isEmpty()) {
        setCurrentCanvas((Canvas*)NULL);
    } else if(id < mCanvasList.count()) {
        setCurrentCanvas(id);
    } else {
        setCurrentCanvas(id - 1);
    }
}

void CanvasWindow::setCanvasMode(const CanvasMode &mode) {
    if(hasNoCanvas()) {
        setCursor(QCursor(Qt::ArrowCursor) );
        return;
    }

    if(mode == MOVE_PATH) {
        setCursor(QCursor(Qt::ArrowCursor) );
    } else if(mode == MOVE_POINT) {
        setCursor(QCursor(QPixmap(":/cursors/cursor-node.xpm"), 0, 0) );
    } else if(mode == PICK_PATH_SETTINGS) {
        setCursor(QCursor(QPixmap(":/cursors/cursor_color_picker.png"), 2, 20) );
    } else if(mode == ADD_CIRCLE) {
        setCursor(QCursor(QPixmap(":/cursors/cursor-ellipse.xpm"), 4, 4) );
    } else if(mode == ADD_RECTANGLE || mode == ADD_PARTICLE_BOX) {
        setCursor(QCursor(QPixmap(":/cursors/cursor-rect.xpm"), 4, 4) );
    } else if(mode == ADD_TEXT) {
        setCursor(QCursor(QPixmap(":/cursors/cursor-text.xpm"), 4, 4) );
    } else {
        setCursor(QCursor(QPixmap(":/cursors/cursor-pen.xpm"), 4, 4) );
    }

    mCurrentCanvas->setCanvasMode(mode);
    MainWindow::getInstance()->updateCanvasModeButtonsChecked();
    callUpdateSchedulers();
}

void CanvasWindow::callUpdateSchedulers() {
    MainWindow::getInstance()->callUpdateSchedulers();
}

void CanvasWindow::setMovePathMode() {
    setCanvasMode(MOVE_PATH);
}

void CanvasWindow::setMovePointMode() {
    setCanvasMode(MOVE_POINT);
}

void CanvasWindow::setAddPointMode() {
    setCanvasMode(ADD_POINT);
}

void CanvasWindow::setRectangleMode() {
    setCanvasMode(ADD_RECTANGLE);
}

void CanvasWindow::setCircleMode() {
    setCanvasMode(ADD_CIRCLE);
}

void CanvasWindow::setTextMode() {
    setCanvasMode(ADD_TEXT);
}

void CanvasWindow::setParticleBoxMode() {
    setCanvasMode(ADD_PARTICLE_BOX);
}

void CanvasWindow::setParticleEmitterMode() {
    setCanvasMode(ADD_PARTICLE_EMITTER);
}

void CanvasWindow::addCanvasToListAndSetAsCurrent(Canvas *canvas) {
    addCanvasToList(canvas);
    setCurrentCanvas(canvas);
}

void CanvasWindow::renameCanvas(Canvas *canvas, const QString &newName) {
    canvas->setName(newName);
}

void CanvasWindow::renameCanvas(const int &id, const QString &newName) {
    renameCanvas(mCanvasList.at(id), newName);
}

bool CanvasWindow::hasNoCanvas() {
    return mCurrentCanvas == NULL;
}

void CanvasWindow::renameCurrentCanvas(const QString &newName) {
    if(mCurrentCanvas == NULL) return;
    renameCanvas(mCurrentCanvas, newName);
}

void CanvasWindow::qRender(QPainter *p) {
    if(mCurrentCanvas == NULL) return;
    mCurrentCanvas->drawInputText(p);
}

void CanvasWindow::renderSk(SkCanvas *canvas) {
    if(mCurrentCanvas == NULL) return;
    mCurrentCanvas->renderSk(canvas);
}

void CanvasWindow::mousePressEvent(QMouseEvent *event) {
    KFT_setFocus();
    if(mCurrentCanvas == NULL) return;
    mCurrentCanvas->mousePressEvent(event);
}

void CanvasWindow::mouseReleaseEvent(QMouseEvent *event) {
    if(mCurrentCanvas == NULL) return;
    mCurrentCanvas->mouseReleaseEvent(event);
}

void CanvasWindow::mouseMoveEvent(QMouseEvent *event) {
    if(mCurrentCanvas == NULL) return;
    mCurrentCanvas->mouseMoveEvent(event);
}

void CanvasWindow::wheelEvent(QWheelEvent *event) {
    if(mCurrentCanvas == NULL) return;
    mCurrentCanvas->wheelEvent(event);
}

void CanvasWindow::mouseDoubleClickEvent(QMouseEvent *event) {
    if(mCurrentCanvas == NULL) return;
    mCurrentCanvas->mouseDoubleClickEvent(event);
}

void CanvasWindow::keyPressEvent(QKeyEvent *event) {
    if(mCurrentCanvas == NULL) return;
    mCurrentCanvas->keyPressEvent(event);
}

void CanvasWindow::openSettingsWindowForCurrentCanvas() {
    if(hasNoCanvas()) return;
    CanvasSettingsDialog dialog(mCurrentCanvas, mCanvasWidget);

    if(dialog.exec() == QDialog::Accepted) {
        dialog.applySettingsToCanvas(mCurrentCanvas);
        setCurrentCanvas(mCurrentCanvas);
    }
}

void CanvasWindow::rotate90CCW() {
    if(hasNoCanvas()) return;
    //mCurrentCanvas->rotate90CCW();
}

void CanvasWindow::rotate90CW() {
    if(hasNoCanvas()) return;
    //mCurrentCanvas->rotate90CW();
}

bool CanvasWindow::KFT_handleKeyEventForTarget(QKeyEvent *event) {
    if(hasNoCanvas()) return false;
    if(event->key() == Qt::Key_F1) {
        setCanvasMode(CanvasMode::MOVE_PATH);
    } else if(event->key() == Qt::Key_F2) {
        setCanvasMode(CanvasMode::MOVE_POINT);
    } else if(event->key() == Qt::Key_F3) {
        setCanvasMode(CanvasMode::ADD_POINT);
    } else if(event->key() == Qt::Key_F4) {
        setCanvasMode(CanvasMode::ADD_CIRCLE);
    } else if(event->key() == Qt::Key_F5) {
        setCanvasMode(CanvasMode::ADD_RECTANGLE);
    } else if(event->key() == Qt::Key_F6) {
        setCanvasMode(CanvasMode::ADD_TEXT);
    } else if(event->key() == Qt::Key_F7) {
        setCanvasMode(CanvasMode::ADD_PARTICLE_BOX);
    } else if(mCurrentCanvas->keyPressEvent(event)) {
    } else {
        return false;
    }
    return true;
}

void CanvasWindow::raiseAction() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->raiseSelectedBoxes();
    callUpdateSchedulers();
}

void CanvasWindow::lowerAction() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->lowerSelectedBoxes();
    callUpdateSchedulers();
}

void CanvasWindow::raiseToTopAction() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->raiseSelectedBoxesToTop();
    callUpdateSchedulers();
}

void CanvasWindow::lowerToBottomAction() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->lowerSelectedBoxesToBottom();
    callUpdateSchedulers();
}

void CanvasWindow::objectsToPathAction() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->convertSelectedBoxesToPath();
    callUpdateSchedulers();
}

void CanvasWindow::strokeToPathAction() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->convertSelectedPathStrokesToPath();
    callUpdateSchedulers();
}

void CanvasWindow::rotate90CWAction() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->rotateSelectedBoxesStartAndFinish(90.);
    callUpdateSchedulers();
}

void CanvasWindow::rotate90CCWAction() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->rotateSelectedBoxesStartAndFinish(-90.);
    callUpdateSchedulers();
}

void CanvasWindow::pathsUnionAction() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->selectedPathsUnion();
    callUpdateSchedulers();
}

void CanvasWindow::pathsDifferenceAction() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->selectedPathsDifference();
    callUpdateSchedulers();
}

void CanvasWindow::pathsIntersectionAction() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->selectedPathsIntersection();
    callUpdateSchedulers();
}

void CanvasWindow::pathsDivisionAction() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->selectedPathsDivision();
    callUpdateSchedulers();
}

void CanvasWindow::pathsExclusionAction() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->selectedPathsExclusion();
    callUpdateSchedulers();
}

void CanvasWindow::setFontFamilyAndStyle(QString family, QString style) {
    if(hasNoCanvas()) return;
    mCurrentCanvas->setSelectedFontFamilyAndStyle(family, style);
    callUpdateSchedulers();
}

void CanvasWindow::setFontSize(qreal size) {
    if(hasNoCanvas()) return;
    mCurrentCanvas->setSelectedFontSize(size);
    callUpdateSchedulers();
}

void CanvasWindow::connectPointsSlot() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->connectPoints();
    callUpdateSchedulers();
}

void CanvasWindow::disconnectPointsSlot() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->disconnectPoints();
    callUpdateSchedulers();
}

void CanvasWindow::mergePointsSlot() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->mergePoints();
    callUpdateSchedulers();
}

void CanvasWindow::makePointCtrlsSymmetric() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->makePointCtrlsSymmetric();
    callUpdateSchedulers();
}

void CanvasWindow::makePointCtrlsSmooth() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->makePointCtrlsSmooth();
    callUpdateSchedulers();
}

void CanvasWindow::makePointCtrlsCorner() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->makePointCtrlsCorner();
    callUpdateSchedulers();
}

void CanvasWindow::makeSegmentLine() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->makeSegmentLine();
    callUpdateSchedulers();
}

void CanvasWindow::makeSegmentCurve() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->makeSegmentCurve();
    callUpdateSchedulers();
}

void CanvasWindow::startSelectedStrokeWidthTransform() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->startSelectedStrokeWidthTransform();
    callUpdateSchedulers();
}

void CanvasWindow::deleteAction() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->deleteAction();
}

void CanvasWindow::copyAction() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->copyAction();
}

void CanvasWindow::pasteAction() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->pasteAction();
}

void CanvasWindow::cutAction() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->cutAction();
}

void CanvasWindow::duplicateAction() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->duplicateAction();
}

void CanvasWindow::selectAllAction() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->selectAllAction();
}

void CanvasWindow::clearSelectionAction() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->clearSelectionAction();
}

void CanvasWindow::groupSelectedBoxes() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->groupSelectedBoxes();
}

void CanvasWindow::ungroupSelectedBoxes() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->ungroupSelectedBoxes();
}

void CanvasWindow::startSelectedStrokeColorTransform() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->startSelectedStrokeColorTransform();
    callUpdateSchedulers();
}

void CanvasWindow::startSelectedFillColorTransform() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->startSelectedFillColorTransform();
    callUpdateSchedulers();
}

void CanvasWindow::strokeCapStyleChanged(const Qt::PenCapStyle &capStyle) {
    if(hasNoCanvas()) return;
    mCurrentCanvas->setSelectedCapStyle(capStyle);
    callUpdateSchedulers();
}

void CanvasWindow::strokeJoinStyleChanged(const Qt::PenJoinStyle &joinStyle) {
    if(hasNoCanvas()) return;
    mCurrentCanvas->setSelectedJoinStyle(joinStyle);
    callUpdateSchedulers();
}

void CanvasWindow::strokeWidthChanged(const qreal &strokeWidth,
                                      const bool &finish) {
    if(hasNoCanvas()) return;
    mCurrentCanvas->setSelectedStrokeWidth(strokeWidth, finish);
    callUpdateSchedulers();
}

void CanvasWindow::applyPaintSettingToSelected(
        const PaintSetting &setting) {
    if(hasNoCanvas()) return;
    mCurrentCanvas->applyPaintSettingToSelected(setting);
}

void CanvasWindow::setSelectedFillColorMode(const ColorMode &mode) {
    if(hasNoCanvas()) return;
    mCurrentCanvas->setSelectedFillColorMode(mode);
}

void CanvasWindow::setSelectedStrokeColorMode(const ColorMode &mode) {
    if(hasNoCanvas()) return;
    mCurrentCanvas->setSelectedStrokeColorMode(mode);
}

void CanvasWindow::updateAfterFrameChanged(const int &currentFrame) {
    if(hasNoCanvas()) return;
    mCurrentCanvas->prp_setAbsFrame(currentFrame);
}

void CanvasWindow::pickPathForSettings() {
    if(hasNoCanvas()) return;
    setCanvasMode(PICK_PATH_SETTINGS);
}

void CanvasWindow::updateDisplayedFillStrokeSettings() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->setDisplayedFillStrokeSettingsFromLastSelected();
}

void CanvasWindow::setEffectsPaintEnabled(const bool &bT) {
    if(hasNoCanvas()) return;
    mCurrentCanvas->setEffectsPaintEnabled(bT);
    mCurrentCanvas->updateAllBoxes();
    callUpdateSchedulers();
}

void CanvasWindow::setResolutionFraction(const qreal &percent) {
    if(hasNoCanvas()) return;
    mCurrentCanvas->setResolutionFraction(percent);
    mCurrentCanvas->clearAllCache();
    mCurrentCanvas->updateAllBoxes();
    callUpdateSchedulers();
}

void CanvasWindow::updatePivotIfNeeded() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->updatePivotIfNeeded();
}

void CanvasWindow::schedulePivotUpdate() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->schedulePivotUpdate();
}

BoxesGroup *CanvasWindow::getCurrentGroup() {
    if(hasNoCanvas()) return NULL;
    return mCurrentCanvas->getCurrentBoxesGroup();
}

void CanvasWindow::renderOutput() {
    QSize size = mCurrentCanvas->getCanvasSize();
    RenderOutputWidget *dialog = new RenderOutputWidget(
                                        size.width(),
                                        size.height(),
                                        MainWindow::getInstance());
    connect(dialog, SIGNAL(renderOutput(QString, qreal)),
            this, SLOT(saveOutput(QString, qreal)));
    dialog->exec();
}

void CanvasWindow::renderFromSettings(RenderInstanceSettings *settings) {
    Canvas *canvas = settings->getTargetCanvas();
    const QString &destination = settings->getOutputDestination();
    setCurrentCanvas(canvas);
    emit changeCurrentFrame(0);
    saveOutput(destination, 1.);
}

void CanvasWindow::nextCurrentRenderFrame() {
    int newCurrentRenderFrame = mCurrentCanvas->getCacheHandler()->
            getFirstEmptyFrameAfterFrame(mCurrentRenderFrame);
    if(newCurrentRenderFrame - mCurrentRenderFrame > 1) {
        mCurrentCanvas->getCacheHandler()->
            setContainersInFrameRangeBlocked(mCurrentRenderFrame + 1,
                                             newCurrentRenderFrame - 1,
                                             true);
    }

    mCurrentRenderFrame = newCurrentRenderFrame;
    emit changeCurrentFrame(mCurrentRenderFrame);
}

void CanvasWindow::renderPreview() {
    if(hasNoCanvas()) return;
    mBoxesUpdateFinishedFunction = &CanvasWindow::nextPreviewRenderFrame;
    mSavedCurrentFrame = getCurrentFrame();

    mCurrentRenderFrame = mSavedCurrentFrame;
    setRendering(true);

    mCurrentCanvas->prp_setAbsFrame(mSavedCurrentFrame);
    mCurrentCanvas->updateAllBoxes();
    callUpdateSchedulers();
    if(mNoBoxesAwaitUpdate) {
        nextPreviewRenderFrame();
    }
    MainWindow::getInstance()->previewBeingRendered();
}

void CanvasWindow::addUpdatableAwaitingUpdate(Updatable *updatable) {
    if(mNoBoxesAwaitUpdate) {
        mNoBoxesAwaitUpdate = false;
    }
    if(!mFreeThreads.isEmpty()) {
        updatable->beforeUpdate();
        emit updateUpdatable(updatable, mFreeThreads.takeFirst());
    } else {
        mUpdatablesAwaitingUpdate << updatable;
    }
}

void CanvasWindow::sendNextUpdatableForUpdate(const int &threadId,
                                              Updatable *lastUpdatable) {
    if(lastUpdatable != NULL) {
        lastUpdatable->afterUpdate();
//        mLastUpdatedBox->setAwaitingUpdate(false);
//        if(mLastUpdatedBox->shouldRedoUpdate()) {
//            mLastUpdatedBox->setRedoUpdateToFalse();
//            mLastUpdatedBox->awaitUpdate();
//        }
    }
    if(mUpdatablesAwaitingUpdate.isEmpty()) {
        mNoBoxesAwaitUpdate = true;
        callUpdateSchedulers();
        if(mBoxesUpdateFinishedFunction != NULL) {
            (*this.*mBoxesUpdateFinishedFunction)();
        }
        mFreeThreads << threadId;
        //callUpdateSchedulers();
    } else {
        foreach(Updatable *updatablaT, mUpdatablesAwaitingUpdate) {
            if(updatablaT->readyToBeProcessed()) {
                mUpdatablesAwaitingUpdate.removeOne(updatablaT);
                updatablaT->beforeUpdate();
                emit updateUpdatable(updatablaT, threadId);
                return;
            }
        }
        mFreeThreads << threadId;
    }
}

void CanvasWindow::interruptPreview() {
    if(mRendering) {
        interruptRendering();
    } else if(mPreviewing) {
        stopPreview();
    }
}

void CanvasWindow::outOfMemory() {
    if(mRendering) {
        playPreview();
    }
}

void CanvasWindow::setRendering(const bool &bT) {
    mRendering = bT;
    mCurrentCanvas->setRendering(bT);
}

void CanvasWindow::setPreviewing(const bool &bT) {
    mPreviewing = bT;
    mCurrentCanvas->setPreviewing(bT);
}

void CanvasWindow::interruptRendering() {
    setRendering(false);
    mBoxesUpdateFinishedFunction = NULL;
    mCurrentCanvas->clearPreview();
    mCurrentCanvas->getCacheHandler()->
        setContainersInFrameRangeBlocked(mSavedCurrentFrame + 1,
                                         mCurrentRenderFrame,
                                         false);
    emit changeCurrentFrame(mSavedCurrentFrame);
    MainWindow::getInstance()->previewFinished();
}

void CanvasWindow::stopPreview() {
    setPreviewing(false);
    mCurrentCanvas->getCacheHandler()->
        setContainersInFrameRangeBlocked(mSavedCurrentFrame + 1,
                                         mCurrentRenderFrame,
                                         false);
    emit changeCurrentFrame(mSavedCurrentFrame);
    mPreviewFPSTimer->stop();
    stopAudio();
    repaint();
    MainWindow::getInstance()->previewFinished();
}

void CanvasWindow::pausePreview() {
    if(mPreviewing) {
        mPreviewFPSTimer->stop();
        MainWindow::getInstance()->previewPaused();
    }
}

void CanvasWindow::resumePreview() {
    if(mPreviewing) {
        mPreviewFPSTimer->start();
        MainWindow::getInstance()->previewBeingPlayed();
    }
}

void CanvasWindow::playPreview() {
    setRendering(false);
    //setPreviewing(true);
    //emit changeCurrentFrame(mSavedCurrentFrame);
    mBoxesUpdateFinishedFunction = NULL;
    mCurrentCanvas->playPreview(mSavedCurrentFrame,
                                mCurrentRenderFrame);
    mCurrentSoundComposition->generateData(mSavedCurrentFrame,
                                           mCurrentRenderFrame,
                                           mCurrentCanvas->getFps());
    startAudio();
    mPreviewFPSTimer->setInterval(1000/mCurrentCanvas->getFps());
    mPreviewFPSTimer->start();
    MainWindow::getInstance()->previewBeingPlayed();
}

void CanvasWindow::nextPreviewRenderFrame() {
    //mCurrentCanvas->renderCurrentFrameToPreview();
    if(!mRendering) return;
    if(mCurrentRenderFrame > getMaxFrame()) {
        playPreview();
    } else {
        nextCurrentRenderFrame();
        if(mNoBoxesAwaitUpdate) {
            nextPreviewRenderFrame();
        }
    }
}

void CanvasWindow::nextSaveOutputFrame() {
    mCurrentCanvas->renderCurrentFrameToOutput(mOutputString);
    if(mCurrentRenderFrame > getMaxFrame()) {
        emit changeCurrentFrame(mSavedCurrentFrame);
        mBoxesUpdateFinishedFunction = NULL;
        mCurrentCanvas->setOutputRendering(false);
        mCurrentCanvas->clearCurrentPreviewImage();
        if(qAbs(mSavedResolutionFraction -
                mCurrentCanvas->getResolutionFraction()) > 0.1) {
            mCurrentCanvas->setResolutionFraction(mSavedResolutionFraction);
        }
        mCurrentCanvas->setNoCache(false);
    } else {
        mCurrentRenderFrame++;
        emit changeCurrentFrame(mCurrentRenderFrame);
        if(mNoBoxesAwaitUpdate) {
            nextSaveOutputFrame();
        }
    }
}

Canvas *CanvasWindow::loadCanvasesFromSql() {
    QSqlQuery query;

    QString queryStr = "SELECT * FROM canvas";
    if(query.exec(queryStr)) {
        while(query.next()) {
            int idWidth = query.record().indexOf("width");
            int idHeight = query.record().indexOf("height");
            int idFrameCount = query.record().indexOf("framecount");
            int idBoundingBoxId = query.record().indexOf("boundingboxid");

            int width = query.value(idWidth).toInt();
            int height = query.value(idHeight).toInt();
            int frameCount = query.value(idFrameCount).toInt();
            int boundingBoxId = query.value(idBoundingBoxId).toInt();

            Canvas *canvas =
                    new Canvas(MainWindow::getInstance()->getFillStrokeSettings(),
                               this,
                               width, height,
                               frameCount);
            canvas->prp_loadFromSql(boundingBoxId);
            MainWindow::getInstance()->addCanvas(canvas);
            return canvas;
        }
    } else {
        qDebug() << "Could not load canvases";
    }
    return NULL;
}

void CanvasWindow::saveCanvasesFromSql(QSqlQuery *query) {
    Q_FOREACH(Canvas *canvas, mCanvasList) {
        canvas->saveToSql(query);
    }
}

void CanvasWindow::saveOutput(const QString &renderDest,
                              const qreal &resolutionFraction) {
    mOutputString = renderDest;
    mBoxesUpdateFinishedFunction = &CanvasWindow::nextSaveOutputFrame;
    mSavedCurrentFrame = getCurrentFrame();
    mCurrentCanvas->fitCanvasToSize();
    mSavedResolutionFraction = mCurrentCanvas->getResolutionFraction();
    if(qAbs(mSavedResolutionFraction - resolutionFraction) > 0.001) {
        mCurrentCanvas->setResolutionFraction(resolutionFraction);
    }

    mCurrentRenderFrame = mSavedCurrentFrame;
    mCurrentCanvas->prp_setAbsFrame(mSavedCurrentFrame);
    mCurrentCanvas->setOutputRendering(true);
    mCurrentCanvas->updateAllBoxes();
    mCurrentCanvas->setNoCache(true);
    if(mNoBoxesAwaitUpdate) {
        nextSaveOutputFrame();
    }
}

void CanvasWindow::clearAll() {
    //SWT_clearAll();
    mCanvasList.clear();
    setCurrentCanvas((Canvas*)NULL);
}

void CanvasWindow::createLinkToFileWithPath(const QString &path) {
    if(hasNoCanvas()) return;
    mCurrentCanvas->createLinkToFileWithPath(path);
}

void CanvasWindow::createAnimationBoxForPaths(
        const QStringList &importPaths) {
    if(hasNoCanvas()) return;
    mCurrentCanvas->createAnimationBoxForPaths(importPaths);
}

VideoBox *CanvasWindow::createVideoForPath(const QString &path) {
    if(hasNoCanvas()) return NULL;
    return mCurrentCanvas->createVideoForPath(path);
}

ImageBox *CanvasWindow::createImageForPath(const QString &path) {
    if(hasNoCanvas()) return NULL;
    return mCurrentCanvas->createImageBox(path);
}

SingleSound *CanvasWindow::createSoundForPath(const QString &path) {
    if(hasNoCanvas()) return NULL;
    return mCurrentCanvas->createSoundForPath(path);
}

void CanvasWindow::saveToSql(QSqlQuery *query) {
    if(hasNoCanvas()) return;
    mCurrentCanvas->saveToSql(query);
}

int CanvasWindow::getCurrentFrame() {
    if(hasNoCanvas()) return 0;
    return mCurrentCanvas->getCurrentFrame();
}

int CanvasWindow::getMaxFrame() {
    if(hasNoCanvas()) return 0;
    return mCurrentCanvas->getMaxFrame();
}

void CanvasWindow::updateHoveredElements() {
    if(hasNoCanvas()) return;
    mCurrentCanvas->updateHoveredElements();
}

void CanvasWindow::switchLocalPivot() {
    setLocalPivot(!mCurrentCanvas->getPivotLocal());
}

bool CanvasWindow::getLocalPivot() {
    if(hasNoCanvas()) return false;
    return mCurrentCanvas->getPivotLocal();
}

void CanvasWindow::setLocalPivot(const bool &bT) {
    if(hasNoCanvas()) return;
    mCurrentCanvas->setLocalPivot(bT);
}

const int BufferSize = 32768;

void CanvasWindow::initializeAudio() {
    mAudioBuffer = QByteArray(BufferSize, 0);
    connect(mPreviewFPSTimer, SIGNAL(timeout()),
            this, SLOT(pushTimerExpired()));

    mAudioDevice = QAudioDeviceInfo::defaultOutputDevice();
    mAudioFormat.setSampleRate(SAMPLERATE);
    mAudioFormat.setChannelCount(1);
    mAudioFormat.setSampleSize(32);
    mAudioFormat.setCodec("audio/pcm");
    mAudioFormat.setByteOrder(QAudioFormat::LittleEndian);
    mAudioFormat.setSampleType(QAudioFormat::Float);

    QAudioDeviceInfo info(mAudioDevice);
    if (!info.isFormatSupported(mAudioFormat)) {
        qWarning() << "Default format not supported - trying to use nearest";
        mAudioFormat = info.nearestFormat(mAudioFormat);
    }

    mAudioOutput = new QAudioOutput(mAudioDevice, mAudioFormat, this);
}

void CanvasWindow::startAudio() {
    mCurrentSoundComposition->start();
    mAudioIOOutput = mAudioOutput->start();
}

void CanvasWindow::stopAudio() {
    //mAudioOutput->suspend();
    //mCurrentSoundComposition->stop();
    mAudioIOOutput = NULL;
    mAudioOutput->stop();
    mAudioOutput->reset();
    mCurrentSoundComposition->stop();
}

void CanvasWindow::volumeChanged(int value) {
    if(mAudioOutput) {
        mAudioOutput->setVolume(qreal(value/100.));
    }
}

void CanvasWindow::pushTimerExpired() {
    if(mAudioOutput && mAudioOutput->state() != QAudio::StoppedState) {
        int chunks = mAudioOutput->bytesFree()/mAudioOutput->periodSize();
        while(chunks) {
           const qint64 len = mCurrentSoundComposition->read(
                                                mAudioBuffer.data(),
                                                mAudioOutput->periodSize());
           if(len) {
               mAudioIOOutput->write(mAudioBuffer.data(), len);
           }
           if(len != mAudioOutput->periodSize()) {
               break;
           }
           --chunks;
        }
    }
}

void CanvasWindow::dropEvent(QDropEvent *event) {
    const QMimeData* mimeData = event->mimeData();

    if(mimeData->hasUrls()) {
        QList<QUrl> urlList = mimeData->urls();

        for (int i = 0; i < urlList.size() && i < 32; i++) {
            importFile(urlList.at(i).toLocalFile(),
                       event->posF());
        }
    }
}

void CanvasWindow::dragEnterEvent(QDragEnterEvent *event) {
    if(event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

#include "svgimporter.h"
#include <QFileDialog>
void CanvasWindow::importFile(const QString &path,
                              const QPointF &absDropPos) {
    if(hasNoCanvas()) return;
    MainWindow::getInstance()->disable();

    QFile file(path);
    if(!file.exists()) {
        return;
    }

    QString extension = path.split(".").last();
    BoundingBox *boxToPosition = NULL;
    if(extension == "svg") {
        boxToPosition = loadSVGFile(path, mCurrentCanvas);
    } else if(extension == "png" ||
              extension == "jpg") {
        boxToPosition = createImageForPath(path);
    } else if(extension == "avi" ||
              extension == "mp4" ||
              extension == "mov") {
        boxToPosition = createVideoForPath(path);
    } else if(extension == "mp3" ||
              extension == "wav") {
        createSoundForPath(path);
    } else if(extension == "av") {
        MainWindow::getInstance()->loadAVFile(path);
    }
    if(boxToPosition != NULL) {
        QPointF trans = mCurrentCanvas->mapCanvasAbsToRel(absDropPos);
        trans -= boxToPosition->mapRelPosToAbs(
                    boxToPosition->getRelCenterPosition());
        boxToPosition->moveByAbs(trans);
    }
    updateHoveredElements();
    MainWindow::getInstance()->enable();

    MainWindow::getInstance()->callUpdateSchedulers();
}

QWidget *CanvasWindow::getCanvasWidget() {
    return mCanvasWidget;
}

void CanvasWindow::grabMouse() {
    mMouseGrabber = true;
    setMouseGrabEnabled(true);
    //mCanvasWidget->grabMouse();
}

void CanvasWindow::releaseMouse() {
    mMouseGrabber = false;
    setMouseGrabEnabled(false);

    //        QWidget *grabber = mWidgetContainer->mouseGrabber();
    //mCanvasWidget->releaseMouse();
    //        grabber = mWidgetContainer->mouseGrabber();
}

bool CanvasWindow::isMouseGrabber() {
    return mMouseGrabber;
}

void CanvasWindow::repaint() {
    //mCanvasWidget->update();
    requestUpdate();
}

QRect CanvasWindow::rect() {
    return mCanvasWidget->rect();
}

void CanvasWindow::importFile() {
    MainWindow::getInstance()->disableEventFilter();
    QStringList importPaths = QFileDialog::getOpenFileNames(
                                            MainWindow::getInstance(),
                                            "Import File", "",
                                            "Files (*.av *.svg "
                                                   "*.mp4 *.mov *.avi "
                                                   "*.png *.jpg "
                                                   "*.wav *.mp3)");
    MainWindow::getInstance()->enableEventFilter();
    if(!importPaths.isEmpty()) {
        Q_FOREACH(const QString &path, importPaths) {
            if(path.isEmpty()) continue;
            importFile(path);
        }
    }
}
