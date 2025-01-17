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
#include <QFile>
#include <QFont>
#include <QPen>
#include <QPainter>
#include <QTextOption>
#include <qglobal.h>

textPlainThumbnail::textPlainThumbnail(const QString &uri)
{
    if (!uri.startsWith("file:///")) {
        auto fileInfo = FileInfo::fromUri(uri);
        m_url = fileInfo.get()->filePath();
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

    QImage image= gernerateTextImage();
    if (! image.isNull()) {
        QPixmap pixmap = QPixmap::fromImage(image);
        //thumbnailImage.addPixmap(pixmap);
        thumbnailImage = QIcon(pixmap);
        return thumbnailImage;
    }

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

/*
*函数功能：
现将文本文档读取开头部分内容，绘制生成图片供缩略图使用
解决使用libreofice文本转图片看不清内容问题
同时可以不依赖libreofice库接口
*/
QImage textPlainThumbnail::gernerateTextImage()
{
    QImage thumbnailImg;
    const int imageSize = 128;            ///< Final thumbnail size
    const int leftMargin = 15;            ///< Left margin of content area
    const int rightMargin = 18;           ///< Right margin of content area
    const int topMargin = 1;              ///< Top margin of content area
    const int bottomMargin = 8;           ///< Bottom margin of content area
    const int cornerRadius = 12;          ///< Corner radius for rounded rectangle
    const int textPadding = 5;            ///< Padding between text and border
    const int maxReadSize = 2000;         ///< Maximum bytes to read from file

    // Calculate content area dimensions
    const int contentWidth = imageSize - leftMargin - rightMargin;
    const int contentHeight = imageSize - topMargin - bottomMargin;

    // Open and validate file
    QFile file(m_url.path());
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        qWarning() << "thumbnail: can not open this file." << m_url.path();
        return thumbnailImg;
    }

    // Check file size
    QFileInfo fileInfo(file);
    if (fileInfo.size() <= 0) {
        qWarning() << "thumbnail: file is empty." << m_url.path();
        return thumbnailImg;
    }

    // Read and decode file content with size limitation
    QByteArray content = file.read(maxReadSize);
    QString text;
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    if (!codec) {
        codec = QTextCodec::codecForLocale();
    }
    text = codec->toUnicode(content);

    // Validate text content
    if (text.isEmpty()) {
        qWarning() << "thumbnail: text content is empty." << m_url.path();
        return thumbnailImg;
    }

    // Create final transparent image
    thumbnailImg = QImage(imageSize, imageSize, QImage::Format_ARGB32_Premultiplied);
    thumbnailImg.fill(Qt::transparent);

    // Create content area image
    QImage contentImg(contentWidth, contentHeight, QImage::Format_ARGB32_Premultiplied);
    contentImg.fill(Qt::transparent);

    // Setup content painter with high quality rendering hints
    QPainter contentPainter(&contentImg);
    contentPainter.setRenderHint(QPainter::Antialiasing, true);
    contentPainter.setRenderHint(QPainter::TextAntialiasing, true);
    contentPainter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    contentPainter.setRenderHint(QPainter::HighQualityAntialiasing, true);

    // Create and draw rounded rectangle path
    QPainterPath path;
    path.addRoundedRect(0.5, 0.5, contentWidth - 1, contentHeight - 1,
                       cornerRadius, cornerRadius);

    // Fill white background
    contentPainter.fillPath(path, Qt::white);

    // Draw light border
    QPen borderPen(QColor(0, 0, 0, 25));  ///< Semi-transparent black for border
    borderPen.setWidth(1);
    contentPainter.setPen(borderPen);
    contentPainter.drawPath(path);

    // Set clip path to contain text within rounded corners
    contentPainter.setClipPath(path);

    // Configure font settings
    QFont font;
    font.setPixelSize(13);
    font.setHintingPreference(QFont::PreferFullHinting);
    font.setStyleStrategy(QFont::PreferQuality);
    contentPainter.setFont(font);
    contentPainter.setPen(Qt::black);

    // Configure text layout options
    QTextOption option;
    option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    option.setAlignment(Qt::AlignLeft | Qt::AlignTop);

    // Calculate text area with padding
    QRect textRect = contentImg.rect().adjusted(textPadding, textPadding,
                                              -textPadding, -textPadding);

    // Draw text content
    contentPainter.drawText(textRect, text, option);
    contentPainter.end();

    // Draw content onto final image
    QPainter finalPainter(&thumbnailImg);
    finalPainter.setRenderHint(QPainter::Antialiasing, true);
    finalPainter.drawImage(leftMargin, topMargin, contentImg);
    finalPainter.end();

    file.close();
    return thumbnailImg;
}
