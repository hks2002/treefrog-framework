/* Copyright (c) 2012-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include <QList>
#include <QPair>
#include "erbgenerator.h"
#include "global.h"
#include "filewriter.h"
#include "util.h"

#define INDEX_TEMPLATE                                                  \
    "<!DOCTYPE html>\n"                                                 \
    "<%#include \"%1.h\" %>\n"                                          \
    "<html>\n"                                                          \
    "<head>\n"                                                          \
    "  <meta charset=UTF-8\" />\n"                                      \
    "  <title><%= controller()->name() + \": \" + controller()->activeAction() %></title>\n" \
    "</head>\n"                                                         \
    "<body>\n"                                                          \
    "\n"                                                                \
    "<h1>Listing %2</h1>\n"                                             \
    "\n"                                                                \
    "<%== linkTo(\"Create a new %2\", urla(\"create\")) %><br />\n"     \
    "<br />\n"                                                          \
    "<table border=\"1\" cellpadding=\"5\" style=\"border: 1px #d0d0d0 solid; border-collapse: collapse;\">\n" \
    "  <tr>\n"                                                          \
    "%3"                                                                \
    "  </tr>\n"                                                         \
    "<% tfetch(QList<%4>, %5List); %>\n"                                \
    "<% for (const auto &i : %5List) { %>\n"                            \
    "  <tr>\n"                                                          \
    "%6"                                                                \
    "    <td>\n"                                                        \
    "      <%== linkTo(\"Show\", urla(\"show\", %7)) %>\n"              \
    "      <%== linkTo(\"Edit\", urla(\"save\", %7)) %>\n"              \
    "      <%== linkTo(\"Remove\", urla(\"remove\", %7), Tf::Post, \"confirm('Are you sure?')\") %>\n" \
    "    </td>\n"                                                       \
    "  </tr>\n"                                                         \
    "<% } %>\n"                                                         \
    "</table>\n"                                                        \
    "\n"                                                                \
    "</body>\n"                                                         \
    "</html>\n"

#define SHOW_TEMPLATE                                                   \
    "<!DOCTYPE html>\n"                                                 \
    "<%#include \"%1.h\" %>\n"                                          \
    "<% tfetch(%2, %3); %>\n"                                           \
    "<html>\n"                                                          \
    "<head>\n"                                                          \
    "  <meta charset=UTF-8\" />\n"                                      \
    "  <title><%= controller()->name() + \": \" + controller()->activeAction() %></title>\n" \
    "</head>\n"                                                         \
    "<body>\n"                                                          \
    "<p style=\"color: red\"><%=$ error %></p>\n"                       \
    "<p style=\"color: green\"><%=$ notice %></p>\n"                    \
    "\n"                                                                \
    "<h1>Showing %4</h1>\n"                                             \
    "%5"                                                                \
    "\n"                                                                \
    "<%== linkTo(\"Edit\", urla(\"save\", %6)) %> |\n"                  \
    "<%== linkTo(\"Back\", urla(\"index\")) %>\n"                       \
    "\n"                                                                \
    "</body>\n"                                                         \
    "</html>\n"


#define CREATE_TEMPLATE                                                 \
    "<!DOCTYPE html>\n"                                                 \
    "<%#include \"%1.h\" %>\n"                                          \
    "<% tfetch(QVariantMap, %2); %>\n"                                  \
    "<html>\n"                                                          \
    "<head>\n"                                                          \
    "  <meta charset=UTF-8\" />\n"                                      \
    "  <title><%= controller()->name() + \": \" + controller()->activeAction() %></title>\n" \
    "</head>\n"                                                         \
    "<body>\n"                                                          \
    "<p style=\"color: red\"><%=$ error %></p>\n"                       \
    "<p style=\"color: green\"><%=$ notice %></p>\n"                    \
    "\n"                                                                \
    "<h1>New %3</h1>\n"                                                 \
    "\n"                                                                \
    "<%== formTag(urla(\"create\"), Tf::Post) %>\n"                     \
    "%4"                                                                \
    "  <p>\n"                                                           \
    "    <input type=\"submit\" value=\"Create\" />\n"                  \
    "  </p>\n"                                                          \
    "</form>\n"                                                         \
    "\n"                                                                \
    "<%== linkTo(\"Back\", urla(\"index\")) %>\n"                       \
    "\n"                                                                \
    "</body>\n"                                                         \
    "</html>\n"

#define SAVE_TEMPLATE                                                   \
    "<!DOCTYPE html>\n"                                                 \
    "<%#include \"%1.h\" %>\n"                                          \
    "<% tfetch(QVariantMap, %2); %>\n"                                  \
    "<html>\n"                                                          \
    "<head>\n"                                                          \
    "  <meta charset=UTF-8\" />\n"                                      \
    "  <title><%= controller()->name() + \": \" + controller()->activeAction() %></title>\n" \
    "</head>\n"                                                         \
    "<body>\n"                                                          \
    "<p style=\"color: red\"><%=$ error %></p>\n"                       \
    "<p style=\"color: green\"><%=$ notice %></p>\n"                    \
    "\n"                                                                \
    "<h1>Editing %3</h1>\n"                                             \
    "\n"                                                                \
    "<%== formTag(urla(\"save\", %4), Tf::Post) %>\n"                   \
    "%6"                                                                \
    "  <p>\n"                                                           \
    "    <input type=\"submit\" value=\"Save\" />\n"                    \
    "  </p>\n"                                                          \
    "</form>\n"                                                         \
    "\n"                                                                \
    "<%== linkTo(\"Show\", urla(\"show\", %4)) %> |\n"                  \
    "<%== linkTo(\"Back\", urla(\"index\")) %>\n"                       \
    "</body>\n"                                                         \
    "</html>\n"

static const QStringList excludedColumn = {
    "created_at",
    "updated_at",
    "modified_at",
    "lock_revision",
    "createdAt",
    "updatedAt",
    "modifiedAt",
    "lockRevision",
    "created_by",
    "updated_by",
    "modified_by",
    "createdBy",
    "updatedBy",
    "modifiedBy",
};


static const QStringList excludedDirName = {
    "layouts",
    "partial",
    "direct",
    "_src",
    "mailer",
};


ErbGenerator::ErbGenerator(const ModelGenerator &modelGen)
    : viewName(modelGen.model()),
      fieldList(modelGen.fieldList()),
      fieldTypeList(modelGen.fieldTypeList()),
      primaryKeyIndexList(modelGen.primaryKeyIndexList()),
      autoValueIndex(modelGen.autoValueIndex())
{ }


bool ErbGenerator::generate(const QString &dstDir) const
{
    QDir dir(dstDir + viewName.toLower());

    // Reserved word check
    if (excludedDirName.contains(dir.dirName())) {
        qCritical("Reserved word error. Please use another word.  View name: %s",
                  qPrintable(dir.dirName()));
        return false;
    }

    mkpath(dir);
    copy(dataDirPath + ".trim_mode", dir);

    if (primaryKeyIndexList.isEmpty()) {
        qWarning("Primary key not found. [view name: %s]", qPrintable(viewName));
        return false;
    }

    FileWriter fw;
    QString output;
    QString caption = enumNameToCaption(viewName);
    QString modelName = enumNameToVariableName(viewName);

    QString indexUrl = "QStringList()";
    QString showUrl  = "QStringList()";
    QString saveUrl  = "QStringList()";

    for (int pkidx : primaryKeyIndexList) {
        QString enu = fieldNameToEnumName(fieldList[pkidx]);
        QString var = fieldNameToVariableName(fieldList[pkidx]);
        QString type = fieldTypeList[pkidx];
        QVariant::Type vtype = QVariant::nameToType(type.toLatin1().data());

        if (vtype == QVariant::Int || vtype == QVariant::UInt || vtype == QVariant::ULongLong ||
            vtype == QVariant::Double) {
            indexUrl += QString("<<QString::number(i.%1())").arg(var);
            showUrl  += QString("<<QString::number(%1.%2())").arg(modelName, var);
        }
        else if (vtype == QVariant::String) {
            indexUrl += QString("<<i.%1()").arg(var);
            showUrl  += QString("<<%1.%2()").arg(modelName, var);
        }
        else {}

        saveUrl  += QString("<<%1[\"%2\"].toString()").arg(modelName, var);
    }

    // Generates index.html.erb
    QString th, td, showitems, entryitems, edititems;

    for (int i = 0; i < fieldList.count(); ++i) {
        QString icap = fieldNameToCaption(fieldList[i]);
        QString ienu = fieldNameToEnumName(fieldList[i]);
        QString ivar = fieldNameToVariableName(fieldList[i]);

        showitems += "<dt>" + icap + "</dt>";
        showitems += "<dd><%= " + modelName + "." + ivar + "() %></dd><br />\n";

        if (!excludedColumn.contains(ivar, Qt::CaseInsensitive)) {
            th += "    <th>" + icap + "</th>\n";
            td += "    <td><%= i." + ivar + "() %></td>\n";

            if (i != autoValueIndex) {  // case of not auto-value field
                entryitems += "  <p>\n    <label>" + icap + "<br />";
                entryitems += "<input name=\"" + modelName + '[' + ivar + ']' + "\" ";
                entryitems += "value=\"<%= " + modelName + "[\"" + ivar + "\"] %>\"";
                entryitems += " /></label>\n  </p>\n";
            }

            edititems += "  <p>\n    <label>" + icap + "<br />";
            edititems += "<input type=\"text\" name=\"" + modelName + '[' + ivar + ']' + "\" ";
            edititems += "value=\"<%= " + modelName + "[\"" + ivar + "\"]" + " %>\"";

            if (primaryKeyIndexList.contains(i)) {
                edititems += " readonly=\"readonly\"";
            }

            edititems += " /></label>\n  </p>\n";
        }
    }

    output = QString(INDEX_TEMPLATE).arg(modelName.toLower(), caption, th, viewName, modelName, td,
                                         indexUrl);
    fw.setFilePath(dir.filePath("index.erb"));

    if (!fw.write(output, false)) {
        return false;
    }

    output = QString(SHOW_TEMPLATE).arg(modelName.toLower(), viewName, modelName, caption, showitems,
                                        showUrl);
    fw.setFilePath(dir.filePath("show.erb"));

    if (!fw.write(output, false)) {
        return false;
    }

    output = QString(CREATE_TEMPLATE).arg(modelName.toLower(), modelName, caption, entryitems);
    fw.setFilePath(dir.filePath("create.erb"));

    if (!fw.write(output, false)) {
        return false;
    }

    output = QString(SAVE_TEMPLATE).arg(modelName.toLower(), modelName, caption, saveUrl, edititems);
    fw.setFilePath(dir.filePath("save.erb"));

    if (!fw.write(output, false)) {
        return false;
    }

    return true;
}
