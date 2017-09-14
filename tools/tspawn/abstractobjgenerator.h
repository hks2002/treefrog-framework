#ifndef ABSTRACTOBJGENERATOR_H
#define ABSTRACTOBJGENERATOR_H

#include <QString>
#include <QList>
#include <QPair>
#include <QVariant>


class AbstractObjGenerator
{
public:
    virtual ~AbstractObjGenerator() { }
    virtual QString generate(const QString &dstDir) = 0;
    virtual QStringList fieldList() const {return QStringList();}
    virtual QStringList refTableList() const {return QStringList();}
    virtual QList<QStringList> refTableFieldList() const { QList<QStringList> list; return list;}
    virtual QStringList reffedTableList() const {return QStringList();}
    virtual QList<QStringList> reffedTableFieldList() const { QList<QStringList> list; return list;}
    virtual QStringList fieldTypeList() const {return QStringList();}
    virtual QList<int> primaryKeyIndexList() const { QList<int> pkidxList; return pkidxList; }
    virtual int autoValueIndex() const { return -1; }
    virtual int lockRevisionIndex() const { return -1; }
};

#endif // ABSTRACTOBJGENERATOR_H
