/*
 * Peony-Qt's Library
 *
 * Copyright (C) 2019, Tianjin KYLIN Information Technology Co., Ltd.
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
 * Authors: Jing Ding <dingjing@kylinos.cn>
 *
 */
#include "file-operation-error-dialog-base.h"
#include "xatom-helper.h"

#include <QIcon>
#include <QPainter>
#include <QPainter>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QProxyStyle>
#include <QPushButton>
#include <QApplication>

Peony::FileOperationErrorDialogBase::FileOperationErrorDialogBase(QDialog *parent) : QDialog(parent)
{
    setFixedSize (536, 192);
    setAutoFillBackground (true);
    setBackgroundRole (QPalette::Base);

    MotifWmHints hints;
    hints.flags = MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS;
    hints.functions = MWM_FUNC_ALL;
    hints.decorations = MWM_DECOR_BORDER;
    XAtomHelper::getInstance()->setWindowMotifHint(winId(), hints);

    QVBoxLayout* mainLayout = new QVBoxLayout (this);
    mainLayout->setContentsMargins (16, 3, 8, 16);

    QHBoxLayout* headerLayout = new QHBoxLayout;

//    QPushButton* minilize = new QPushButton;
    QPushButton* closebtn = new QPushButton;

//    minilize->setFlat (true);
//    minilize->setFixedSize (36, 36);
//    minilize->setProperty ("isWindowButton", 0x01);
//    minilize->setIconSize (QSize(16, 16));
//    minilize->setIcon (QIcon::fromTheme ("window-minimize-symbolic"));

    closebtn->setFlat (true);
    closebtn->setFixedSize (32, 32);
    closebtn->setProperty ("isWindowButton", 0x02);
    closebtn->setIconSize (QSize(16, 16));
    closebtn->setIcon (QIcon::fromTheme("window-close-symbolic"));

    headerLayout->addStretch ();
//    headerLayout->setSpacing (1);
//    headerLayout->addWidget (minilize);
    headerLayout->addWidget (closebtn);

    mainLayout->addLayout (headerLayout);

    QHBoxLayout* contentLayout = new QHBoxLayout;
    contentLayout->setContentsMargins (6, 0, 0, 0);
    contentLayout->setAlignment (Qt::AlignTop | Qt::AlignLeft);

    m_tipimage = new QLabel(this);
    m_tipimage->setMargin (0);
    m_tipimage->setFixedWidth(64);
    m_tipimage->setAlignment (Qt::AlignTop);
    m_tipimage->setPixmap (QIcon::fromTheme ("dialog-warning").pixmap (64, 64));
    contentLayout->addWidget (m_tipimage);

    m_tipcontent = new QLabel(this);
    m_tipcontent->setWordWrap (true);
    m_tipcontent->setFixedWidth(420);
    m_tipcontent->setAlignment (Qt::AlignLeft | Qt::AlignTop);

    QScrollArea* scroll = new QScrollArea(this);

    scroll->setFixedWidth(440);
    scroll->setWidgetResizable (true);
    scroll->setFrameShape(QFrame::NoFrame);

    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    scroll->setWidget(m_tipcontent);

    contentLayout->addWidget (scroll);

    mainLayout->addLayout (contentLayout);

    m_buttonLeft = new QHBoxLayout;
    m_buttonRight = new QHBoxLayout;
    m_buttonRight->setDirection (QHBoxLayout::RightToLeft);
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->setContentsMargins (0, 0, 13, 3);

    buttonLayout->addLayout (m_buttonLeft);
    buttonLayout->addStretch ();
    buttonLayout->addLayout (m_buttonRight);

    mainLayout->addStretch ();
    mainLayout->addLayout (buttonLayout);

    setLayout (mainLayout);

    connect (closebtn, &QPushButton::clicked, this, [=] () {
        Q_EMIT cancel ();
        done (QDialog::Rejected);
    });

    connect(this, &FileOperationErrorDialogBase::cancel, this, [=] () {
        done (QDialog::Rejected);
    });
}

Peony::FileOperationErrorDialogBase::~FileOperationErrorDialogBase()
{

}

void Peony::FileOperationErrorDialogBase::adjustTextContent()
{
    auto text = m_tipcontent->text();
    if (text.startsWith("<p>") && text.endsWith("</p>")) {
        text.remove(text.length() - 4, 4);
        text.remove(0, 3);
    }
    auto rect = fontMetrics().boundingRect(text);
    bool oneline = rect.width() < m_tipcontent->width();
    int topMargin = 0;
    if (!oneline) {
        topMargin = qMax(32 - fontMetrics().height(), 0);
    } else {
        topMargin = qMax(48 - fontMetrics().height(), 0);
    }

    m_tipcontent->setContentsMargins(0, topMargin, 0, 0);
}

void Peony::FileOperationErrorDialogBase::setText(QString text)
{
    if (!text.isNull () && !text.isEmpty ()) {
        m_tipcontent->setText (text);
        adjustTextContent();
    }
}

void Peony::FileOperationErrorDialogBase::setIcon(QString iconName)
{
    if (!iconName.isNull () && !iconName.isEmpty ()) {
        m_tipimage->setPixmap (QIcon::fromTheme (iconName).pixmap (64, 64));
    }
}

QPushButton *Peony::FileOperationErrorDialogBase::addButton(QString name)
{
    if (!name.isNull () && !name.isEmpty ()) {
        QPushButton* b = new QPushButton(name);
        m_buttonRight->addWidget (b, Qt::AlignRight | Qt::AlignVCenter);
        return b;
    }

    return nullptr;
}

QCheckBox *Peony::FileOperationErrorDialogBase::addCheckBoxLeft(QString name)
{
    if (!name.isNull () && !name.isEmpty ()) {
        QCheckBox* b = new QCheckBox(name);
        m_buttonLeft->addWidget (b, Qt::AlignLeft | Qt::AlignVCenter);
        return b;
    }

    return nullptr;
}

bool Peony::FileOperationErrorDialogBase::event(QEvent *event)
{
    if (event->type() == QEvent::FontChange || event->type() == QEvent::ApplicationFontChange) {
        adjustTextContent();
    }
    return QDialog::event(event);
}
