#ifndef CONTROLLERGENERATOR_H
#define CONTROLLERGENERATOR_H

#include <QString>
#include <QDir>
#include <modelgenerator.h>

class ControllerGenerator
{
public:
    ControllerGenerator(const ModelGenerator &modelGen);
    ControllerGenerator(const QString &controller, const QStringList &actions);
    ~ControllerGenerator() { }
    bool generate(const QString &dstDir) const;

private:
    QString controllerName;
    QString tableName;
    QStringList actionList;
    QStringList fieldList;
    QStringList fieldTypeList;
    QList<int> primaryKeyIndexList;
    int lockRevisionIndex;
};

#endif // CONTROLLERGENERATOR_H
