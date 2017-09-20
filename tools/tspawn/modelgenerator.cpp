/* Copyright (c) 2010-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "modelgenerator.h"
#include "sqlobjgenerator.h"
#include "mongoobjgenerator.h"
#include "global.h"
#include "projectfilegenerator.h"
#include "filewriter.h"
#include "util.h"

#define USER_VIRTUAL_METHOD  "identityKey"
#define LOCK_REVISION_FIELD "lockRevision"

#define MODEL_HEADER_FILE_TEMPLATE                       \
    "#ifndef %1_H\n"                                     \
    "#define %1_H\n"                                     \
    "\n"                                                 \
    "#include <QStringList>\n"                           \
    "#include <QDateTime>\n"                             \
    "#include <QVariant>\n"                              \
    "#include <QSharedDataPointer>\n"                    \
    "#include <TGlobal>\n"                               \
    "#include <TAbstractModel>\n"                        \
    "\n"                                                 \
    "class TModelObject;\n"                              \
    "class %2Object;\n"                                  \
    "%7"                                                 \
    "\n\n"                                               \
    "class T_MODEL_EXPORT %2 : public TAbstractModel\n"  \
    "{\n"                                                \
    "public:\n"                                          \
    "    %2();\n"                                        \
    "    %2(const %2 &other);\n"                         \
    "    %2(const %2Object &object);\n"                  \
    "    ~%2();\n"                                       \
    "\n"                                                 \
    "%3"                                                 \
    "    %2 &operator=(const %2 &other);\n"              \
    "\n"                                                 \
    "    bool create() override { return TAbstractModel::create(); }\n" \
    "    bool update() override { return TAbstractModel::update(); }\n" \
    "    bool save()   override { return TAbstractModel::save(); }\n"   \
    "    bool remove() override { return TAbstractModel::remove(); }\n" \
    "\n"                                                 \
    "    static %2 create(%4);\n"                        \
    "    static %2 create(const QVariantMap &values);\n" \
    "    static %2 get(%5);\n"                           \
    "%6"                                                 \
    "    static int count();\n"                          \
    "    static QList<%2> getAll();\n"                   \
    "\n"                                                 \
    "private:\n"                                         \
    "    QSharedDataPointer<%2Object> d;\n"              \
    "\n"                                                 \
    "    TModelObject *modelData() override;\n"          \
    "    const TModelObject *modelData() const override;\n" \
    "    friend QDataStream &operator<<(QDataStream &ds, const %2 &model);\n" \
    "    friend QDataStream &operator>>(QDataStream &ds, %2 &model);\n" \
    "};\n"                                               \
    "\n"                                                 \
    "Q_DECLARE_METATYPE(%2)\n"                           \
    "Q_DECLARE_METATYPE(QList<%2>)\n"                    \
    "\n"                                                 \
    "#endif // %1_H\n"

#define MODEL_IMPL_TEMPLATE                                   \
    "#include <TreeFrogModel>\n"                              \
    "#include \"%1.h\"\n"                                     \
    "#include \"%1object.h\"\n"                               \
    "%12"                                                      \
    "\n"                                                      \
    "%2::%2()\n"                                              \
    "    : TAbstractModel(), d(new %2Object())\n"             \
    "{%3}\n"                                                  \
    "\n"                                                      \
    "%2::%2(const %2 &other)\n"                               \
    "    : TAbstractModel(), d(new %2Object(*other.d))\n"     \
    "{ }\n"                                                   \
    "\n"                                                      \
    "%2::%2(const %2Object &object)\n"                        \
    "    : TAbstractModel(), d(new %2Object(object))\n"       \
    "{ }\n"                                                   \
    "\n"                                                      \
    "%2::~%2()\n"                                             \
    "{\n"                                                     \
    "    // If the reference count becomes 0,\n"              \
    "    // the shared data object '%2Object' is deleted.\n"  \
    "}\n"                                                     \
    "\n"                                                      \
    "%4"                                                      \
    "%2 &%2::operator=(const %2 &other)\n"                    \
    "{\n"                                                     \
    "    d = other.d;  // increments the reference count of the data\n" \
    "    return *this;\n"                                     \
    "}\n\n"                                                   \
    "%2 %2::create(%5)\n"                                     \
    "{\n"                                                     \
    "%6"                                                      \
    "}\n"                                                     \
    "\n"                                                      \
    "%2 %2::create(const QVariantMap &values)\n"              \
    "{\n"                                                     \
    "    %2 model;\n"                                         \
    "    model.setProperties(values);\n"                      \
    "    if (!model.d->create()) {\n"                         \
    "        model.d->clear();\n"                             \
    "    }\n"                                                 \
    "    return model;\n"                                     \
    "}\n"                                                     \
    "\n"                                                      \
    "%2 %2::get(%7)\n"                                        \
    "{\n"                                                     \
    "%8"                                                      \
    "}\n"                                                     \
    "\n"                                                      \
    "%9"                                                     \
    "int %2::count()\n"                                       \
    "{\n"                                                     \
    "    %11<%2Object> mapper;\n"                             \
    "    return mapper.findCount();\n"                        \
    "}\n"                                                     \
    "\n"                                                      \
    "QList<%2> %2::getAll()\n"                                \
    "{\n"                                                     \
    "    return tfGetModelListBy%10Criteria<%2, %2Object>(TCriteria());\n" \
    "}\n"                                                     \
    "\n"                                                      \
    "TModelObject *%2::modelData()\n"                         \
    "{\n"                                                     \
    "    return d.data();\n"                                  \
    "}\n"                                                     \
    "\n"                                                      \
    "const TModelObject *%2::modelData() const\n"             \
    "{\n"                                                     \
    "    return d.data();\n"                                  \
    "}\n"                                                     \
    "\n"                                                      \
    "QDataStream &operator<<(QDataStream &ds, const %2 &model)\n" \
    "{\n"                                                     \
    "    auto varmap = model.toVariantMap();\n"               \
    "    ds << varmap;\n"                                     \
    "    return ds;\n"                                        \
    "}\n"                                                     \
    "\n"                                                      \
    "QDataStream &operator>>(QDataStream &ds, %2 &model)\n"   \
    "{\n"                                                     \
    "    QVariantMap varmap;\n"                               \
    "    ds >> varmap;\n"                                     \
    "    model.setProperties(varmap);\n"                      \
    "    return ds;\n"                                        \
    "}\n"                                                     \
    "\n"                                                      \
    "// Don't remove below this line\n"                       \
    "T_REGISTER_STREAM_OPERATORS(%2)\n"


#define USER_MODEL_HEADER_FILE_TEMPLATE                  \
    "#ifndef %1_H\n"                                     \
    "#define %1_H\n"                                     \
    "\n"                                                 \
    "#include <QStringList>\n"                           \
    "#include <QDateTime>\n"                             \
    "#include <QVariant>\n"                              \
    "#include <QSharedDataPointer>\n"                    \
    "#include <TGlobal>\n"                               \
    "#include <TAbstractUser>\n"                         \
    "#include <TAbstractModel>\n"                        \
    "\n"                                                 \
    "class TModelObject;\n"                              \
    "class %2Object;\n"                                  \
    "%7"                                                \
    "\n\n"                                               \
    "class T_MODEL_EXPORT %2 : public TAbstractUser, public TAbstractModel\n" \
    "{\n"                                                \
    "public:\n"                                          \
    "    %2();\n"                                        \
    "    %2(const %2 &other);\n"                         \
    "    %2(const %2Object &object);\n"                  \
    "    ~%2();\n"                                       \
    "\n"                                                 \
    "%3"                                                 \
    "%10"                                                 \
    "    %2 &operator=(const %2 &other);\n"              \
    "\n"                                                 \
    "    bool create() { return TAbstractModel::create(); }\n" \
    "    bool update() { return TAbstractModel::update(); }\n" \
    "    bool save()   { return TAbstractModel::save(); }\n"   \
    "    bool remove() { return TAbstractModel::remove(); }\n" \
    "\n"                                                 \
    "    static %2 authenticate(const QString &%8, const QString &%9);\n" \
    "    static %2 create(%4);\n"                        \
    "    static %2 create(const QVariantMap &values);\n" \
    "    static %2 get(%5);\n"                           \
    "    static int count();\n"                          \
    "%6"                                                 \
    "    static QList<%2> getAll();\n"                   \
    "\n"                                                 \
    "private:\n"                                         \
    "    QSharedDataPointer<%2Object> d;\n"              \
    "\n"                                                 \
    "    TModelObject *modelData();\n"                   \
    "    const TModelObject *modelData() const;\n"       \
    "    friend QDataStream &operator<<(QDataStream &ds, const %2 &model);\n" \
    "    friend QDataStream &operator>>(QDataStream &ds, %2 &model);\n" \
    "};\n"                                               \
    "\n"                                                 \
    "Q_DECLARE_METATYPE(%2)\n"                           \
    "Q_DECLARE_METATYPE(QList<%2>)\n"                    \
    "\n"                                                 \
    "#endif // %1_H\n"

#define USER_MODEL_IMPL_TEMPLATE                              \
    "#include <TreeFrogModel>\n"                              \
    "#include \"%1.h\"\n"                                     \
    "#include \"%1object.h\"\n"                               \
    "%12"                                                     \
    "\n"                                                      \
    "%2::%2()\n"                                              \
    "    : TAbstractUser(), TAbstractModel(), d(new %2Object())\n" \
    "{%3}\n"                                                  \
    "\n"                                                      \
    "%2::%2(const %2 &other)\n"                               \
    "    : TAbstractUser(), TAbstractModel(), d(new %2Object(*other.d))\n" \
    "{ }\n"                                                   \
    "\n"                                                      \
    "%2::%2(const %2Object &object)\n"                        \
    "    : TAbstractUser(), TAbstractModel(), d(new %2Object(object))\n" \
    "{ }\n"                                                   \
    "\n"                                                      \
    "\n"                                                      \
    "%2::~%2()\n"                                             \
    "{\n"                                                     \
    "    // If the reference count becomes 0,\n"              \
    "    // the shared data object '%2Object' is deleted.\n"  \
    "}\n"                                                     \
    "\n"                                                      \
    "%4"                                                      \
    "%2 &%2::operator=(const %2 &other)\n"                    \
    "{\n"                                                     \
    "    d = other.d;  // increments the reference count of the data\n" \
    "    return *this;\n"                                     \
    "}\n"                                                     \
    "\n"                                                      \
    "%2 %2::authenticate(const QString &%13, const QString &%14)\n" \
    "{\n"                                                     \
    "    if (%13.isEmpty() || %14.isEmpty())\n"               \
    "        return %2();\n"                                  \
    "\n"                                                      \
    "    %11<%2Object> mapper;\n"                             \
    "    %2Object obj = mapper.findFirst(TCriteria(%2Object::%15, %13));\n" \
    "    if (obj.isNull() || obj.%16 != %14) {\n"             \
    "        obj.clear();\n"                                  \
    "    }\n"                                                 \
    "    return %2(obj);\n"                                   \
    "}\n"                                                     \
    "\n"                                                      \
    "%2 %2::create(%5)\n"                                     \
    "{\n"                                                     \
    "%6"                                                      \
    "}\n"                                                     \
    "\n"                                                      \
    "%2 %2::create(const QVariantMap &values)\n"              \
    "{\n"                                                     \
    "    %2 model;\n"                                         \
    "    model.setProperties(values);\n"                      \
    "    if (!model.d->create()) {\n"                         \
    "        model.d->clear();\n"                             \
    "    }\n"                                                 \
    "    return model;\n"                                     \
    "}\n"                                                     \
    "\n"                                                      \
    "%2 %2::get(%7)\n"                                        \
    "{\n"                                                     \
    "%8"                                                      \
    "}\n"                                                     \
    "\n"                                                      \
    "%9"                                                     \
    "int %2::count()\n"                                       \
    "{\n"                                                     \
    "    %11<%2Object> mapper;\n"                             \
    "    return mapper.findCount();\n"                        \
    "}\n"                                                     \
    "\n"                                                      \
    "QList<%2> %2::getAll()\n"                                \
    "{\n"                                                     \
    "    return tfGetModelListBy%10Criteria<%2, %2Object>();\n" \
    "}\n"                                                     \
    "\n"                                                      \
    "TModelObject *%2::modelData()\n"                         \
    "{\n"                                                     \
    "    return d.data();\n"                                  \
    "}\n"                                                     \
    "\n"                                                      \
    "const TModelObject *%2::modelData() const\n"             \
    "{\n"                                                     \
    "    return d.data();\n"                                  \
    "}\n"                                                     \
    "\n"                                                      \
    "QDataStream &operator<<(QDataStream &ds, const %2 &model)\n" \
    "{\n"                                                     \
    "    auto varmap = model.toVariantMap();\n"               \
    "    ds << varmap;\n"                                     \
    "    return ds;\n"                                        \
    "}\n"                                                     \
    "\n"                                                      \
    "QDataStream &operator>>(QDataStream &ds, %2 &model)\n"   \
    "{\n"                                                     \
    "    QVariantMap varmap;\n"                               \
    "    ds >> varmap;\n"                                     \
    "    model.setProperties(varmap);\n"                      \
    "    return ds;\n"                                        \
    "}\n"                                                     \
    "\n"                                                      \
    "// Don't remove below this line\n"                       \
    "T_REGISTER_STREAM_OPERATORS(%2)"


static const QStringList excludedSetter = {
    "created_at",
    "updated_at",
    "modified_at",
    "lock_revision",
    "createdAt",
    "updatedAt",
    "modifiedAt",
    LOCK_REVISION_FIELD,
};


ModelGenerator::ModelGenerator(ModelGenerator::ObjectType type, const QString &model,
                               const QString &table, const QStringList &userModelFields)
    : objectType(type), modelName(), tableName(table), userFields(userModelFields)
{
    modelName = (!model.isEmpty()) ? fieldNameToEnumName(model) : fieldNameToEnumName(table);

    switch (type) {
        case Sql:
            objGen = new SqlObjGenerator(model, table);
            break;

        case Mongo:
            objGen = new MongoObjGenerator(model);
            break;
    }
}


ModelGenerator::~ModelGenerator()
{
    delete objGen;
}


bool ModelGenerator::generate(const QString &dstDir, bool userModel)
{
    QStringList files;

    // Generates model object class
    QString obj = objGen->generate(dstDir);

    if (obj.isEmpty()) {
        return false;
    }

    files << obj;

    // Generates user-model
    if (userModel) {
        if (userFields.count() == 2) {
            files << genUserModel(dstDir, userFields.value(0), userFields.value(1));
        } else if (userFields.isEmpty()) {
            files << genUserModel(dstDir);
        } else {
            qCritical("invalid parameters");
            return false;
        }
    } else {
        files << genModel(dstDir);
    }

    // Generates a project file
    ProjectFileGenerator progen(QDir(dstDir).filePath("models.pro"));
    bool ret = progen.add(files);

#ifdef Q_OS_WIN

    if (ret) {
        // Deletes dummy models
        QStringList dummy = { "_dummymodel.h", "_dummymodel.cpp" };
        bool rmd = false;

        for (auto &f : dummy) {
            rmd |= ::remove(QDir(dstDir).filePath(f));
        }

        if (rmd) {
            progen.remove(dummy);
        }
    }

#endif

    return ret;
}


QStringList ModelGenerator::genModel(const QString &dstDir)
{
    QStringList ret;
    QDir dir(dstDir);
    QPair<QStringList, QStringList> p = createModelParams();

    QString fileName = dir.filePath(modelName.toLower() + ".h");
    gen(fileName, MODEL_HEADER_FILE_TEMPLATE, p.first);
    ret << QFileInfo(fileName).fileName();

    fileName = dir.filePath(modelName.toLower() + ".cpp");
    gen(fileName, MODEL_IMPL_TEMPLATE, p.second);
    ret << QFileInfo(fileName).fileName();
    return ret;
}


QStringList ModelGenerator::genUserModel(const QString &dstDir, const QString &usernameField,
        const QString &passwordField)
{
    QStringList ret;
    QDir dir(dstDir);
    QPair<QStringList, QStringList> p = createModelParams();
    QString fileName = dir.filePath(modelName.toLower() + ".h");
    QString userVar = fieldNameToVariableName(usernameField);
    p.first << userVar << fieldNameToVariableName(passwordField);
    p.first << QLatin1String("    QString ") + USER_VIRTUAL_METHOD + "() const { return " + userVar +
            "(); }\n";

    gen(fileName, USER_MODEL_HEADER_FILE_TEMPLATE, p.first);
    ret << QFileInfo(fileName).fileName();

    fileName = dir.filePath(modelName.toLower() + ".cpp");
    p.second << fieldNameToVariableName(usernameField) << fieldNameToVariableName(passwordField)
             << fieldNameToEnumName(usernameField) << passwordField;
    gen(fileName, USER_MODEL_IMPL_TEMPLATE, p.second);
    ret << QFileInfo(fileName).fileName();
    return ret;
}


QPair<QStringList, QStringList> ModelGenerator::createModelParams()
{
    QString setgetDecl, setgetImpl, crtparams, getOptDecl, getOptImpl, initParams;
    QStringList writableFieldList;
    bool optlockMethod = false;

    QStringList fieldList = objGen->fieldList();
    QStringList fieldTypeList = objGen->fieldTypeList();
    QList<int> primaryKeyIndexList = objGen->primaryKeyIndexList();
    int autoIndex = objGen->autoValueIndex();
    QString autoFieldName = (autoIndex >= 0) ? fieldList[autoIndex] : QString();
    QStringList refTableList = objGen->refTableList();
    QList<QStringList> refFieldList = objGen->refTableFieldList();
    QString mapperstr = (objectType == Sql) ? "TSqlORMapper" : "TMongoODMapper";

    for (int i = 0; i < fieldList.length(); ++i) {
        QString field = fieldList[i];
        QString type = fieldTypeList[i];
        QString var = fieldNameToVariableName(field);
        QString enu = fieldNameToEnumName(field);

        if (type.isEmpty()) {
            continue;
        }

        // Getter method
        setgetDecl += QString("    %1 %2() const;\n").arg(type, var);
        setgetImpl += QString("%1 %2::%3() const\n{\n    return d->%4;\n}\n\n").arg(type, modelName, var,
                      field);

        if (!excludedSetter.contains(field, Qt::CaseInsensitive) && field != autoFieldName) {
            // Setter method
            setgetDecl += QString("    void set%1(%2);\n").arg(enu, createParam(type, field));
            setgetImpl += QString("void %1::set%2(%3)\n{\n    d->%4 = %5;\n}\n\n").arg(modelName, enu,
                          createParam(type, field), field, var);

            // Appends to crtparams-string
            crtparams += createParam(type, field);
            crtparams += ", ";

            writableFieldList << field;
        }

        // Initial value in the default constructor
        switch (QVariant::nameToType(type.toLatin1().data())) {
            case QVariant::Int:
            case QVariant::UInt:
            case QVariant::LongLong:
            case QVariant::ULongLong:
            case QVariant::Double:
                initParams += QString("\n    d->") + field + " = 0;";
                break;

            default:
                ;
        }

        if (var == LOCK_REVISION_FIELD) {
            optlockMethod = true;
        }
    }

    crtparams.chop(2);

    if (crtparams.isEmpty()) {
        crtparams += "const QString &";
    }

    initParams += (initParams.isEmpty()) ? ' ' : '\n';

    // Creates parameters of get() method
    QString getparams;

    if (primaryKeyIndexList.isEmpty()) {
        getparams = crtparams;
    } else {
        for (int pkidx : primaryKeyIndexList) {
            getparams += createParam(fieldTypeList[pkidx], fieldList[pkidx]);
            getparams += ", ";
        }

        getparams.chop(2);
    }

    // Creates a declaration and a implementation of 'get' method for optimistic lock
    if (!primaryKeyIndexList.isEmpty() && optlockMethod) {
        getOptDecl = QString("    static %1 get(%2, int lockRevision);\n").arg(modelName, getparams);
        QString criadd;

        for (int pkidx : primaryKeyIndexList) {
            criadd += QString("    cri.add(%1Object::%2, %3);\n").arg(modelName,
                      fieldNameToEnumName(fieldList[pkidx]), fieldNameToVariableName(fieldList[pkidx]));
        }

        getOptImpl = QString("%1 %1::get(%2, int lockRevision)\n"       \
                             "{\n"                                      \
                             "    %3<%1Object> mapper;\n"               \
                             "    TCriteria cri;\n"                     \
                             "%4"                                       \
                             "    cri.add(%1Object::LockRevision, lockRevision);\n" \
                             "    return %1(mapper.findFirst(cri));\n"  \
                             "}\n\n").arg(modelName, getparams, mapperstr, criadd);
    }

    ///////////////////////////////////////////////////////////////////////////////
    QString includeTable, classTable;
    QStringList includeTableList;
    includeTableList << fieldNameToEnumName(tableName);

    for (int i = 0; i < refTableList.length(); ++i) {
        QString enu = fieldNameToEnumName(refTableList[i]);
        QString var = fieldNameToVariableName(refTableList[i]);

        if (includeTableList.contains(enu)) {
            continue;
        } else {
            includeTable += QString("#include \"%1.h\"\n").arg(enu.toLower());
            classTable += QString("class %1;\n").arg(enu);

            includeTableList << enu;

            QString refFieldPara, refFieldName;

            for (QString field : refFieldList[i]) {
                refFieldName += fieldNameToVariableName(field);
                refFieldPara += "d->";
                refFieldPara += field;
                refFieldPara += ", ";
            }

            refFieldPara.chop(2);

            setgetDecl += QString("    %1 %2By%3() const;\n").arg(enu, var, refFieldName);
            setgetImpl += QString("%1 %2::%3By%4() const\n{\n return %1::get(%5);\n}\n\n").arg(enu,
                          modelName, var, refFieldName, refFieldPara);
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    QStringList headerArgs;
    headerArgs << modelName.toUpper() << modelName << setgetDecl << crtparams << getparams <<
               getOptDecl << classTable;

    // Creates a model implementation
    QString createImpl;
    createImpl += QString("    %1Object obj;\n").arg(modelName);

    for (QString field : writableFieldList) {
        createImpl += QString("    obj.%1 = %2;\n").arg(field, fieldNameToVariableName(field));
    }

    createImpl += "    if (!obj.create()) {\n";
    createImpl += QString("        return %1();\n").arg(modelName);
    createImpl += "    }\n";
    createImpl += QString("    return %1(obj);\n").arg(modelName);

    // Creates a implementation of get() method
    QString getImpl;

    if (primaryKeyIndexList.isEmpty()) {
        // If no primary index exists
        getImpl += QString("    TCriteria cri;\n");

        for (QString field : writableFieldList) {
            getImpl += QString("    cri.add(%1Object::%2, %3);\n").arg(modelName, fieldNameToEnumName(field),
                       fieldNameToVariableName(field));
        }
    }

    getImpl += QString("    %1<%2Object> mapper;\n").arg(mapperstr, modelName);
    getImpl += QString("    return %1(mapper.").arg(modelName);

    if (primaryKeyIndexList.isEmpty()) {
        getImpl += "findFirst(cri));\n";
    } else {
        getImpl += (objectType == Sql) ? "findByPrimaryKey(" : "findByObjectId(";

        if (primaryKeyIndexList.length() == 1) {
            getImpl += fieldNameToVariableName(fieldList.at(primaryKeyIndexList.at(0)));
        } else {
            getImpl += "QVariantList()";

            for (int pkidx : primaryKeyIndexList) {
                getImpl += "<<QVariant(";
                getImpl += fieldNameToVariableName(fieldList.at(pkidx));
                getImpl += ")";
            }
        }

        getImpl += QString("));\n");
    }

    QStringList implArgs;
    implArgs << modelName.toLower() << modelName << initParams << setgetImpl << crtparams << createImpl
             << getparams << getImpl << getOptImpl;
    implArgs << ((objectType == Mongo) ? "Mongo" : "") << mapperstr;
    implArgs << includeTable;
    return QPair<QStringList, QStringList>(headerArgs, implArgs);
}


bool ModelGenerator::gen(const QString &fileName, const QString &format, const QStringList &args)
{
    QString out = format;

    for (auto &arg : args) {
        out = out.arg(arg);
    }

    FileWriter fw(fileName);
    fw.write(out, false);
    return true;
}


QString ModelGenerator::createParam(const QString &type, const QString &name)
{
    QString string;
    QString var = fieldNameToVariableName(name);
    QVariant::Type vtype = QVariant::nameToType(type.toLatin1().data());

    if (vtype == QVariant::Int || vtype == QVariant::UInt || vtype == QVariant::ULongLong ||
            vtype == QVariant::Double) {
        string += type;
        string += ' ';
        string += var;
    } else {
        string += QString("const %1 &%2").arg(type, var);
    }

    return string;
}


QStringList ModelGenerator::fieldList() const
{
    return objGen->fieldList();
}


QStringList ModelGenerator::fieldTypeList() const
{
    return objGen->fieldTypeList();
}


QList<int> ModelGenerator::primaryKeyIndexList() const
{
    return objGen->primaryKeyIndexList();
}

QStringList ModelGenerator::refTableList() const
{
    return objGen->refTableList();
}

QList<QStringList> ModelGenerator::refTableFieldList() const
{
    return objGen->refTableFieldList();
}

QStringList ModelGenerator::reffedTableList() const
{
    return objGen->reffedTableList();
}

QList<QStringList> ModelGenerator::reffedTableFieldList() const
{
    return objGen->reffedTableFieldList();
}

int ModelGenerator::autoValueIndex() const
{
    return objGen->autoValueIndex();
}


int ModelGenerator::lockRevisionIndex() const
{
    return objGen->lockRevisionIndex();
}
