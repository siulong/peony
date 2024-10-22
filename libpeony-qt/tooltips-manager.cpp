/*
 * Peony-Qt
 *
 * Copyright (C) 2024, Tianjin KYLIN Information Technology Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * @author: Renyg <renyangguang@kylinos.cn>
 * @date:   2024-09-23 14:55
 */

#include "tooltips-manager.h"
#include "file-info.h"
#include "file-utils.h"
#include "global-settings.h"

#include <QUrl>
#include <QSet>
#include <QImageReader>
#include <QVariant>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

namespace Peony {

// HTML templates for tooltips
static const QString CONTENT_TEMPLATE_WITH_FULLNAME = R"(
<html>
    <head>
        <style>
        table { width: 200px; border-collapse: collapse; }
        td { word-wrap: break-word; max-width: 0; }
        </style>
    </head>
    <body>
        <table>
        <tr><td>%1</td></tr>
        <tr><td>%2%3</td></tr>
        <tr><td>%4%5</td></tr>
        <tr><td>%6%7</td></tr>
        </table>
    </body>
</html>
)";

static const QString CONTENT_TEMPLATE_WITHOUT_FULLNAME = R"(
<html>
    <head>
        <style>
        table { width: 200px; border-collapse: collapse; }
        td { word-wrap: break-all; max-width: 0; }
        </style>
    </head>
    <body>
        <table>
        <tr><td>%1%2</td></tr>
        <tr><td>%3%4</td></tr>
        <tr><td>%5%6</td></tr>
        </table>
    </body>
</html>
)";

TooltipsManager& TooltipsManager::instance()
{
    static TooltipsManager instance;
    return instance;
}

QString TooltipsManager::generateTooltip(FileInfo* info) const
{
    if (!info) {
        return QString();
    }

    if (isSystemFile(info)) {
        return info->displayName();
    }

    if (info->fileType() == tr("Unknow")) {
        return toNormalFile(info);
    }

    if (info->isImageFile()) {
        return toImage(info);
    }
    else if (info->isVideoFile() || info->isAudioFile()) {
        return toVideoAudio(info);
    }
    else {
        return toNormalFile(info);
    }
}

QString TooltipsManager::getMediaDuration(const QString& filePath, int timeoutMs)
{
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        qWarning() << "File does not exist:" << filePath;
        return "N/A";
    }

    QProcess process;
    QStringList arguments;
    arguments << "-v" << "error"
              << "-show_entries" << "format=duration"
              << "-of" << "default=noprint_wrappers=1:nokey=1"
              << "-sexagesimal"
              << filePath;

    process.start("ffprobe", arguments);

    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        qWarning() << "FFprobe process timed out for file:" << filePath;
        return "N/A";
    }

    if (process.exitCode() != 0) {
        qWarning() << "FFprobe process failed:" << process.errorString();
        return "N/A";
    }

    QString output = process.readAllStandardOutput().trimmed();
    QStringList parts = output.split(".");

    if (parts.isEmpty()) {
        return "N/A";
    }

    // Get the main time part (excluding milliseconds)
    QString mainPart = parts[0];

    // Ensure format is HH:MM:SS
    QStringList timeParts = mainPart.split(":");
    while (timeParts.size() < 3) {
        timeParts.prepend("00");
    }

    // Combine time string, ensuring each part is two digits
    return QString("%1:%2:%3")
        .arg(timeParts[0].rightJustified(2, '0'))
        .arg(timeParts[1].rightJustified(2, '0'))
        .arg(timeParts[2].rightJustified(2, '0'));
}

bool TooltipsManager::isSystemFile(FileInfo* info)
{
    static const QString homeUri = "file://" + QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    static const QSet<QString> systemUris = {
        "trash:///", "computer:///", "network:///", "recent:///", homeUri
    };

    bool isSystemFile = systemUris.contains(info->uri());
    if (!isSystemFile && !info->targetUri().isEmpty()) {
        isSystemFile = systemUris.contains(info->targetUri());
    }
    return isSystemFile;
}

QString TooltipsManager::toVideoAudio(FileInfo* info) const
{
    QString typeLbl = tr("Type: ");
    QString fileType = info->fileType();

    QUrl url = info->uri();
    QString duration = getMediaDuration(url.path());
    QString durationLbl = tr("Duration：");

    QString sizeLbl = tr("Size: ");
    bool isElided = info->property("isElided").toBool();

    if (isElided) {
        QString displayName = getDisplayName(info).toHtmlEscaped();
        return QString(CONTENT_TEMPLATE_WITH_FULLNAME).arg(
           displayName,
           typeLbl, fileType,
           durationLbl, duration,
           sizeLbl, info->fileSize()
        );
    } else {
        return QString(CONTENT_TEMPLATE_WITHOUT_FULLNAME).arg(
           typeLbl, fileType,
           durationLbl, duration,
           sizeLbl, info->fileSize()
        );
    }
}

QString TooltipsManager::toImage(FileInfo* info) const
{
    QString typeLbl = tr("Type: ");
    QString fileType = info->fileType();

    QUrl url = info->uri();
    QImageReader r(url.path());
    QString resolution = QString("%1x%2").arg(r.size().width()).arg(r.size().height());
    QString resolutionLbl = tr("Res: ");

    QString sizeLbl = tr("Size: ");
    bool isElided = info->property("isElided").toBool();

    if (isElided) {
        QString displayName = getDisplayName(info).toHtmlEscaped();
        return QString(CONTENT_TEMPLATE_WITH_FULLNAME).arg(
           displayName,
           typeLbl, fileType,
           resolutionLbl, resolution,
           sizeLbl, info->fileSize()
        );
    } else {
        return QString(CONTENT_TEMPLATE_WITHOUT_FULLNAME).arg(
           typeLbl, fileType,
           resolutionLbl, resolution,
           sizeLbl, info->fileSize()
        );
    }
}

QString TooltipsManager::toNormalFile(FileInfo* info) const
{
    QString typeLbl = tr("Type: ");
    QString typeUnKnow = tr("Unknow");
    QString fileType = info->fileType().isEmpty() ? typeUnKnow : info->fileType();

    QString modDateLbl = tr("Mod Date: ");
    QString sizeLbl = tr("Size: ");
    bool isElided = info->property("isElided").toBool();

    if (isElided) {
        QString displayName = getDisplayName(info).toHtmlEscaped();
        return QString(CONTENT_TEMPLATE_WITH_FULLNAME).arg(
           displayName,
           typeLbl, fileType,
           modDateLbl, info->modifiedDate(),
           sizeLbl, info->fileSize()
        );
    } else {
        return QString(CONTENT_TEMPLATE_WITHOUT_FULLNAME).arg(
           typeLbl, fileType,
           modDateLbl, info->modifiedDate(),
           sizeLbl, info->fileSize()
        );
    }
}

QString TooltipsManager::getDisplayName(FileInfo* info) const
{
    QString displayName = info->displayName();

    auto settings = GlobalSettings::getInstance();
    bool showFileExtension = settings->isExist(SHOW_FILE_EXTENSION) ?
                             settings->getValue(SHOW_FILE_EXTENSION).toBool() :
                             true;

    if (showFileExtension || info->isDir() || !displayName.contains(".")) {
        return displayName;
    }

    return FileUtils::getBaseNameOfFile(displayName);
}

} // namespace Peony
