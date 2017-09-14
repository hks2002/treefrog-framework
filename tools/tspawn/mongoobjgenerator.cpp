/* Copyright (c) 2013-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "mongoobjgenerator.h"
#include "global.h"
#include "projectfilegenerator.h"
#include "filewriter.h"
#include <QPair>
#include <QRegExp>
#ifndef Q_CC_MSVC
# include <unistd.h>
#endif

#define MONGOOBJECT_HEADER_TEMPLATE                                     \
    "#ifndef %1OBJECT_H\n"                                              \
    "#define %1OBJECT_H\n"                                              \
    "\n"                                                                \
    "#include <TMongoObject>\n"                                         \
    "#include <QSharedData>\n"                                          \
    "\n\n"                                                              \
    "class T_MODEL_EXPORT %2Object : public TMongoObject, public QSharedData\n" \
    "{\n"                                                               \
    "public:\n"                                                         \
    "%3"                                                                \
    "\n"                                                                \
    "    enum PropertyIndex {\n"                                        \
    "%4"                                                                \
    "    };\n"                                                          \
    "\n"                                                                \
    "    virtual QString collectionName() const override { return QLatin1String(\"%5\"); }\n" \
    "    virtual QString objectId() const override { return _id; }\n"   \
    "    virtual QString &objectId() override { return _id; }\n"        \
    "\n"                                                                \
    "private:\n"                                                        \
    "    Q_OBJECT\n"                                                    \
    "%6"                                                                \
    "};\n"                                                              \
    " \n"                                                               \
    "#endif // %1OBJECT_H"

#define MONGOOBJECT_HEADER_UPDATE_TEMPLATE                              \
    "%3\n"                                                              \
    "%4"                                                                \
    "\n"                                                                \
    "    enum PropertyIndex {\n"                                        \
    "%5"                                                                \
    "    };\n"                                                          \
    "\n"                                                                \
    "    virtual QString collectionName() const override { return QLatin1String(\"%2\"); }\n" \
    "    virtual QString objectId() const override { return _id; }\n"   \
    "    virtual QString &objectId() override { return _id; }\n"        \
    "\n"                                                                \
    "private:\n"                                                        \
    "    Q_OBJECT\n"                                                    \
    "%6"                                                                \
    "};\n"                                                              \
    " \n"                                                               \
    "#endif // %1OBJECT_H"

#define MONGOOBJECT_PROPERTY_TEMPLATE                \
    "    Q_PROPERTY(%1 %2 READ get%2 WRITE set%2)\n" \
    "    T_DEFINE_PROPERTY(%1, %2)\n"

const QRegExp rxstart("\\{\\s*public\\s*:", Qt::CaseSensitive, QRegExp::RegExp2);


MongoObjGenerator::MongoObjGenerator(const QString &model)
    : modelName(), collectionName(model), fieldlist()
{
    modelName = fieldNameToEnumName(model);
}


QString MongoObjGenerator::mongoObjectFilePath(const QString &dstDir) const
{
    return QDir(dstDir + "/mongoobjects").filePath(modelName.toLower() + "object.h");
}


QString MongoObjGenerator::generate(const QString &dstDir)
{
    QString mobjpath = mongoObjectFilePath(dstDir);
    QFileInfo fi(mobjpath);

    if (fi.exists()) {
        updateMongoObject(mobjpath);
    }
    else {
        QDir dir = fi.dir();

        if (!dir.exists()) {
            dir.mkpath(".");
        }

        createMongoObject(mobjpath);
    }

    return QLatin1String("mongoobjects/") + QFileInfo(mobjpath).fileName();
}


static QStringList generateCode(const QStringList &fieldList, const QStringList &fieldTypeList)
{
    QString params, enums, macros;

    for (int i = 0; i < fieldList.length(); ++i) {
        params += QString("    %1 %2;\n").arg(fieldTypeList[i], fieldList[i]);
        macros += QString(MONGOOBJECT_PROPERTY_TEMPLATE).arg(fieldTypeList[i], fieldList[i]);
        QString estr = fieldNameToEnumName(fieldList[i]);
        enums  += (enums.isEmpty()) ? QString("        %1 = 0,\n").arg(estr) : QString("        %1,\n").arg(
                      estr);
    }

    return QStringList() << params << enums << macros;
}


bool MongoObjGenerator::createMongoObject(const QString &path)
{
    fieldlist << QString("_id") << QString("createdAt") << QString("updatedAt") <<
              QString("lockRevision");
    fieldtypelist << QString("String") << QString("DateTime") << QString("DateTime") << QString("Int");
    QStringList code = generateCode(fieldlist, fieldtypelist);
    QString output = QString(MONGOOBJECT_HEADER_TEMPLATE).arg(modelName.toUpper(), modelName, code[0],
                     code[1], collectionName, code[2]);
    // Writes to a file
    return FileWriter(path).write(output, false);
}


void MongoObjGenerator::updateFieldAndTypeList(const QString &filePath)
{
    QStringList fields, fieldTypes;
    QRegExp rxend("(\\n[\\t\\r ]*\\n|\\senum\\s)", Qt::CaseSensitive, QRegExp::RegExp2);
    QRegExp rx("\\s([a-zA-Z0-9_<>]+)\\s+([a-zA-Z0-9_]+)\\s*;", Qt::CaseSensitive, QRegExp::RegExp2);

    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical("file open error: %s", qPrintable(filePath));
        _exit(1);
    }

    QString src = QString::fromUtf8(file.readAll().data());
    int pos = rxstart.indexIn(src, 0);

    if (pos < 0) {
        qCritical("parse error");
        _exit(1);
    }

    pos += rxstart.matchedLength();

    int end = rxend.indexIn(src, pos);

    while ((pos = rx.indexIn(src, pos)) > 0 && pos < end) {
        QVariant::Type type = QVariant::nameToType(rx.cap(1).toLatin1().data());
        QString var = rx.cap(2);

        if (type != QVariant::Invalid && var.toLower() != "id") {
            fields << var;
            fieldTypes << rx.cap(1).toLatin1().data();
        }

        pos += rx.matchedLength();
    }

    fieldlist = fields;
    fieldtypelist = fieldTypes;
}

bool MongoObjGenerator::updateMongoObject(const QString &path)
{
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical("file open error: %s", qPrintable(path));
        _exit(1);
    }

    QString src = QString::fromUtf8(file.readAll().data());
    QString headerpart;

    int pos = rxstart.indexIn(src, 0);

    if (pos > 0) {
        pos += rxstart.matchedLength();
        headerpart = src.mid(0, pos);
    }

    updateFieldAndTypeList(path);
    QStringList prop = generateCode(fieldList(), fieldTypeList());
    QString output = QString(MONGOOBJECT_HEADER_UPDATE_TEMPLATE).arg(modelName.toUpper(),
                     collectionName, headerpart, prop[0], prop[1], prop[2]);
    // Writes to a file
    return FileWriter(path).write(output, true);
}


QList<int> MongoObjGenerator::primaryKeyIndexList() const
{
    QList<int> pkidxList;

    if (fieldlist.isEmpty()) {
        qCritical("Mongo file not generated");
        return pkidxList;
    }

    for (int i = 0; i < fieldlist.count(); ++i) {
        if (fieldlist[i] == "_id") {
            return pkidxList << i;
        }
    }

    return pkidxList;
}


int MongoObjGenerator::autoValueIndex() const
{
    if (primaryKeyIndexList().isEmpty()) {
        return -1;
    }
    else {
        return primaryKeyIndexList()[0];
    }
}


int MongoObjGenerator::lockRevisionIndex() const
{
    if (fieldlist.isEmpty()) {
        qCritical("Mongo file not generated");
        return -1;
    }

    for (int i = 0; i < fieldlist.count(); ++i) {
        QString var = fieldNameToVariableName(fieldlist[i]);

        if (var == "lockRevision") {
            return i;
        }
    }

    return -1;

}
