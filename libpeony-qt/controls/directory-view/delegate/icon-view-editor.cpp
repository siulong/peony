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

#include <QPaintEvent>
#include <QPainter>
#include <QPolygon>
#include <QLineEdit>
#include <QAbstractItemView>

#include "icon-view-editor.h"

using namespace Peony;
using namespace DirectoryView;

IconViewEditor::IconViewEditor(QWidget *parent) : QTextEdit(parent)
{
    setAcceptRichText(false);
    //setContextMenuPolicy(Qt::CustomContextMenu);
    m_styled_edit = new QLineEdit;
    setContentsMargins(0, 0, 0, 0);
    setAlignment(Qt::AlignCenter);
    // fix #164278, icon view text editor doesn't cover view item.
    // note on ukui platform theme, style panel frame is not visible,
    // we have to draw a frame by ourselves.
    setFrameShape(QFrame::NoFrame);

//    setStyleSheet("padding: 0px;"
//                  "background-color: white;");

    connect(this, &IconViewEditor::textChanged, this, &IconViewEditor::minimalAdjust);
}

IconViewEditor::~IconViewEditor()
{
    m_styled_edit->deleteLater();
}

void IconViewEditor::paintEvent(QPaintEvent *e)
{
    QPainter p(this->viewport());
    p.fillRect(this->viewport()->rect(), m_styled_edit->palette().base());
    QPen pen;
    pen.setWidth(3);
    pen.setColor(this->palette().highlight().color());
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.drawRect(this->viewport()->rect());
    QTextEdit::paintEvent(e);
}

void IconViewEditor::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        Q_EMIT returnPressed();
        return;
    }
    QTextEdit::keyPressEvent(e);
}

void IconViewEditor::minimalAdjust()
{
    if (m_max_length_limit) {
        //fix #154584
        blockSignals(true);
        auto privousText = toPlainText();
        auto currentText = privousText;
        auto position = textCursor().position();
        bool needReset = false;
        while (true) {
            if (m_limit_bytes) {
                auto local8Bit = currentText.toLocal8Bit();
                if (local8Bit.length() <= m_max_length_limit) {
                    break;
                }
            } else {
                if (currentText.length() <= m_max_length_limit) {
                    break;
                }
            }

            if (position > 0) {
                position--;
                currentText.remove(position, 1);
            } else {
                currentText.remove(0, 1);
            }
            needReset = true;
        }
        if (needReset) {
            setText(currentText);
            auto currentTextCursor = textCursor();
            currentTextCursor.setPosition(position);
            setTextCursor(currentTextCursor);
        }
        blockSignals(false);
    }

    this->resize(QSize(document()->size().width(), document()->size().height() + 24));
}

void IconViewEditor::setMaxLengthLimit(int length)
{
    m_max_length_limit = length;
}

void IconViewEditor::setLimitBytes(bool limitBytes)
{
    m_limit_bytes = limitBytes;
}
