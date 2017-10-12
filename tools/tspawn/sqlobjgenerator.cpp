/* Copyright (c) 2010-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "sqlobjgenerator.h"
#include "global.h"
#include "filewriter.h"
#include "tableschema.h"

#define USER_VIRTUAL_METHOD  "identityKey"
#define LOCK_REVISION_FIELD  "lock_revision"

#define SQLOBJECT_HEADER_TEMPLATE                            \
    "#ifndef %1OBJECT_H\n"                                   \
    "#define %1OBJECT_H\n"                                   \
    "\n"                                                     \
    "#include <TSqlObject>\n"                                \
    "#include <QSharedData>\n"                               \
    "\n\n"                                                   \
    "class T_MODEL_EXPORT %2Object : public TSqlObject, public QSharedData\n" \
    "{\n"                                                    \
    "public:\n"

#define SQLOBJECT_PROPERTY_TEMPLATE                  \
    "    Q_PROPERTY(%1 %2 READ get%2 WRITE set%2)\n" \
    "    T_DEFINE_PROPERTY(%1, %2)\n"

#define SQLOBJECT_FOOTER_TEMPLATE  \
    "};\n"                         \
    "\n"                           \
    "#endif // %1OBJECT_H\n"


static bool isNumericType(const QString &typeName)
{
    switch (QMetaType::type(typeName.toLatin1().data())) {
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::Long:
    case QMetaType::LongLong:
    case QMetaType::Short:
    case QMetaType::Char:
    case QMetaType::UChar:
    case QMetaType::ULong:
    case QMetaType::ULongLong:
    case QMetaType::UShort:
    case QMetaType::Double:
    case QMetaType::Float:
#if QT_VERSION >= 0x050000
    case QMetaType::SChar:
#endif
        return true;

    default:
        return false;
    }
}


SqlObjGenerator::SqlObjGenerator(const QString &model, const QString &table)
    : modelName(), tableSch(new TableSchema(table))
{
    modelName = (!model.isEmpty()) ? model : fieldNameToEnumName(table);
}


SqlObjGenerator::~SqlObjGenerator()
{
    delete tableSch;
}


QString SqlObjGenerator::generate(const QString &dstDir)
{
    QStringList fieldList = tableSch->fieldList();
    QStringList fieldTypeList = tableSch->fieldTypeList();

    if (fieldList.isEmpty()) {
        qCritical("table not found, %s", qPrintable(tableSch->tableName()));
        return QString();
    }

    QString output;

    // Header part
    output += QString(SQLOBJECT_HEADER_TEMPLATE).arg(modelName.toUpper(), modelName);

    for (int i = 0; i < fieldList.length(); ++i) {
        QString fieldType = fieldTypeList[i];

        if (isNumericType(fieldType)) {
            output += QString("    %1 %2 {0};\n").arg(fieldType, fieldList[i]);
        }
        else {
            output += QString("    %1 %2;\n").arg(fieldType, fieldList[i]);
        }
    }

    // enum part
    output += QLatin1String("\n    enum PropertyIndex {\n");
    output += QString("        %1 = 0,\n").arg(fieldNameToEnumName(fieldList[0]));

    for (int i = 1; i < fieldList.length(); ++i) {
        output += QString("        %1,\n").arg(fieldNameToEnumName(fieldList[i]));
    }

    output += QLatin1String("    };\n\n");

    // primaryKeyIndexList() method
    output += QLatin1String("    QList<int> primaryKeyIndexList() const override { QList<int> pkidxList; return pkidxList");
    QList<int> primaryKeyIndexList = tableSch->primaryKeyIndexList();

    for (auto pkidx : primaryKeyIndexList) {
        output += QLatin1String("<<");
        output += fieldNameToEnumName(fieldList[pkidx]);
    }

    output += QLatin1String("; }\n");

    // auto-value field, for example auto-increment value
    output += QLatin1String("    int autoValueIndex() const override { return ");
    int autoValue = tableSch->autoValueIndex();

    if (autoValue == -1) {
        output += QLatin1String("-1; }\n");
    }
    else {
        output += fieldNameToEnumName(fieldList[autoValue]);
        output += QLatin1String("; }\n");
    }

	// foreignKeyFieldList() method
    output += QLatin1String("    QList<int> foreignKeyIndexList() const { QList<int> fkIdxList;return fkIdxList");
    QList<QStringList> foreignKeyFieldList = tableSch->refTableFieldList();

	for (auto &fkFieldGroup : foreignKeyFieldList){
		 for (auto &fkField : fkFieldGroup) {
			output += QLatin1String("<<");
			output += fieldNameToEnumName(fkField);
       }
	}

    output += QLatin1String("; }\n");
	
    // tableName() method
    output += QLatin1String("    QString tableName() const override { return QLatin1String(\"");
    output += tableSch->tableName();
    output += QLatin1String("\"); }\n\n");

    // Property macros part
    output += QLatin1String("private:    /*** Don't modify below this line ***/\n    Q_OBJECT\n");

    for (int i = 0; i < fieldList.length(); ++i) {
        output += QString(SQLOBJECT_PROPERTY_TEMPLATE).arg(fieldTypeList[i], fieldList[i]);
    }

    // Footer part
    output += QString(SQLOBJECT_FOOTER_TEMPLATE).arg(modelName.toUpper());

    // Writes to a file
    QDir dst = QDir(dstDir).filePath("sqlobjects");
    FileWriter fw(dst.filePath(modelName.toLower() + "object.h"));
    fw.write(output, false);
    return QLatin1String("sqlobjects/") + fw.fileName();
}


QStringList SqlObjGenerator::fieldList() const
{
    return tableSch->fieldList();
}

QStringList SqlObjGenerator::fieldTypeList() const
{
    return tableSch->fieldTypeList();
}

QList<int> SqlObjGenerator::primaryKeyIndexList() const
{
    return tableSch->primaryKeyIndexList();
}

QStringList SqlObjGenerator::refTableList() const
{
    return tableSch->refTableList();

}

QList<QStringList> SqlObjGenerator::refTableFieldList() const
{
    return tableSch->refTableFieldList();
}

QStringList SqlObjGenerator::reffedTableList() const
{
    return tableSch->reffedTableList();

}

QList<QStringList> SqlObjGenerator::reffedTableFieldList() const
{
    return tableSch->reffedTableFieldList();
}

int SqlObjGenerator::autoValueIndex() const
{
    return tableSch->autoValueIndex();
}

int SqlObjGenerator::lockRevisionIndex() const
{
    return tableSch->lockRevisionIndex();
}
