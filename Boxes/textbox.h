#ifndef TEXTBOX_H
#define TEXTBOX_H
#include "Boxes/pathbox.h"
#include "skiaincludes.h"
class QStringAnimator;
typedef QSharedPointer<QStringAnimator> QStringAnimatorQSPtr;

class TextBox : public PathBox {
public:
    TextBox();

    void setFont(const QFont &font, const bool &saveUndoRedo = true);
    void setSelectedFontSize(const qreal &size);
    void setSelectedFontFamilyAndStyle(const QString &fontFamily,
                                       const QString &fontStyle);

    qreal getFontSize();
    QString getFontFamily();
    QString getFontStyle();

    void openTextEditor(const bool &saveUndoRedo = true);
    MovablePoint *getPointAtAbsPos(const QPointF &absPtPos,
                             const CanvasMode &currentCanvasMode,
                             const qreal &canvasScaleInv);
//    QRectF getTextRect();
    void setPathText(bool pathText);

    void setTextAlignment(const Qt::Alignment &alignment) {
        mAlignment = alignment;
        scheduleUpdate();
    }

    bool SWT_isTextBox() { return true; }
    void addActionsToMenu(QMenu *menu);
    bool handleSelectedCanvasAction(QAction *selectedAction);
    SkPath getPathAtRelFrame(const int &relFrame);
    SkPath getPathAtRelFrameF(const qreal &relFrame);
    void setCurrentTextValue(const QString &text,
                             const bool &saveUndoRedo = true);

    void writeBoundingBox(QIODevice *target);
    void readBoundingBox(QIODevice *target);
private:
    QStringAnimatorQSPtr mText;
    QFont mFont;
    Qt::Alignment mAlignment = Qt::AlignLeft;
};

#endif // TEXTBOX_H
