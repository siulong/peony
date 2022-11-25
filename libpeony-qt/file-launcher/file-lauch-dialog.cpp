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
#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QDebug>
#include <QPainter>
#include <QApplication>
#include <QMainWindow>

#include "file-lauch-dialog.h"
#include "properties-window.h"
#include "file-launch-action.h"
#include "file-launch-manager.h"

#include "file-info.h"
#include "xatom-helper.h"
#include "global-settings.h"
#include <gio/gdesktopappinfo.h>

#define DESKTOPPATH           "/usr/share/applications/"

using namespace Peony;

FileLauchDialog::FileLauchDialog(const QString &uri, QWidget *parent) : QDialog(parent)
{
    this->getFIleInfo(uri);
    QString fileName = m_info->displayName();
    setWindowFilePath(fileName);
    QIcon windowicon = QIcon::fromTheme(m_info->iconName());
    setWindowIcon(windowicon);
    this->setWindowFlags(windowFlags() & ~Qt::WindowMinMaxButtonsHint );
    init(uri);
}
void FileLauchDialog::getFIleInfo(QString uri)
{
    std::shared_ptr<FileInfo> fileInfo = FileInfo::fromUri(uri);
    FileInfoJob *fileInfoJob = new FileInfoJob(fileInfo);
    fileInfoJob->setAutoDelete();
    fileInfoJob->querySync();
    m_info = fileInfo;
}

void FileLauchDialog::saveChange()
{
    if (!OpenWithPropertiesPage::openWithGlobalData) {
        return;
    }

    FileLaunchAction* newAction = OpenWithPropertiesPage::openWithGlobalData->getActionByUri(m_info->uri());
    if (newAction) {
        QString newAppId(g_app_info_get_id(newAction->gAppInfo()));
        QString oldAppId(g_app_info_get_id(FileLaunchManager::getDefaultAction(m_info->uri())->gAppInfo()));
        if (newAppId != oldAppId) {
            FileLaunchManager::setDefaultLauchAction(m_info->uri(), newAction);
        }
    }

    OpenWithPropertiesPage::openWithGlobalData->removeAction(m_info->uri());
}

void FileLauchDialog::init(const QString &uri)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(10,0,10,10);

    this->initFloorOne();
    this->addSeparator();

    this->initFloorTwo(uri);
    this->addSeparator();

    this->initFloorThree();
    this->addSeparator();

    this->initFloorFour();
    this->setLayout(m_layout);
}

void FileLauchDialog::initFloorOne()
{
    QFrame      *floor1  = new QFrame(this);
    QVBoxLayout *layout1 = new QVBoxLayout(this);
    layout1->setContentsMargins(22,0,22,0);

    floor1->setLayout(layout1);
    floor1->setMaximumHeight(142);

    QLabel *targetTypeMsgLabel = new QLabel(floor1);
    targetTypeMsgLabel->setMinimumHeight(60);
    qDebug() << "targetTypeMsgLabel :" << targetTypeMsgLabel->height();

    if(m_info.get()->displayName().contains(".")||m_info.get()->isDir() || m_info.get()->isDesktopFile()){
        QString appname = FileLaunchManager::getDefaultAction(m_info.get()->uri())->getAppInfoDisplayName();
        if("" == appname)
        {
            targetTypeMsgLabel->setText(tr("No application is set to open file %1").arg(m_info.get()->displayName()));
            layout1->addWidget(targetTypeMsgLabel);

        }
        else{
//            if(m_info.get()->isDir() || m_info.get()->isDesktopFile()){
//                targetTypeMsgLabel->setText(tr("How do you want to open %1 files ?").arg(m_info.get()->displayName()));
//            }else{
//                targetTypeMsgLabel->setText(tr("How do you want to open %1%2 files ?").arg(".").arg(m_info.get()->displayName().split(".").last()));
//            }

//            layout1->addWidget(targetTypeMsgLabel);
            QLabel *defaultOpenLabel = new QLabel(floor1);
            defaultOpenLabel->setMinimumHeight(55);
            defaultOpenLabel->setText(tr("Open application is used by default:"));
            layout1->addWidget(defaultOpenLabel);


            auto action = FileLaunchManager::getDefaultAction(m_info->uri());
            OpenWithPropertiesPage::setNewLaunchAction(action,false);
            m_defaultOpenWithWidget = OpenWithPropertiesPage::createDefaultOpenWithWidget(m_info->uri(), floor1);

            m_defaultOpenWithWidget->setMinimumHeight(45);
            layout1->addWidget(m_defaultOpenWithWidget);
        }
    }else{
        targetTypeMsgLabel->setText(tr("No application is set to open file %1").arg(m_info.get()->displayName()));

        layout1->addWidget(targetTypeMsgLabel);
        QLabel *tipsLabel = new QLabel(floor1);
        tipsLabel->setMinimumHeight(35);
        tipsLabel->setMaximumWidth(546);
        tipsLabel->setWordWrap(true);
        tipsLabel->setText(tr("You can search in the Software Center for an application that can open this file, or select an existing application on your computer."));
        layout1->addWidget(tipsLabel);

    }
        m_layout->addWidget(floor1);
}

void FileLauchDialog::initFloorTwo(const QString &uri)
{
    QFrame      *floor2  = new QFrame(this);
    QVBoxLayout *layout2 = new QVBoxLayout(this);
    floor2->setLayout(layout2);
    floor2->setMaximumHeight(336);
    layout2->setContentsMargins(0,0,0,0);

    QLabel *otherOpenLabel = new QLabel(floor2);
    if(m_info.get()->displayName().contains(".")||m_info.get()->isDir() || m_info.get()->isDesktopFile()){
        otherOpenLabel->setText(tr("Other application:"));
    }else{
        otherOpenLabel->setText(tr("Select application:"));
    }

    otherOpenLabel->setContentsMargins(22,0,0,0);
    layout2->addWidget(otherOpenLabel);
    otherOpenLabel->setFixedHeight(30);

    m_view = new QListWidget(this);

    auto actions = FileLaunchManager::getAllActions(uri);
    for (auto action : actions) {
        if (action->icon().isNull() ||
                "org.gnome.font-viewer" == action->icon().name())
            continue;

        if (action->icon().name().contains("uninstall"))
            continue;

        action->setParent(this);
        auto item = new QListWidgetItem(!action->icon().isNull()? action->icon(): QIcon::fromTheme("application-x-desktop"),
                                        action->text(),
                                        m_view);
        m_view->addItem(item);
        m_hash.insert(item, action);
    }
    //自定义列表属性
    m_view->setFrameShape(QListWidget::NoFrame);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setFixedHeight(180);
    m_view->setAlternatingRowColors(true);
    m_view->setIconSize(QSize(40, 40));
    m_view->setStyleSheet("QListWidget::Item{margin-left:22px;}");

    connect(m_view, &QListWidget::currentItemChanged, [=](QListWidgetItem *current) {
        if (!current)
            return ;
        FileLaunchAction *action = m_hash.value(m_view->currentItem());
        if (action == FileLaunchManager::getDefaultAction(m_info.get()->uri()))
            return ;

        OpenWithPropertiesPage::setNewLaunchAction(action, false);
    });

    layout2->addWidget(m_view);

    this->m_layout->addWidget(floor2);
    m_check_box = new QCheckBox(tr("Always open the %1%2 file with this application").arg(".").arg(m_info.get()->displayName().split(".").last()));

    this->m_layout->addWidget(m_check_box);
    if(!m_info.get()->displayName().contains("."))
    m_check_box->setVisible(false);

    //FIXME: replace BLOCKING api in ui thread.
    auto info = FileInfo::fromUri(uri);
    if (info->isDir() || info->isDesktopFile()) {
        m_check_box->setEnabled(false);
    }
}

void FileLauchDialog::initFloorThree()
{
    QFrame      *floor3  = new QFrame(this);
    QVBoxLayout *layout3 = new QVBoxLayout(this);
    floor3->setLayout(layout3);
    floor3->setMaximumHeight(60);
    layout3->setContentsMargins(22,0,0,0);

    QString str1;
    str1 = "<a href=\"ukui-software-center\" style=\"color: #3D6BE5;text-underline: none;\">"
          + tr("Choose other application")
          + "</a>";
    QLabel *allOpenLabel = new QLabel(str1, floor3);
    allOpenLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    connect(allOpenLabel, &QLabel::linkActivated, this, [=]() {
        FileLauchDialog::moreAction();
    });

    layout3->addWidget(allOpenLabel);

    QString str2;
    str2 = "<a href=\"ukui-software-center\" style=\"color: #3D6BE5;text-underline: none;\">"
          + tr("Go to application center")
          + "</a>";
    QLabel *otherOpenLabel = new QLabel(str2, floor3);
    otherOpenLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    connect(otherOpenLabel, &QLabel::linkActivated, this, [=]() {
        QtConcurrent::run([=]() {
            QProcess p;
            if (COMMERCIAL_VERSION || (GlobalSettings::getInstance()->getProjectName() == V10_SP1_EDU))
                p.setProgram("kylin-software-center");
            else
                p.setProgram("ubuntu-kylin-software-center");

#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
            p.startDetached();
#else
            if (COMMERCIAL_VERSION)
                p.startDetached("kylin-software-center");
            else
                p.startDetached("ubuntu-kylin-software-center");

#endif
        });
    });

    bool isVisible = false;
    if(QFileInfo::exists("/usr/bin/kylin-software-center")
            || QFileInfo::exists("/usr/bin/ubuntu-kylin-software-center")){
        isVisible = true;
    }

    otherOpenLabel->setVisible(isVisible);
    layout3->addWidget(otherOpenLabel);
    layout3->addStretch(1);

    this->m_layout->addWidget(floor3);

}

void FileLauchDialog::initFloorFour()
{
    QWidget *container = new QWidget();
    container->setContentsMargins(0, 0, 0, 0);
    container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    container->setMaximumHeight(50);
    //use HBox-layout
    QHBoxLayout *bottomToolLayout = new QHBoxLayout(container);
    bottomToolLayout->setSpacing(16);
    bottomToolLayout->setContentsMargins(24, 0, 24, 12);

    QPushButton *okButton = new QPushButton(tr("Ok"), container);
    QPushButton *cancelButton = new QPushButton(tr("Cancel"), container);

    bottomToolLayout->addStretch(1);
    bottomToolLayout->addWidget(cancelButton);
    bottomToolLayout->addWidget(okButton);

    connect(cancelButton, &QPushButton::clicked, this, &QMainWindow::close);
    connect(okButton, &QPushButton::clicked, this, [=]() {
        if (m_hash.value(m_view->currentItem())) {
            auto action = m_hash.value(m_view->currentItem());
            if (m_check_box->isChecked()) {
                FileLauchDialog::saveChange();
            }
            action->lauchFileAsync(true);
        }
        else {
            FileLaunchAction *action = FileLaunchManager::getDefaultAction(m_info->uri());
            if (m_check_box->isChecked()) {
                FileLauchDialog::saveChange();
            }
            action->lauchFileAsync(true);
//            FileLaunchManager::openAsync(m_uri);
        }
    });

    container->setLayout(bottomToolLayout);
    this->m_layout->addWidget(container);
}

void FileLauchDialog::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(this->rect(), qApp->palette().base());
    QWidget::paintEvent(event);
}

bool FileLauchDialog::event(QEvent *event)
{
    //失去焦点即关闭窗口
    if (event->type() == QEvent::ActivationChange) {
        if (QApplication::activeWindow() != this) {
            this->close();
            return false;
        }
    }
    return QWidget::event(event);
}

void FileLauchDialog::moreAction()
{
    QString filters = tr("Desktop files(*.desktop)");
    fd = new QFileDialog(this);
    fd->setDirectory(DESKTOPPATH);
    fd->setAcceptMode(QFileDialog::AcceptOpen);
    fd->setViewMode(QFileDialog::List);
    fd->setNameFilter(filters);
    fd->setFileMode(QFileDialog::ExistingFile);
    fd->setWindowTitle(tr("Select Open Action"));
    fd->setLabelText(QFileDialog::Accept, tr("Select"));
    fd->setLabelText(QFileDialog::Reject, tr("Cancel"));
    if (fd->exec() != QDialog::Accepted)
        return;

    QString selectedfile;
    selectedfile = fd->selectedFiles().first();

    QByteArray ba;
    ba = selectedfile.toUtf8();
    QString uri;
    uri = selectedfile;

    // 解析desktop文件
    GKeyFile *keyfile;
    keyfile = g_key_file_new();
    if (!g_key_file_load_from_file(keyfile, ba.data(), G_KEY_FILE_NONE, NULL)) {
        g_key_file_free(keyfile);
        return;
    }

    auto info = FileInfo::fromUri(uri);
    QString mimeType = info->mimeType();

    if (mimeType.isEmpty()) {
        FileInfoJob job(info);
        job.querySync();
        mimeType = info->mimeType();
    }

    if (info->uri().endsWith(".desktop")) {
        QUrl url = uri;
        auto path = url.path();
        bool isMdmApp = false;
        qDebug() << "getDefaultAction uri:"<<uri;
        GDesktopAppInfo *info = g_desktop_app_info_new_from_filename(path.toUtf8().constData());
        //fix bug#130690, set mdm app can not get info, open tips not correct issue
        if (!info) {
            GKeyFile *keyfile = g_key_file_new();
            g_key_file_load_from_file(keyfile, path.toUtf8().data(), G_KEY_FILE_NONE, nullptr);
            g_autofree gchar* execmd = g_key_file_get_string(keyfile, "Desktop Entry", "Exec", nullptr);
            QString cmd = execmd;
            // 通过ukui-menu的配置文件判断
            QString settingsPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.cache/ukui-menu/ukui-menu.ini";
            QSettings settings(settingsPath, QSettings::IniFormat);
            auto g = settings.childGroups();
            auto k = settings.allKeys();
            settings.setIniCodec(QTextCodec::codecForName("utf-8"));
            settings.beginGroup("application");
            bool isExist = settings.contains(execmd);
            bool notDisable = true;
            if (isExist) {
                notDisable = settings.value(execmd).toBool();
            }
            settings.endGroup();

            qDebug() << "getDefaultAction empty info cmd:"<<cmd<<isExist<<notDisable;
            if (isExist && !notDisable) {
                isMdmApp = true;
            }
        }
        FileLaunchAction *action = new FileLaunchAction(m_info->uri(), G_APP_INFO(info));
        qDebug() << "getDefaultAction isMdmApp:"<<isMdmApp;
        if (isMdmApp) {
            action->setProperty("isMdmApp", isMdmApp);
        }
        g_object_unref(info);
        action->lauchFileAsync(true);
    }
    g_key_file_free(keyfile);
    return;
}
