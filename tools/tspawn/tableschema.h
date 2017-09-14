#ifndef TABLESCHEMA_H
#define TABLESCHEMA_H

#include <QString>
#include <QList>
#include <QPair>
#include <QSqlRecord>
#include <QVariant>


class TableSchema
{
public:
    TableSchema(const QString &table, const QString &env = "dev");
    bool exists() const;
    QStringList fieldList() const;
    QStringList fieldTypeList() const;
    QStringList refTableList() const;
    QList<QStringList> refTableFieldList() const;
    QStringList reffedTableList() const;
    QList<QStringList> reffedTableFieldList() const;
    QList<int> primaryKeyIndexList() const;
    int autoValueIndex() const;
    int lockRevisionIndex() const;
    QString tableName() const { return tablename; }
    bool hasLockRevisionField() const;
    static QStringList databaseDrivers();
    static QStringList tables(const QString &env = "dev");

protected:
    bool openDatabase(const QString &env) const;
    bool isOpen() const;

private:
    QString tablename;
    QSqlRecord tableFields;
};

#endif // TABLESCHEMA_H
