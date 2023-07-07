/*
 * Peony-Qt
 *
 * Copyright (C) 2023, KylinSoft Information Technology Co., Ltd.
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
 * Authors: Yue Lan <lanyue@kylinos.cn>
 *
 */

#ifdef KY_FILE_DIALOG

#include "kyfiledialogrename.h"

#include <QStackedWidget>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTextEdit>

#include <QApplication>
#include <QFontMetrics>
#include <QDBusInterface>
#include <QDBusReply>

#include "file-utils.h"
#include "rename-editor.h"

#include "global-settings.h"

KyFileDialogRename::KyFileDialogRename(QWidget *parent) : KyFileOperationDialog(parent), Peony::FileOperationErrorHandler()
{
    bool isTabletMode = false;
    m_statusManagerDBus = new QDBusInterface(DBUS_STATUS_MANAGER_IF, "/" ,DBUS_STATUS_MANAGER_IF,QDBusConnection::sessionBus(),this);
    if (m_statusManagerDBus) {
        qDebug() << "[PeonyDesktopApplication::initGSettings] init statusManagerDBus" << m_statusManagerDBus->isValid();
        if (m_statusManagerDBus->isValid()) {
            QDBusReply<bool> message = m_statusManagerDBus->call("get_current_tabletmode");
            if (message.isValid()) {
                isTabletMode = message.value();
            }
        }
    }

    setFixedWidth(600);
    if(true == isTabletMode){
        setFixedHeight(255);
    }else{
        setFixedHeight(225);
    }

}

void KyFileDialogRename::handle(Peony::FileOperationError &error)
{
    Peony::ExceptionResponse responseCode = Peony::ExceptionResponse::Cancel;
    QString newName = Peony::FileUtils::getUriBaseName(error.destDirUri);
    newName = Peony::FileUtils::urlDecode(newName);

    auto stack = new QStackedWidget(this);
    stack->setContentsMargins(20, 0, 20, 20);
    auto layout = new QHBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(stack);
    mainWidget()->setLayout(layout);

    // todo:
    // 根据错误码和文件操作码拼接文件信息
    QString line1;
    QString line2;
    QString line3;

    switch (error.op) {
    case Peony::FileOpRename: {
        line1 = tr("Renaming \"%1\"").arg(newName);
        line2 = tr("Renaming failed, the reason is: %1").arg(error.errorCode == G_IO_ERROR_FILENAME_TOO_LONG? tr("Filename too long"): error.errorStr);
        break;
    }
    case Peony::FileOpCopy:
        line1 = tr("Copying \"%1\"").arg(newName); {
        auto destDir = Peony::FileUtils::urlDecode(error.destDirUri);
        line2 = tr("To \"%1\"").arg(destDir);
        line3 = tr("Copying failed, the reason is: %1").arg(error.errorCode == G_IO_ERROR_FILENAME_TOO_LONG? tr("Filename too long"): error.errorStr);
        break;
    }
    case Peony::FileOpMove: {
        line1 = tr("Moving \"%1\"").arg(newName);
        auto destDir = Peony::FileUtils::urlDecode(error.destDirUri);
        line2 = tr("To \"%1\"").arg(destDir);
        line3 = tr("Moving failed, the reason is: %1").arg(error.errorCode == G_IO_ERROR_FILENAME_TOO_LONG? tr("Filename too long"): error.errorStr);
        break;
    }
    default: {
        line1 = tr("File operation error:");
        line2 = tr("The reason is: %1").arg(error.errorCode == G_IO_ERROR_FILENAME_TOO_LONG? tr("Filename too long"): error.errorStr);
        break;
    }
    }

    QString line;
    QStringList messages;
    line = qApp->fontMetrics().elidedText(line1, Qt::ElideMiddle, 500);
    messages.append(line);
    line = qApp->fontMetrics().elidedText(line2, Qt::ElideMiddle, 500);
    messages.append(line);
    line = qApp->fontMetrics().elidedText(line3, Qt::ElideMiddle, 500);
    messages.append(line);

    auto labelText = messages.join('\n');

    // page1
    auto page1 = new QWidget(this);
    auto gridLayout1 = new QGridLayout;
    gridLayout1->setSpacing(20);
    auto labelIcon = new QLabel;
    labelIcon->setPixmap(QIcon::fromTheme("dialog-warning").pixmap(24, 24));
    gridLayout1->addWidget(labelIcon, 0, 0, Qt::AlignTop|Qt::AlignLeft);
    auto content = new QLabel;
    //content->setWordWrap(true);
    //int textWidth = (this->width() - 40 - 40 - 24) * 3;
    //QString elidedString = qApp->fontMetrics().elidedText(error.errorStr, Qt::ElideMiddle, textWidth);
    content->setText(labelText);
    gridLayout1->addWidget(content, 0, 1,  Qt::AlignTop|Qt::AlignLeft);
    auto buttonBox = new QDialogButtonBox;
    buttonBox->setStandardButtons(QDialogButtonBox::NoButton);
    auto skip = buttonBox->addButton(tr("Skip"), QDialogButtonBox::ActionRole);
    auto skipAll = buttonBox->addButton(tr("Skip All"), QDialogButtonBox::ActionRole);
    auto cancel = buttonBox->addButton(tr("Cancel"), QDialogButtonBox::ActionRole);
    auto rename = buttonBox->addButton(tr("Rename"), QDialogButtonBox::ActionRole);
    rename->setProperty("isImportant", true);
    rename->setDefault(true);
    rename->setFocus();
    gridLayout1->addWidget(buttonBox, 1, 1, Qt::AlignBottom|Qt::AlignRight);
    page1->setLayout(gridLayout1);
    stack->addWidget(page1);

    // page2
    auto page2 = new QWidget(this);
    auto gridLayout2 = new QGridLayout;
    auto renameIcon = new QLabel;
    renameIcon->setPixmap(QIcon::fromTheme("dialog-warning").pixmap(24, 24));

    gridLayout2->addWidget(renameIcon, 0, 0, Qt::AlignVCenter|Qt::AlignLeft);
    auto label2 = new QLabel;
    label2->setText(tr("Please enter a new name"));
    gridLayout2->addWidget(label2, 0, 1, Qt::AlignVCenter|Qt::AlignLeft);
    auto textEdit = new RenameEditor;
    textEdit->setBackgroundRole(QPalette::Button);
    textEdit->setAutoFillBackground(true);
    textEdit->viewport()->setBackgroundRole(QPalette::Button);
    textEdit->viewport()->setAutoFillBackground(true);
    int height = qApp->fontMetrics().lineSpacing() * 3 + qApp->fontMetrics().descent();
    textEdit->setFixedHeight(height);
    gridLayout2->addWidget(textEdit, 1, 1, Qt::AlignTop);
    auto buttonBox2 = new QDialogButtonBox;
    buttonBox2->setStandardButtons(QDialogButtonBox::NoButton);
    auto cancel2 = buttonBox2->addButton(tr("Cancel"), QDialogButtonBox::ActionRole);
    auto ensure2 = buttonBox2->addButton(tr("OK"), QDialogButtonBox::ActionRole);
    ensure2->setDefault(true);
    gridLayout2->addWidget(buttonBox2, 2, 1, Qt::AlignBottom|Qt::AlignRight);
    page2->setLayout(gridLayout2);
    stack->addWidget(page2);

    textEdit->setText(newName);
    textEdit->selectAll();
    //textEdit->setFocus();
    auto cursor = textEdit->textCursor();
    cursor.setPosition(0, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    bool isDir = Peony::FileUtils::getFileIsFolder(textEdit->toPlainText());
    bool isDesktopFile = textEdit->toPlainText().endsWith(".desktop");
    bool isSoftLink = Peony::FileUtils::getFileIsSymbolicLink(textEdit->toPlainText());

    if (!isDesktopFile && !isSoftLink && !isDir && textEdit->toPlainText().contains(".") && !textEdit->toPlainText().startsWith(".")) {
        int n = 1;
        if(textEdit->toPlainText().contains(".tar.")) //ex xxx.tar.gz xxx.tar.bz2
            n = 2;
        while(n){
            cursor.movePosition(QTextCursor::WordLeft, QTextCursor::KeepAnchor, 1);
            cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 1);
            --n;
        }
    }

    textEdit->setTextCursor(cursor);

    gridLayout2->addWidget(textEdit, 1, 1, Qt::AlignTop);
    textEdit->activateWindow();
    //textEdit->setFocus();

    stack->setCurrentWidget(page1);
    if (QGSettings::isSchemaInstalled("org.ukui.style")) {
        QGSettings *settings = new QGSettings("org.ukui.style", QByteArray(), this);
        connect(settings, &QGSettings::changed, this, [=](const QString &key) {
            if("iconThemeName" == key){
                labelIcon->setPixmap(QIcon::fromTheme("dialog-warning").pixmap(24, 24));
                renameIcon->setPixmap(QIcon::fromTheme("dialog-warning").pixmap(24, 24));
            } else if ("systemFont" == key || "systemFontSize" == key) {
                QFontMetrics font = qApp->fontMetrics();
                int height = font.lineSpacing() * 3 + font.descent();
                textEdit->setFixedHeight(height);
                textEdit->setTextCursor(cursor);

                QStringList messages;
                QString line;
                line = qApp->fontMetrics().elidedText(line1, Qt::ElideMiddle, 500);
                messages.append(line);
                line = qApp->fontMetrics().elidedText(line2, Qt::ElideMiddle, 500);
                messages.append(line);
                line = qApp->fontMetrics().elidedText(line3, Qt::ElideMiddle, 500);
                messages.append(line);
                auto labelText = messages.join('\n');
                content->setText(labelText);
                this->repaint();
            }
        });
    }
    connect(rename, &QPushButton::clicked, this, [=]{
        setFixedHeight(300);
        stack->setCurrentWidget(page2);
        textEdit->setFocus();
    });
    connect(cancel2, &QPushButton::clicked, this, &KyFileDialogRename::reject);
    connect(ensure2, &QPushButton::clicked, this, [=, &error]{
        error.respValue.insert("newName", textEdit->toPlainText());
        error.respCode = Peony::ExceptionResponse::RenameOne;
        accept();
    });

    connect(cancel, &QPushButton::clicked, this, &KyFileDialogRename::reject);

    connect(skip, &QPushButton::clicked, this, [=, &error]{
        error.respCode = Peony::ExceptionResponse::IgnoreOne;
        accept();
    });
    connect(skipAll, &QPushButton::clicked, this, [=, &error]{
        error.respCode = Peony::ExceptionResponse::IgnoreAll;
        accept();
    });
    connect(textEdit, &RenameEditor::returnPressed, ensure2, &QPushButton::click);

    if (!exec()) {
        error.respCode = responseCode;
    }

}

#endif
