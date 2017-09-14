#ifndef ERBGENERATOR_H
#define ERBGENERATOR_H

#include <QStringList>
#include <QDir>
#include <QPair>
#include <QVariant>
#include <modelgenerator.h>

class ErbGenerator
{
public:
    ErbGenerator(const ModelGenerator &modelGen);
    bool generate(const QString &dstDir) const;

private:
    QString viewName;
    QStringList fieldList;
    QStringList fieldTypeList;
    QList<int> primaryKeyIndexList;
    int autoValueIndex;
};

#endif // ERBGENERATOR_H
