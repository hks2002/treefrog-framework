#ifndef OTAMAGENERATOR_H
#define OTAMAGENERATOR_H

#include <QStringList>
#include <QDir>
#include <QPair>
#include <QVariant>
#include <modelgenerator.h>

class OtamaGenerator
{
public:
    OtamaGenerator(const ModelGenerator &modelGen);
    bool generate(const QString &dstDir) const;

protected:
    QStringList generateViews(const QString &dstDir) const;

private:
    QString viewName;
    QStringList fieldList;
    QStringList fieldTypeList;
    QList<int> primaryKeyIndexList;
    int autoValueIndex;
};

#endif // OTAMAGENERATOR_H
