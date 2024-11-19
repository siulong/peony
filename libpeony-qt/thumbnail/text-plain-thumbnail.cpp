/*
 * Peony-Qt's Library
 *
 * Copyright (C) 2022, KylinSoft Co., Ltd.
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
 * Authors: hemeihong <hemeihong@kylinos.cn>
 *
 */

#include "text-plain-thumbnail.h"
#include "generic-thumbnailer.h"
#include "file-utils.h"
#include <QFileInfo>
#include <QDebug>
#include <QtConcurrent>
#include <QImage>
#include <QMessageAuthenticationCode>
#include <QPainter>
#include <QImageReader>
#include <qglobal.h>

textPlainThumbnail::textPlainThumbnail(const QString &uri)
{
    if (!uri.startsWith("file:///")) {
        m_url = FileUtils::getTargetUri(uri);
        qDebug()<<"target uri:"<< m_url.path();
    }
    else {
        m_url = uri;
    }

    auto fileInfo = FileInfo::fromUri(uri);
    m_modifyTime = fileInfo->modifiedTime();
}

textPlainThumbnail::~textPlainThumbnail()
{

}

/*
*函数功能：
现将文本文档转换为PDF文档，再转换为jpg文件，直接转换为图片会失败
*/
QIcon textPlainThumbnail::generateThumbnail()
{
    QIcon thumbnailImage;
    QString md5Name=GenericThumbnailer::codeMd5WithModifyTime(m_url.path(), m_modifyTime);
    QString thumbnail_dir= GenericThumbnailer::thumbnaileCachDir() + "/" + md5Name;
    QString fileName = m_url.fileName();
    qint16 idx = fileName.lastIndexOf(".");
    QString fileThumbnail=thumbnail_dir + "/" + fileName.left(idx) + ".jpg";

    //优化无效流程，未安装libreoffice则直接返回
    if (! QFile::exists("/usr/bin/libreoffice"))
    {
        qDebug()<<"libreoffice not installed, textPlainThumbnail return";
        return thumbnailImage;
    }

    qDebug()<<"file thumbnail:"<<fileThumbnail;
    if (!QFile::exists(fileThumbnail)) {
        //libreoffice --convert-to PDF --convert-to jpg:writer_jpg_Export test1.txt --outdir ./
        QStringList list;
        //text file also use libreoffice create thumbnail,related to task#82064
        //注意txt文档需要先转换为pdf文档再转图片，直接转图片会生成缩略图失败
        list<<"--headless"  /*headless和invisible的方式可以避免出现界面以及无用的log信息，速度更快*/
            <<"--invisible"
              //新版本可以直接txt文档转图片
//            <<"--convert-to"
//            <<"PDF"                       /*老版本libreoffice,txt文档先转换为PDF文档再转图片，直接转图片会失败*/
            <<"--convert-to"
            <<"jpg:writer_jpg_Export"     /*转换格式jpg*/
            <<m_url.path()                /*要转换的文件*/
            <<"--outdir"                  /*转换完的jpg文件存在的路径*/
            <<thumbnail_dir;

        qDebug()<<"the libreoffice cmd: " << list;

        QProcess p;
        p.start("/usr/bin/libreoffice",list);

        /*
        * 等待30s超时，30s是默认时间，可以修改
        */
        if (!p.waitForStarted()) {
            qWarning()<<"libreoffice start failed, or timeout";
            return thumbnailImage;
        }

        /*
        * 等待30s超时，30s是默认时间，可以修改
        */
        if (!p.waitForFinished()) {
            qWarning()<<"libreoffice run failed, or timeout";
            return thumbnailImage;
        }

        QString err=p.readAllStandardError();
        //QString read=p.readAll();
        if (!err.isEmpty()) {
            qWarning()<<"office convert jpg error: " << err;
            return thumbnailImage;
        }
    }

    /*
     *不知道会不会出现无效的图片的情况，可能需要对这种情况做处理？
    */
    thumbnailImage = GenericThumbnailer::generateThumbnail(fileThumbnail, true);

    return thumbnailImage;
}

