#ifndef BOXTARGETPROPERTY_H
#define BOXTARGETPROPERTY_H
#include <QtCore>
#include "property.h"
class BoundingBox;
class QrealAnimator;

class BoxTargetProperty : public Property {
public:
    BoxTargetProperty();

    BoundingBox *getTarget();
    void setTarget(BoundingBox *box);
    SWT_Type SWT_getType() { return SWT_BoxTarget; }
    void setParentBox(BoundingBox *box);

    void makeDuplicate(Property *property);
    Property *makeDuplicate();
    int prp_saveToSql(QSqlQuery *query,
                      const int &parentId) {}
    void prp_loadFromSql(const int &identifyingId) {}
private:
    BoundingBox *mTarget = NULL;
    BoundingBox *mParentBox = NULL;
};

#endif // BOXTARGETPROPERTY_H
