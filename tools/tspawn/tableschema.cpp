/* Copyright (c) 2010-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QtCore>
#include <QtSql>
#include "tableschema.h"
#include "global.h"

QSettings *dbSettings = 0;


TableSchema::TableSchema(const QString &table, const QString &env)
    : tablename(table)
{
    if (!dbSettings) {
        QString path = QLatin1String("config") + QDir::separator() + "database.ini";

        if (!QFile::exists(path)) {
            qCritical("not found, %s", qPrintable(path));
        }

        dbSettings = new QSettings(path, QSettings::IniFormat);
    }

    if (openDatabase(env)) {
        if (!tablename.isEmpty()) {
            QSqlTableModel model;
            model.setTable(tablename);
            tableFields = model.record();

            if (model.database().driverName().toUpper() == "QPSQL") {
                // QPSQLResult doesn't call QSqlField::setAutoValue(), fix it
                for (int i = 0; i < tableFields.count(); ++i) {
                    QSqlField f = tableFields.field(i);

                    if (f.defaultValue().toString().startsWith(QLatin1String("nextval"))) {
                        f.setAutoValue(true);
                        tableFields.replace(i, f);
                    }
                }
            }
        }
        else {
            qCritical("Empty table name");
        }
    }
}


bool TableSchema::exists() const
{
    return (tableFields.count() > 0);
}

QStringList TableSchema::fieldList() const
{
    QStringList fieldList;

    for (int i = 0; i < tableFields.count(); ++i) {
        fieldList << tableFields.fieldName(i);
    }

    return fieldList;
}

QStringList TableSchema::fieldTypeList() const
{
    QStringList fieldList;

    for (int i = 0; i < tableFields.count(); ++i) {
        QSqlField f = tableFields.field(i);
        fieldList << QString(QVariant::typeToName(f.type()));
    }

    return fieldList;
}

QList<int> TableSchema::primaryKeyIndexList() const
{
    QSqlTableModel model;
    model.setTable(tablename);
    QSqlIndex index = model.primaryKey();

    refTableList();
    //getRefTableFieldList();

    QList<int> pkidxs;

    for (int i = 0; i < index.count(); ++i) {
        pkidxs << model.record().indexOf(index.fieldName(i));
    }

    return pkidxs;
}

QStringList TableSchema::refTableList() const
{
    QStringList reftablelist;
    QString str;
    str.append("SELECT cl1.relname FROM pg_constraint con ");
    str.append("INNER JOIN pg_class cl1 ON cl1.oid = con.confrelid ");
    str.append("WHERE con.contype = 'f' AND con.conrelid = '%1'::regclass");
    str = str.arg(tablename);

    QSqlQuery query;
    query.exec(str);

    while (query.next()) {
        reftablelist << query.value(0).toString();
    }

    return reftablelist;
}

QList<QStringList> TableSchema::refTableFieldList() const
{
    QList<QStringList> reftablefieldlist;
    QString str;
    str.append("SELECT array_to_string(con.conkey,' ') FROM pg_constraint con ");
    str.append("INNER JOIN pg_class cl1 ON cl1.oid = con.confrelid ");
    str.append("WHERE con.contype = 'f' AND con.conrelid = '%1'::regclass");
    str = str.arg(tablename);

    QSqlQuery query;
    query.exec(str);

    while (query.next()) {

        QStringList reftablefields = query.value(0).toString().split(" ");
        QStringList fieldList;

        for (QString idx : reftablefields) {
            //This max attnum maybe not equal total field count.
            //Because delete field still have a attnum.
            QString str2;
            str2.append("SELECT att.attname FROM pg_attribute att ");
            str2.append("WHERE att.attrelid='%1'::regclass and att.attnum='%2'");
            str2 = str2.arg(tablename, idx);

            QSqlQuery query2;
            query2.exec(str2);

            while (query2.next()) {
                fieldList << query2.value(0).toString();
            }
        }

        reftablefieldlist << fieldList;
    }

    return reftablefieldlist;
}

QStringList TableSchema::reffedTableList() const
{
    QStringList reffedtablelist;
    QString str;
    str.append("SELECT cl1.relname FROM pg_constraint con ");
    str.append("INNER JOIN pg_class cl1 ON cl1.oid = con.conrelid ");
    str.append("WHERE con.contype = 'f' AND con.confrelid = '%1'::regclass");
    str = str.arg(tablename);

    QSqlQuery query;
    query.exec(str);

    while (query.next()) {
        reffedtablelist << query.value(0).toString();
    }

    return reffedtablelist;
}

QList<QStringList> TableSchema::reffedTableFieldList() const
{
    QList<QStringList> reffedtablefieldlist;
    QString str;
    str.append("SELECT array_to_string(con.conkey,' ') FROM pg_constraint con ");
    str.append("INNER JOIN pg_class cl1 ON cl1.oid = con.conrelid ");
    str.append("WHERE con.contype = 'f' AND con.confrelid = '%1'::regclass");
    str = str.arg(tablename);

    QSqlQuery query;
    query.exec(str);

    while (query.next()) {
        QStringList reffedtablefields = query.value(0).toString().split(" ");
        QStringList fieldList;

        for (QString idx : reffedtablefields) {
            //This max attnum maybe not equal total field count.
            //Because delete field still have a attnum.
            QString str2;
            str2.append("SELECT att.attname FROM pg_attribute att ");
            str2.append("WHERE att.attrelid='%1'::regclass and att.attnum='%2'");
            str2 = str2.arg(tablename, idx);

            QSqlQuery query2;
            query2.exec(str2);

            while (query2.next()) {
                fieldList << query2.value(0).toString();
            }
        }

        reffedtablefieldlist << fieldList;
    }

    return reffedtablefieldlist;
}

int TableSchema::autoValueIndex() const
{
    for (int i = 0; i < tableFields.count(); ++i) {
        QSqlField f = tableFields.field(i);

        if (f.isAutoValue()) {
            return i;
        }
    }

    return -1;
}


int TableSchema::lockRevisionIndex() const
{
    for (int i = 0; i < tableFields.count(); ++i) {
        QSqlField f = tableFields.field(i);

        if (fieldNameToVariableName(f.name()) == "lockRevision") {
            return i;
        }
    }

    return -1;
}


bool TableSchema::hasLockRevisionField() const
{
    return lockRevisionIndex() >= 0;
}


bool TableSchema::openDatabase(const QString &env) const
{
    if (isOpen()) {
        return true;
    }

    if (!dbSettings->childGroups().contains(env)) {
        qCritical("invalid environment: %s", qPrintable(env));
        return false;
    }

    dbSettings->beginGroup(env);

    QString driverType = dbSettings->value("DriverType").toString().trimmed();

    if (driverType.isEmpty()) {
        qWarning("Parameter 'DriverType' is empty");
    }

    printf("DriverType:   %s\n", qPrintable(driverType));

    QSqlDatabase db = QSqlDatabase::addDatabase(driverType);

    if (!db.isValid()) {
        qWarning("Parameter 'DriverType' is invalid or RDB client library not available.");
        return false;
    }

    QString databaseName = dbSettings->value("DatabaseName").toString().trimmed();
    printf("DatabaseName: %s\n", qPrintable(databaseName));

    if (!databaseName.isEmpty()) {
        db.setDatabaseName(databaseName);
    }

    QString hostName = dbSettings->value("HostName").toString().trimmed();
    printf("HostName:     %s\n", qPrintable(hostName));

    if (!hostName.isEmpty()) {
        db.setHostName(hostName);
    }

    int port = dbSettings->value("Port").toInt();

    if (port > 0) {
        db.setPort(port);
    }

    QString userName = dbSettings->value("UserName").toString().trimmed();

    if (!userName.isEmpty()) {
        db.setUserName(userName);
    }

    QString password = dbSettings->value("Password").toString().trimmed();

    if (!password.isEmpty()) {
        db.setPassword(password);
    }

    QString connectOptions = dbSettings->value("ConnectOptions").toString().trimmed();

    if (!connectOptions.isEmpty()) {
        db.setConnectOptions(connectOptions);
    }

    dbSettings->endGroup();

    if (!db.open()) {
        qWarning("Database open error");
        return false;
    }

    printf("Database opened successfully\n");
    return true;
}


bool TableSchema::isOpen() const
{
    return QSqlDatabase::database().isOpen();
}


QStringList TableSchema::databaseDrivers()
{
    return QSqlDatabase::drivers();
}


QStringList TableSchema::tables(const QString &env)
{
    QSet<QString> set;
    TableSchema dummy("dummy", env);  // to open database

    if (QSqlDatabase::database().isOpen()) {
        for (QStringListIterator i(QSqlDatabase::database().tables(QSql::Tables)); i.hasNext();) {
            TableSchema t(i.next());

            if (t.exists()) {
                set << t.tableName(); // If value already exists, the set is left unchanged
            }
        }
    }

    QStringList ret = set.toList();
    qSort(ret);
    return ret;
}
