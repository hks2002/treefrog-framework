/* Copyright (c) 2010-2017, AOYAMA Kazuharu
 * All rights reserved.
 *
 * This software may be used and distributed according to the terms of
 * the New BSD License, which is incorporated herein by reference.
 */

#include "otamagenerator.h"
#include "global.h"
#include "projectfilegenerator.h"
#include "filewriter.h"
#include "util.h"


#define INDEX_HTML_TEMPLATE                                             \
    "<!DOCTYPE html>\n"                                                 \
    "<html>\n"                                                          \
    "<head>\n"                                                          \
    "  <meta charset=UTF-8\" />\n"                                      \
    "  <title data-tf=\"@head_title\"></title>\n"                       \
    "</head>\n"                                                         \
    "<body>\n"                                                          \
    "\n"                                                                \
    "<h1>Listing %1</h1>\n"                                             \
    "\n"                                                                \
    "<a href=\"#\" data-tf=\"@link_to_entry\">Create a new %1</a><br />\n" \
    "<br />\n"                                                          \
    "<table border=\"1\" cellpadding=\"5\" style=\"border: 1px #d0d0d0 solid; border-collapse: collapse;\">\n" \
    "  <tr>\n"                                                          \
    "%2"                                                                \
    "    <th></th>\n"                                                   \
    "  </tr>\n"                                                         \
    "  <tr data-tf=\"@for\">\n"                                         \
    "%3"                                                                \
    "    <td>\n"                                                        \
    "      <a href=\"#\" data-tf=\"@link_to_show\">Show</a>\n"          \
    "      <a href=\"#\" data-tf=\"@link_to_edit\">Edit</a>\n"          \
    "      <a href=\"#\" data-tf=\"@link_to_remove\">Remove</a>\n"      \
    "    </td>\n"                                                       \
    "  </tr>\n"                                                         \
    "</table>\n"                                                        \
    "\n"                                                                \
    "</body>\n"                                                         \
    "</html>\n"

#define INDEX_OTM_TEMPLATE                                              \
    "#include \"%1.h\"\n"                                               \
    "\n"                                                                \
    "@head_title ~= controller()->name() + \": \" + controller()->activeAction()\n" \
    "\n"                                                                \
    "@for :\n"                                                          \
    "tfetch(QList<%2>, %3List);\n"                                      \
    "for (const auto &i : %3List) {\n"                                  \
    "    %%\n"                                                          \
    "}\n"                                                               \
    "\n"                                                                \
    "%4"                                                                \
    "@link_to_entry :== linkTo(\"Create a new %6\", urla(\"create\"))\n" \
    "\n"                                                                \
    "@link_to_show :== linkTo(\"Show\", urla(\"show\", %5))\n"          \
    "\n"                                                                \
    "@link_to_edit :== linkTo(\"Edit\", urla(\"save\", %5))\n"      \
    "\n"                                                                \
    "@link_to_remove :== linkTo(\"Remove\", urla(\"remove\", %5), Tf::Post, \"confirm('Are you sure?')\")\n"

#define SHOW_HTML_TEMPLATE                                              \
    "<!DOCTYPE html>\n"                                                 \
    "<html>\n"                                                          \
    "<head>\n"                                                          \
    "  <meta charset=UTF-8\" />\n"                                      \
    "  <title data-tf=\"@head_title\"></title>\n"                       \
    "</head>\n"                                                         \
    "<body>\n"                                                          \
    "<p style=\"color: red\" data-tf=\"@error_msg\"></p>\n"             \
    "<p style=\"color: green\" data-tf=\"@notice_msg\"></p>\n"          \
    "\n"                                                                \
    "<h1>Showing %1</h1>\n"                                             \
    "%2"                                                                \
    "\n"                                                                \
    "<a href=\"#\" data-tf=\"@link_to_edit\">Edit</a> |\n"              \
    "<a href=\"#\" data-tf=\"@link_to_index\">Back</a>\n"               \
    "\n"                                                                \
    "</body>\n"                                                         \
    "</html>\n"

#define SHOW_OTM_TEMPLATE                                               \
    "#include \"%1.h\"\n"                                               \
    "\n"                                                                \
    "#init\n"                                                           \
    " tfetch(%2, %3);\n"                                                \
    "\n"                                                                \
    "@head_title ~= controller()->name() + \": \" + controller()->activeAction()\n" \
    "\n"                                                                \
    "@error_msg ~=$ error\n"                                            \
    "\n"                                                                \
    "@notice_msg ~=$ notice\n"                                          \
    "\n"                                                                \
    "%4"                                                                \
    "@link_to_edit :== linkTo(\"Edit\", urla(\"save\", %5))\n"     \
    "\n"                                                                \
    "@link_to_index :== linkTo(\"Back\", urla(\"index\"))\n"

#define CREATE_HTML_TEMPLATE                                            \
    "<!DOCTYPE html>\n"                                                 \
    "<html>\n"                                                          \
    "<head>\n"                                                          \
    "  <meta charset=UTF-8\" />\n"                                      \
    "  <title data-tf=\"@head_title\"></title>\n"                       \
    "</head>\n"                                                         \
    "<body>\n"                                                          \
    "<p style=\"color: red\" data-tf=\"@error_msg\"></p>\n"             \
    "<p style=\"color: green\" data-tf=\"@notice_msg\"></p>\n"          \
    "\n"                                                                \
    "<h1>New %1</h1>\n"                                                 \
    "\n"                                                                \
    "<form method=\"post\" data-tf=\"@entry_form\">\n"                  \
    "%2"                                                                \
    "  <p>\n"                                                           \
    "    <input type=\"submit\" value=\"Create\" />\n"                  \
    "  </p>\n"                                                          \
    "</form>\n"                                                         \
    "\n"                                                                \
    "<a href=\"#\" data-tf=\"@link_to_index\">Back</a>\n"               \
    "\n"                                                                \
    "</body>\n"                                                         \
    "</html>\n"

#define CREATE_OTM_TEMPLATE                                             \
    "#include \"%1.h\"\n"                                               \
    "\n"                                                                \
    "#init\n"                                                           \
    " tfetch(QVariantMap, %2);\n"                                       \
    "\n"                                                                \
    "@head_title ~= controller()->name() + \": \" + controller()->activeAction()\n" \
    "\n"                                                                \
    "@error_msg ~=$ error\n"                                            \
    "\n"                                                                \
    "@notice_msg ~=$ notice\n"                                          \
    "\n"                                                                \
    "@entry_form |== formTag(urla(\"create\"))\n"                       \
    "\n"                                                                \
    "%3"                                                                \
    "@link_to_index |== linkTo(\"Back\", urla(\"index\"))\n"

#define SAVE_HTML_TEMPLATE                                              \
    "<!DOCTYPE html>\n"                                                 \
    "<html>\n"                                                          \
    "<head>\n"                                                          \
    "  <meta charset=UTF-8\" />\n"                                      \
    "  <title data-tf=\"@head_title\"></title>\n"                       \
    "</head>\n"                                                         \
    "<body>\n"                                                          \
    "<p style=\"color: red\" data-tf=\"@error_msg\"></p>\n"             \
    "<p style=\"color: green\" data-tf=\"@notice_msg\"></p>\n"          \
    "\n"                                                                \
    "<h1>Editing %1</h1>\n"                                             \
    "\n"                                                                \
    "<form method=\"post\" data-tf=\"@edit_form\">\n"                   \
    "%2"                                                                \
    "  <p>\n"                                                           \
    "    <input type=\"submit\" value=\"Save\" />\n"                    \
    "  </p>\n"                                                          \
    "</form>\n"                                                         \
    "\n"                                                                \
    "<a href=\"#\" data-tf=\"@link_to_show\">Show</a> |\n"              \
    "<a href=\"#\" data-tf=\"@link_to_index\">Back</a>\n"               \
    "\n"                                                                \
    "</body>\n"                                                         \
    "</html>\n"

#define SAVE_OTM_TEMPLATE                                               \
    "#include \"%1.h\"\n"                                               \
    "\n"                                                                \
    "#init\n"                                                           \
    " tfetch(QVariantMap, %2);\n"                                       \
    "\n"                                                                \
    "@head_title ~= controller()->name() + \": \" + controller()->activeAction()\n" \
    "\n"                                                                \
    "@error_msg ~=$ error\n"                                            \
    "\n"                                                                \
    "@notice_msg ~=$ notice\n"                                          \
    "\n"                                                                \
    "@edit_form |== formTag(urla(\"save\", %3))\n"                      \
    "\n"                                                                \
    "%4"                                                                \
    "@link_to_show |== linkTo(\"Show\", urla(\"show\", %3))\n"          \
    "\n"                                                                \
    "@link_to_index |== linkTo(\"Back\", urla(\"index\"))\n"


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


OtamaGenerator::OtamaGenerator(const ModelGenerator &modelGen)
    : viewName(modelGen.model()),
      fieldList(modelGen.fieldList()),
      fieldTypeList(modelGen.fieldTypeList()),
      primaryKeyIndexList(modelGen.primaryKeyIndexList()),
      autoValueIndex(modelGen.autoValueIndex())
{ }


bool OtamaGenerator::generate(const QString &dstDir) const
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

    // Generates view files
    generateViews(dir.path());
    return true;
}


QStringList OtamaGenerator::generateViews(const QString &dstDir) const
{
    QStringList files;

    if (primaryKeyIndexList.isEmpty()) {
        qWarning("Primary key not found. [view name: %s]", qPrintable(viewName));
        return files;
    }

    QDir dir(dstDir);
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
            indexUrl += QString("<<QString::number(i.get%1())").arg(enu);
            showUrl  += QString("<<QString::number(%1.get%2())").arg(modelName, enu);
        }
        else if (vtype == QVariant::String) {
            indexUrl += QString("<<i.get%1()").arg(enu);
            showUrl  += QString("<<%1.get%2()").arg(modelName, enu);
        }
        else {}

        saveUrl  += QString("<<%1[\"%2\"].toString()").arg(modelName, var);
    }

    QString th, td, indexOtm, showColumn, showOtm, entryColumn, editColumn, entryOtm, editOtm;

    for (int i = 0; i < fieldList.count(); ++i) {
        QString cap = fieldNameToCaption(fieldList[i]);
        QString enu = fieldNameToEnumName(fieldList[i]);
        QString var = fieldNameToVariableName(fieldList[i]);
        QString mrk = fieldList[i].toLower();
        QString readonly;

        if (!excludedColumn.contains(var, Qt::CaseInsensitive)) {
            th += "    <th>" + cap + "</th>\n";
            td += "    <td data-tf=\"@" + mrk + "\"></td>\n";

            indexOtm += QString("@%1 ~= i.get%2()\n\n").arg(mrk, enu);

            if (i != autoValueIndex) {  // case of not auto-value field
                entryColumn += QString("  <p>\n    <label>%1<br /><input data-tf=\"@%2\" /></label>\n  </p>\n").arg(
                                   cap, mrk);
                entryOtm += QString("@%1 |== inputTextTag(\"%2[%3]\", %2[\"%3\"].toString())\n\n").arg(mrk,
                            modelName, enu);
            }

            editColumn += QString("  <p>\n    <label>%1<br /><input data-tf=\"@%2\" /></label>\n  </p>\n").arg(
                              cap, mrk);

            if (primaryKeyIndexList.contains(i)) {
                readonly = QLatin1String(", a(\"readonly\", \"readonly\")");
            }

            editOtm += QString("@%1 |== inputTextTag(\"%2[%3]\", %2[\"%3\"].toString()%4);\n\n").arg(mrk,
                       modelName, var, readonly);
        }

        showColumn += QString("<dt>%1</dt><dd data-tf=\"@%2\">(%3)</dd><br />\n").arg(cap, mrk, var);
        showOtm += QString("@%1 ~= %2.get%3()\n\n").arg(mrk, modelName, enu);
    }

    // Generates index.html
    output = QString(INDEX_HTML_TEMPLATE).arg(caption, th, td);
    fw.setFilePath(dir.filePath("index.html"));

    if (fw.write(output, false)) {
        files << fw.fileName();
    }

    // Generates index.otm
    output = QString(INDEX_OTM_TEMPLATE).arg(modelName.toLower(), viewName, modelName, indexOtm,
             indexUrl, caption);
    fw.setFilePath(dir.filePath("index.otm"));

    if (fw.write(output, false)) {
        files << fw.fileName();
    }

    // Generates show.html
    output = QString(SHOW_HTML_TEMPLATE).arg(caption, showColumn);
    fw.setFilePath(dir.filePath("show.html"));

    if (fw.write(output, false)) {
        files << fw.fileName();
    }

    // Generates show.otm
    output = QString(SHOW_OTM_TEMPLATE).arg(modelName.toLower(), viewName, modelName, showOtm, showUrl);
    fw.setFilePath(dir.filePath("show.otm"));

    if (fw.write(output, false)) {
        files << fw.fileName();
    }

    // Generates entry.html
    output = QString(CREATE_HTML_TEMPLATE).arg(caption, entryColumn);
    fw.setFilePath(dir.filePath("create.html"));

    if (fw.write(output, false)) {
        files << fw.fileName();
    }

    // Generates entry.otm
    output = QString(CREATE_OTM_TEMPLATE).arg(modelName.toLower(), modelName, entryOtm);
    fw.setFilePath(dir.filePath("create.otm"));

    if (fw.write(output, false)) {
        files << fw.fileName();
    }

    // Generates edit.html
    output = QString(SAVE_HTML_TEMPLATE).arg(caption, editColumn);
    fw.setFilePath(dir.filePath("save.html"));

    if (fw.write(output, false)) {
        files << fw.fileName();
    }

    // Generates edit.otm
    output = QString(SAVE_OTM_TEMPLATE).arg(modelName.toLower(), modelName, saveUrl, editOtm);
    fw.setFilePath(dir.filePath("save.otm"));

    if (fw.write(output, false)) {
        files << fw.fileName();
    }

    return files;
}
