#ifndef SQLOBJGENERATOR_H
#define SQLOBJGENERATOR_H

#include <QStringList>
#include <QDir>
#include <QPair>
#include "abstractobjgenerator.h"

class TableSchema;


class SqlObjGenerator : public AbstractObjGenerator
{
public:
    SqlObjGenerator(const QString &model, const QString &table);
    ~SqlObjGenerator();
    QString generate(const QString &dstDir);
    QStringList fieldList() const;
    QStringList fieldTypeList() const;
    QList<int> primaryKeyIndexList() const;
    QStringList refTableList() const;
    QList<QStringList> refTableFieldList() const;
    QStringList reffedTableList() const;
    QList<QStringList> reffedTableFieldList() const;
    int autoValueIndex() const;
    int lockRevisionIndex() const;
    QString model() const { return modelName; }

private:
    QString modelName;
    TableSchema *tableSch;
};

#endif // SQLOBJGENERATOR_H
