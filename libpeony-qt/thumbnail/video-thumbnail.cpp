/*
 * Peony-Qt's Library
 *
 * Copyright (C) 2020, KylinSoft Co., Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Authors: renpeijia <renpeijia@kylinos.cn>
 *
 */

#include "generic-thumbnailer.h"
#include "video-thumbnail.h"
#include "file-utils.h"
#include <QFileInfo>
#include <QDebug>
#include <QtConcurrent>
#include <QImage>
#include <QMessageAuthenticationCode>
#include <QPainter>
#include <QImageReader>
#include <qglobal.h>

VideoThumbnail::VideoThumbnail(const QString &uri)
{
    if (!uri.startsWith("file:///")) {
        auto fileInfo = FileInfo::fromUri(uri);
        m_url = fileInfo.get()->filePath();
        if (m_url.isEmpty()) {
            m_url = FileUtils::getTargetUri(uri);
        }
        qDebug()<<"target uri:"<< m_url.path();
    }
    else {
        m_url = uri;
    }

    auto fileInfo = FileInfo::fromUri(uri);
    m_modifyTime = fileInfo->modifiedTime();
}

VideoThumbnail::~VideoThumbnail()
{

}

/*
*获取要显示的图片的位置，因为视频文件的前几帧有可能没有图像是黑的
*/
QMap<QString, QString> VideoThumbnail::videoInfo()
{
    QMap<QString, QString> map;
    QStringList list2;
    list2<<"-hwaccel"<<"auto";
    list2<<"-i"<< m_url.path();
    QString ret="5.0";
    QString time=QString();
    map["Pos"]=ret;
    map["Time"]=time;
    QProcess p;

    p.start("/usr/bin/ffmpeg",list2);
    if (!p.waitForStarted()) {
        return map ;
    }

    if (!p.waitForFinished()) {
        return map;
    }

    QString error=p.readAllStandardError();

    if(error.isEmpty())
        return map;

    QStringList list=error.split("\n");

    for (QString s:list) {
        if (s.trimmed().startsWith("Duration")) {
            s=s.remove("Duration:");
            QString name=s.section(",",0,0);

            QStringList listtime=name.trimmed().split(":");
            if (listtime.count()>=3) {
                QString h=listtime.at(0);
                QString m=listtime.at(1);
                QString s=listtime.at(2);

                if (h.toFloat()>0) {
                    ret= "15.0"; time+=h+":";
                }
                else if (m.toFloat()>0) {
                    ret=  "7.0";
                }
                else if (s.toFloat()<=1) {
                    ret=  "0.1";
                }
                else if (s.toFloat()<=5) {
                    ret=  "1.0";
                }
                else if (s.toFloat()<=10) {
                    ret=  "3.0";
                }
                else if (s.toFloat()>10 ) {
                    ret=    ret;
                }

                time+=m+":";
                time+=s.leftRef(2);

                qDebug()<<"info"<<ret+"|"+time;;

                map["Pos"]=ret;
                map["Time"]=time;
                return map;
            }
        }
    }

    return map;
}

QIcon VideoThumbnail::generateThumbnail()
{
    QIcon thumbnailImage;
    static const QString thumbnailDir = GenericThumbnailer::thumbnaileCachDir();
    const QString md5Name = GenericThumbnailer::codeMd5WithModifyTime(m_url.path(), m_modifyTime);
    const QString fileThumbnail = thumbnailDir + "/" + md5Name;

    /**
     * @bug #192691: [M900] [Audio/Video] The probability of adding a video will be stuttered,
     *  and the added video will not be played until it has stuttered for more than 20s (Probability of recurrence: 2/10)
     *
     * Use the ffmpegthumbnailer command to get the thumbnail of a video file to improve the efficiency of getting it.
     *
     * @author: Renyg <renyangguang@kylinos.cn>
     * @date:   2024-10-21
     */
    // Check if thumbnail already exists
    if (!QFile::exists(fileThumbnail)) {
        QStringList arguments;
        arguments << "-i" << m_url.path()     // Input file
                  << "-o" << fileThumbnail    // Output file
                  << "-s" << "640";           // Thumbnail size
        // Uncomment the following lines to add more options
        // << "-t" << "10%"                   // Seek to 10% of the video
        // << "-q" << "8"                     // JPEG quality
        // << "-f";                           // Add film strip effect

        QProcess ffmpegProcess;
        ffmpegProcess.start("ffmpegthumbnailer", arguments);

        // Wait for the process to start
        if (!ffmpegProcess.waitForStarted()) {
            qWarning() << "Failed to start ffmpegthumbnailer process.";
        }

        // Wait for the process to finish with a timeout
        constexpr int timeout = 10000; // 10 seconds
        if (!ffmpegProcess.waitForFinished(timeout)) {
            qWarning() << "ffmpegthumbnailer process timed out.";
            ffmpegProcess.kill();
        }

        // Check for any errors
        const QString errorOutput = ffmpegProcess.readAllStandardError();
        if (!errorOutput.isEmpty()) {
            qWarning() << "ffmpegthumbnailer process reported an error:" << errorOutput;
        }
    }

    // Generate the thumbnail icon
    thumbnailImage = GenericThumbnailer::generateThumbnail(fileThumbnail, true);

    return thumbnailImage;
}
