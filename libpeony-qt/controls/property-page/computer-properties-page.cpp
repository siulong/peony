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
 * Authors: Yue Lan <lanyue@kylinos.cn>
 *
 */

#include "computer-properties-page.h"

#include "linux-pwd-helper.h"

#include "file-utils.h"
#include "file-info.h"
#include "file-enumerator.h"
#include "file-count-operation.h"

#ifndef KY_UDF_BURN
#include "datacdrom.h"
#else
#include <libkyudfburn/datacdrom.h>
using namespace UdfBurn;
#endif

#include <QFormLayout>
#include <QFile>

#include <QLabel>
#include <QFrame>

#include <QProgressBar>
#include <QPushButton>
#include <QProcess>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QTimer>

#include <glib.h>
#include <gio/gdesktopappinfo.h>

using namespace Peony;

ComputerPropertiesPage::ComputerPropertiesPage(const QString &uri, QWidget *parent) : PropertiesWindowTabIface(parent)
{
    m_uri = uri;
    m_layout = new QFormLayout(this);
    m_layout->setContentsMargins(24, 16, 24, 16);
    m_layout->setRowWrapPolicy(QFormLayout::WrapLongRows);
    m_layout->setFormAlignment(Qt::AlignLeft);
    m_layout->setVerticalSpacing(8);
    m_layout->setHorizontalSpacing(24);
    //m_layout->setLabelAlignment(Qt::AlignRight);
    setLayout(m_layout);

    if (uri == "computer:///") {

        //computer infos
        QFile cpuInfo("/proc/cpuinfo");
        cpuInfo.open(QFile::ReadOnly);
        QFile memInfo("/proc/meminfo");
        memInfo.open(QFile::ReadOnly);

        auto cpuInfos = cpuInfo.readAll().split('\n');
        auto memInfos = memInfo.readAll().split('\n');

        QString cpuName;
        QString cpuCoreCount;
        QString memSize;

        for (QString string : cpuInfos) {
            if (!cpuName.isEmpty() && !cpuCoreCount.isEmpty()) {
                break;
            }

            if (string.startsWith("model name")) {
                cpuName = string.split(":").last();
            }

            if (string.startsWith("cpu cores")) {
                cpuCoreCount = string.split(":").last();
            }
        }

        for (QString string : memInfos) {
            if (string.startsWith("MemTotal")) {
                memSize = string.split(":").last();
                break;
            }
        }

        memSize.remove(" ");

        m_layout->addRow(tr("CPU Name:"), new QLabel(cpuName, this));
        m_layout->addRow(tr("CPU Core:"), new QLabel(cpuCoreCount, this));
        m_layout->addRow(tr("Memory Size:"), new QLabel(" "+memSize, this));

        cpuInfo.close();
        memInfo.close();

        addSeparator();

        //user infos
        auto user = LinuxPWDHelper::getCurrentUser();
        QString userName = user.fullName();
        userName.replace(',', " ");
        QString desktopEnv = g_getenv("XDG_CURRENT_DESKTOP");
        m_layout->addRow(tr("User Name: "), new QLabel(userName, this));
        m_layout->addRow(tr("Desktop: "), new QLabel(desktopEnv, this));
    } else {
        //FIXME: replace BLOCKING api in ui thread.
        auto targetUri = FileUtils::getTargetUri(uri);
        if (uri == "computer:///ukui-data-volume") {
            targetUri = "file:///data";
        }
        if (targetUri.isNull()) {
            m_layout->addRow(new QLabel(tr("You should mount this volume first"), nullptr));
            return;
        }

        if (targetUri == "file:///" || targetUri == "file:///data") {
            //NOTE: file:/// has not mount.
            GFile *file = g_file_new_for_uri(targetUri.toUtf8().constData());
            GFileInfo *info = g_file_query_filesystem_info(file, "*", nullptr, nullptr);
            quint64 total = g_file_info_get_attribute_uint64(info, G_FILE_ATTRIBUTE_FILESYSTEM_SIZE);
            quint64 used = g_file_info_get_attribute_uint64(info, G_FILE_ATTRIBUTE_FILESYSTEM_USED);
            quint64 available = g_file_info_get_attribute_uint64(info, G_FILE_ATTRIBUTE_FILESYSTEM_FREE);

            char *fs_type = g_file_info_get_attribute_as_string(info, G_FILE_ATTRIBUTE_FILESYSTEM_TYPE);
            QString type(fs_type);
            if (type.contains("ext")) {
                used = total - available;
            }
            m_layout->addRow(tr("Name: "), new QLabel(targetUri == "file:///" ? tr("System Disk") : tr("Data"), this));
            m_layout->addRow(tr("Total Space: "), new QLabel(formatCapacityString(total), this));
            m_layout->addRow(tr("Used Space: "), new QLabel(formatCapacityString(used), this));

            //story 28545, improve data block solution
            if (targetUri == "file:///data" && Peony::FileUtils::isFileExsit("file:///data/usershare") &&
                    Peony::FileUtils::isFileExsit("file:///data/root") && Peony::FileUtils::isFileExsit("file:///data/home")) {
               QLabel* rootUsedLabel = new QLabel(tr("In calculation..."), this);
               QLabel* homeUsedLabel = new QLabel(tr("In calculation..."), this);
               QLabel* usershareLabel = new QLabel(tr("In calculation..."), this);
               m_layout->addRow(tr("/root used: "), rootUsedLabel);
               m_layout->addRow(tr("/home used: "), homeUsedLabel);
               m_layout->addRow(tr("/usershare used: "), usershareLabel);

               QLabel* othersLabel = new QLabel(tr("In calculation..."), this);
               othersLabel->setVisible(false);
               if (FileUtils::isDataBlockHasUserFile()) {
                   othersLabel->setVisible(true);
                   m_layout->addRow(tr("/data/* used: "), othersLabel);
               }

               m_timer = new QTimer(this);
               m_timer->setInterval(500);
               connect(m_timer, &QTimer::timeout, this, [=]{
                   homeUsedLabel->setText(formatCapacityString(m_home_counted_size));
                   usershareLabel->setText(formatCapacityString(m_usershare_counted_size));
                   if (FileUtils::isDataBlockHasUserFile()) {
                       othersLabel->setText(formatCapacityString(m_others_counted_size));
                   }else {
                       m_count_others_done = true;
                   }

                   if (m_count_home_done && m_count_usershare_done && m_count_others_done) {
                       m_timer->stop();
                       if (used > (m_home_counted_size + m_root_counted_size + m_others_counted_size)) {
                           rootUsedLabel->setText(formatCapacityString(used - (m_home_counted_size + m_root_counted_size + m_others_counted_size)));
                       }else {
                           rootUsedLabel->setText(tr("Unknow (No permission)"));
                       }
                   }
               });
               m_timer->start();

               m_home_counted_size = 0;
               m_root_counted_size = 0;
               m_usershare_counted_size = 0;
               m_others_counted_size = 0;
               m_count_home_done = false;
               m_count_usershare_done = false;
               m_count_others_done = false;

               //计算家目录占用空间
               FileCountOperation *homeCountOp = new FileCountOperation(QStringList() << "file:///home", false);
               connect(homeCountOp, &FileOperation::operationPreparedOne, this, [=](QString uri, quint64 countSize) {
                   m_home_counted_size += countSize;
               }, Qt::BlockingQueuedConnection);
               connect(homeCountOp, &FileCountOperation::countDone, this, [=](quint64 file_count, quint64 hidden_file_count, quint64 total_size) {
                   homeUsedLabel->setText(formatCapacityString(total_size));
                   m_count_home_done = true;
               }, Qt::BlockingQueuedConnection);

               //计算useshare目录占用空间
               FileCountOperation *useshareCountOp = new FileCountOperation(QStringList() << "file:///data/usershare", false);
               connect(useshareCountOp, &FileOperation::operationPreparedOne, this, [=](QString uri, quint64 countSize) {
                   m_usershare_counted_size += countSize;
               }, Qt::BlockingQueuedConnection);
               connect(useshareCountOp, &FileCountOperation::countDone, this, [=](quint64 file_count, quint64 hidden_file_count, quint64 total_size) {
                   usershareLabel->setText(formatCapacityString(total_size));
                   m_count_usershare_done = true;
               }, Qt::BlockingQueuedConnection);

               //计算除了useshare，home, root之外的目录占用空间
               FileCountOperation *othersCountOp = new FileCountOperation(QStringList() << "file:///data", false);
               QStringList systemUris;
               systemUris<< "file:///data/root"<< "file:///data/home" <<"file:///data/usershare" << "file:///root" <<"file:///home";
               othersCountOp->setSkipUris(systemUris);
               connect(othersCountOp, &FileOperation::operationPreparedOne, this, [=](QString uri, quint64 countSize) {
                   m_others_counted_size += countSize;
               }, Qt::BlockingQueuedConnection);
               connect(othersCountOp, &FileCountOperation::countDone, this, [=](quint64 file_count, quint64 hidden_file_count, quint64 total_size) {
                   if (FileUtils::isDataBlockHasUserFile()) {
                       othersLabel->setText(formatCapacityString(m_others_counted_size));
                   }else {
                       m_others_counted_size = 0;
                   }
                   m_count_others_done = true;
               }, Qt::BlockingQueuedConnection);

               QThreadPool::globalInstance()->start(homeCountOp);
               QThreadPool::globalInstance()->start(useshareCountOp);
               QThreadPool::globalInstance()->start(othersCountOp);


               //阻塞方式，文件数量大场景阻塞，替换为异步方式
//               QTimer::singleShot(500, this, [=]() {
//                   quint64 homeUsed = FileUtils::getFileTotalSize("file:///home");
//                   quint64 usershareUsed = FileUtils::getFileTotalSize("file:///data/usershare");
//                   homeUsedLabel->setText(formatCapacityString(homeUsed));
//                   usershareLabel->setText(formatCapacityString(usershareUsed));
//                   quint64 rootUsed = 0;
//                   if (used > (homeUsed + usershareUsed)) {
//                       rootUsed = used - homeUsed -usershareUsed;
//                       rootUsedLabel->setText(formatCapacityString(rootUsed));
//                   }
//                   else
//                       rootUsedLabel->setText(tr("Unknow (No permission)"));

//                   if (FileUtils::isDataBlockHasUserFile()) {
//                       FileEnumerator e;
//                       e.setEnumerateDirectory("file:///data");
//                       e.enumerateSync();
//                       QStringList systemUris;
//                       systemUris<< "file:///data/root"<< "file:///data/home" <<"file:///data/usershare" << "file:///root" <<"file:///home";

//                       quint64 others = 0;
//                       for (auto fileInfo : e.getChildren()) {
//                          QString childUri = fileInfo->uri();
//                          if (systemUris.contains(childUri))
//                              continue;

//                          others += FileUtils::getFileTotalSize(childUri);
//                       }

//                       if (rootUsed > others) {
//                           rootUsed -= others;
//                           rootUsedLabel->setText(formatCapacityString(rootUsed));
//                       }else {
//                           rootUsedLabel->setText(tr("Unknow (No permission)"));
//                       }
//                       othersLabel->setText(formatCapacityString(others));
//                   }
//               });
            }

            m_layout->addRow(tr("Free Space: "), new QLabel(formatCapacityString(available), this));
            m_layout->addRow(tr("Type: "), new QLabel(fs_type, this));

            g_free(fs_type);
            g_object_unref(info);
            g_object_unref(file);

            auto progressBar = new QProgressBar(this);
            auto value = double(used*1.0/total)*100;
            progressBar->setValue(int(value));
            m_layout->addRow(progressBar);
            m_layout->setAlignment(progressBar, Qt::AlignBottom);
            return;
        }

        //FIXME: get volume info correctly.
        //使用枚举的方法解决空光盘显示未知分区的问题。#58255，#58199
        std::shared_ptr<Volume> volume = ComputerPropertiesPage::EnumerateOneVolumeByTargetUri(targetUri);
        std::shared_ptr<Mount>  mount  = nullptr;

        if (volume) {
            mount = std::make_shared<Mount>(g_volume_get_mount(volume->getGVolume()), true);
        } else {
            mount  = VolumeManager::getMountFromUri(targetUri);
            volume = VolumeManager::getVolumeFromMount(mount);
        }

        if (mount) {
            GFile *file = g_file_new_for_uri(targetUri.toUtf8().constData());
            GFileInfo *info = g_file_query_filesystem_info(file, "*", nullptr, nullptr);

            quint64 totalSpace     = 0;
            quint64 usedSpace      = 0;
            quint64 availableSpace = 0;

            bool isCDDisk = false;
            bool isFtp = false;

            if (info) {
                quint64 total = g_file_info_get_attribute_uint64(info, G_FILE_ATTRIBUTE_FILESYSTEM_SIZE);
                quint64 used  = g_file_info_get_attribute_uint64(info, G_FILE_ATTRIBUTE_FILESYSTEM_USED);
                quint64 free  = g_file_info_get_attribute_uint64(info, G_FILE_ATTRIBUTE_FILESYSTEM_FREE);

                if (nullptr != volume) {
                    char *deviceName = g_volume_get_identifier(G_VOLUME(volume->getGVolume()), G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
                    QString unixDeviceName;
                    if(deviceName) {
                        unixDeviceName = QString(deviceName);
                        g_free(deviceName);
                    }
                    //光盘
                    if (!unixDeviceName.isNull() && !unixDeviceName.isEmpty() && unixDeviceName.startsWith("/dev/sr")) {
                        isCDDisk = true;
#ifdef KY_UDF_BURN
                        UdfBurn::DataCDROM *cdrom = new UdfBurn::DataCDROM(unixDeviceName);
#else
                        Peony::DataCDROM *cdrom = new Peony::DataCDROM(unixDeviceName);
#endif
                        if (cdrom) {
                            cdrom->getCDROMInfo();
                            //usedSpace = used;
                            //used无法正确获取追加刻录后光盘的使用容量，getCDROMUsedCapacity()无法获取可擦除光盘的使用容量
                            usedSpace = cdrom->getCDROMUsedCapacity();
                            if((cdrom->getCDROMType()).contains("DVD+RW") || (cdrom->getCDROMType()).contains("DVD-RW")){
                                QString fsType = getFileSystemType(uri);
                                if(fsType.toLower().startsWith("iso")){/* "iso9660"文件系统类型"DVD+RW"和"DVD-RW"光盘的使用容量从libkyudfburn中获取,link bug#271455. */
                                    usedSpace = cdrom->getCDROMUsedCapacity();
                                }else{
                                    usedSpace =used;
                                }
                            }
                            totalSpace = cdrom->getCDROMCapacity();
                            availableSpace = totalSpace - usedSpace;
                            delete cdrom;
                            cdrom = nullptr;
                        }
                    }
                }

                if (totalSpace == 0) {
                    if (total > 0 && (used > 0 || free > 0)) {
                        if (used > 0 && used <= total) {
                            usedSpace = used;
                            totalSpace = total;
                        } else if (free > 0 && free <= total) {
                            usedSpace = total - free;
                            totalSpace = total;
                        }
                    }
                    availableSpace = free;
                }
            }

            char *fs_type = g_file_info_get_attribute_as_string(info, G_FILE_ATTRIBUTE_FILESYSTEM_TYPE);
            //use dubs to get file system type, fix ntfs show as fuse issue
            QString type = getFileSystemType(uri);
            if (type.length() <=0)
                type = fs_type;
            if (type.contains("ext")) {
                usedSpace = totalSpace - availableSpace;
            }
            if (targetUri.startsWith("ftp://")) {
                isFtp = true;
            }
            m_layout->addRow(tr("Name: "), new QLabel(mount->name(), this));
/*            if (bMobileDevice)
                m_layout->addRow(tr("Total Space: "), new QLabel(sizeInfo, this));
            else */
            if (!isFtp) {
                m_layout->addRow(tr("Total Space: "), new QLabel(formatCapacityString(totalSpace), this));
                m_layout->addRow(tr("Used Space: "), new QLabel(formatCapacityString(usedSpace), this));
                m_layout->addRow(tr("Free Space: "), new QLabel(formatCapacityString(availableSpace), this));
            }
            m_layout->addRow(tr("Type: "), new QLabel(type, this));

            if (!isFtp) {
                auto progressBar = new QProgressBar(this);
                auto value = double(usedSpace*1.0/totalSpace)*100;
                progressBar->setValue(int((value > 0 && value < 1 ) ? 1 : value));
                m_layout->addRow(progressBar);
                m_layout->setAlignment(progressBar, Qt::AlignBottom);
            }

            //fix bug#141923, empty disks not show kylin burner issue
            //fix bug#146557, U盘系统盘显示刻录软件问题
            if (isCDDisk && QFile::exists("/usr/bin/kylin-burner")) {
                auto pushbutton = new QPushButton(tr("Kylin Burner"));
                connect(pushbutton, &QPushButton::clicked, pushbutton, [=](){
                    QString path = "/usr/share/applications/kylin-burner.desktop";
                    g_autoptr(GDesktopAppInfo) appInfo = g_desktop_app_info_new_from_filename(path.toUtf8().constData());
                    if (info) {
                        g_app_info_launch_uris_async(G_APP_INFO(appInfo), nullptr,
                                                     nullptr, nullptr,
                                                     nullptr, nullptr);
                    }
                });
                m_layout->addRow(new QLabel(tr("Open with: \t")), pushbutton);
            }

            g_free(fs_type);
            g_object_unref(info);
            g_object_unref(file);
        } else {
            m_layout->addRow(new QLabel(tr("Unknown"), nullptr));
        }
    }
}

void ComputerPropertiesPage::addSeparator()
{
    auto separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    m_layout->addRow(separator);
}

void ComputerPropertiesPage::saveAllChange()
{

}

QString ComputerPropertiesPage::getFileSystemType(QString uri)
{
    QString unixDevice,dbusPath;
    QString fsType = "";

    unixDevice = FileUtils::getUnixDevice(uri);

    //fix bug#95731, encrypted data disk show property crash issue
    //encrypted disk unixDevice name is like /dev/mapper/kylin--vg-data
    if (unixDevice.isEmpty() ||
        ! (unixDevice.startsWith("/dev/sd") || unixDevice.startsWith("/dev/sr"))) {
        return fsType;
    }
    dbusPath = "/org/freedesktop/UDisks2/block_devices/" + unixDevice.split("/").last();
    if (! QDBusConnection::systemBus().isConnected())
        return fsType;
    QDBusInterface blockInterface("org.freedesktop.UDisks2",
                                  dbusPath,
                                  "org.freedesktop.DBus.Properties",
                                  QDBusConnection::systemBus());

    if(blockInterface.isValid())
        fsType = blockInterface.property("IdType").toString();

    //if need diff FAT16 and FAT32, should use IdVersion
//    if(fsType == "" && blockInterface.isValid())
//        fsType = blockInterface.property("IdVersion").toString();

    return fsType;
}

std::shared_ptr<Volume> ComputerPropertiesPage::EnumerateOneVolumeByTargetUri(QString targetUri)
{
    std::shared_ptr<Volume> volume = nullptr;
    //enumerate
    auto volume_monitor = g_volume_monitor_get();
    auto current_volumes = g_volume_monitor_get_volumes(volume_monitor);
    GList *l = current_volumes;

    while (l) {
        volume = std::make_shared<Volume>(G_VOLUME(l->data), true);
        GMount *gMount = g_volume_get_mount(volume->getGVolume());
        GFile *gFile = g_mount_get_root(gMount);
        char *volumePath = g_file_get_uri(gFile);
        bool isCurrentVolume = (volumePath == targetUri);

        g_object_unref(gMount);
        g_object_unref(gFile);
        g_free(volumePath);

        if (isCurrentVolume)
            break;

        volume = nullptr;
        l = l->next;
    }

    return volume;
}

QString ComputerPropertiesPage::formatCapacityString(quint64 capacityNum)
{
//    char *strGB = g_format_size_full(capacityNum, G_FORMAT_SIZE_DEFAULT);
    char *strGiB = g_format_size_full(capacityNum, G_FORMAT_SIZE_IEC_UNITS);

//    QString formatString("");
//    formatString = QString("%1%2%3%4").arg(strGB).arg(" (").arg(strGiB).arg(")");

    QString formatString(strGiB);
    //根据设计要求，按照1024字节对数据进行格式化（1GB = 1024MB），同时将GiB改为GB显示，以便于用户理解。参考windows显示样式。
    formatString.replace("iB", "B");

//    g_free(strGB);
    g_free(strGiB);

    return formatString;
}
