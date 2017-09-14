#ifndef MODELGENERATOR_H
#define MODELGENERATOR_H

#include <QStringList>
#include <QDir>
#include <QPair>
#include <QVariant>

class AbstractObjGenerator;


class ModelGenerator
{
public:

    enum ObjectType {
        Sql,
        Mongo,
    };

    ModelGenerator(ObjectType type, const QString &model, const QString &table = QString(),
                   const QStringList &userModelFields = QStringList());
    ~ModelGenerator();
    bool generate(const QString &dst, bool userModel = false);
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

protected:
    QStringList genModel(const QString &dstDir);
    QStringList genUserModel(const QString &dstDir, const QString &usernameField = "username",
                             const QString &passwordField = "password");
    QPair<QStringList, QStringList> createModelParams();

    static bool gen(const QString &fileName, const QString &format, const QStringList &args);
    static QString createParam(const QString &type, const QString &name);

private:
    ObjectType objectType;
    QString modelName;
    QString tableName;
    AbstractObjGenerator *objGen;
    QStringList userFields;
};

#endif // MODELGENERATOR_H
