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

#ifndef TOOLTIPSMANAGER_H
#define TOOLTIPSMANAGER_H

#include <QObject>
#include "peony-core_global.h"

namespace Peony {
class FileInfo;

/**
 * @class TooltipsManager
 * @brief Manages the generation of tooltips for file information.
 *
 * This class is responsible for creating tooltips with relevant information
 * for different types of files, including images, videos, audio, and regular files.
 */
class PEONYCORESHARED_EXPORT TooltipsManager: QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(TooltipsManager)
public:
    /**
     * @brief Get the singleton instance of TooltipsManager.
     * @return Reference to the TooltipsManager instance.
     */
    static TooltipsManager& instance();

    /**
     * @brief Generate a tooltip for the given file information.
     * @param info Pointer to the FileInfo object.
     * @return QString containing the generated tooltip HTML.
     */
    QString generateTooltip(FileInfo* info) const;

    /**
     * @brief Get the duration of a media file.
     * @param filePath Path to the media file.
     * @param timeoutMs Timeout for the ffprobe process in milliseconds.
     * @return QString containing the duration in HH:MM:SS format.
     */
    static QString getMediaDuration(const QString &filePath, int timeoutMs = 3000);

    /**
     * @brief Check if the given file is a system file.
     * @param info Pointer to the FileInfo object.
     * @return true if it's a system file, false otherwise.
     */
    static bool isSystemFile(FileInfo* info);

private:
    TooltipsManager() = default;
    ~TooltipsManager() = default;

    /**
     * @brief Generate tooltip for video and audio files.
     * @param info Pointer to the FileInfo object.
     * @return QString containing the generated tooltip HTML.
     */
    QString toVideoAudio(FileInfo* info) const;

    /**
     * @brief Generate tooltip for image files.
     * @param info Pointer to the FileInfo object.
     * @return QString containing the generated tooltip HTML.
     */
    QString toImage(FileInfo* info) const;

    /**
     * @brief Generate tooltip for normal files.
     * @param info Pointer to the FileInfo object.
     * @return QString containing the generated tooltip HTML.
     */
    QString toNormalFile(FileInfo* info) const;

    /**
     * @brief Get the display name for a file.
     * @param info Pointer to the FileInfo object.
     * @return QString containing the display name.
     */
    QString getDisplayName(FileInfo* info) const;
};

}   // namespace Peony

#define TooltipsManagerInstance Peony::TooltipsManager::instance()

#endif // TOOLTIPSMANAGER_H
