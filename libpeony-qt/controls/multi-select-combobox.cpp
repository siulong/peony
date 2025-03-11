/*
 * Copyright (C) 2024, KylinSoft Co., Ltd.
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
 * Authors: Wenjie Xiang <xiangwenjie@kylinos.cn>
 *
 */

#include "multi-select-combobox.h"
#include <QLineEdit>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QApplication>
#include <QDebug>
#include <QPainterPath>

using namespace Peony;
MultiSelectComboBox::MultiSelectComboBox(QWidget *parent)
    : QComboBox(parent)
{
    m_lineEdit = new MultiSelectLineEdit(this);
    m_lineEdit->setReadOnly(true);
    this->setLineEdit(m_lineEdit);
    this->lineEdit()->disconnect();
    m_lineEdit->installEventFilter(this);

    m_listView = new QListView(this);
    this->setView(m_listView);
    this->setModel(&m_model);
    m_listView->setItemDelegate(new MultiSelectComboBoxDelegate(this));

    connect(this, QOverload<int>::of(&MultiSelectComboBox::activated), this, &MultiSelectComboBox::slotActivated);
}

MultiSelectComboBox::~MultiSelectComboBox()
{

}

void MultiSelectComboBox::addItem(const QString &text, bool checked, const int count, const QColor &color)
{
    QStandardItem *item = new QStandardItem(text);
    item->setCheckable(true);
    item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
    item->setData(count, Qt::UserRole + 1);
    item->setData(color, Qt::UserRole + 2);
    m_model.appendRow(item);

    this->updateText();
    this->updateMultiComboBoxItemWidth();
}

void MultiSelectComboBox::addItems(const QStringList &texts)
{
    for (auto text : texts) {
        this->addItem(text);
    }
    this->updateMultiComboBoxItemWidth();
}

void MultiSelectComboBox::removeItem(int index)
{
    m_model.removeRow(index);
    this->updateText();
}

void MultiSelectComboBox::clear()
{
    m_model.clear();
    this->updateText();
}

QStringList MultiSelectComboBox::getSelectItemsText()
{
    QStringList texts;
    for (int i = 0; i < m_model.rowCount(); i++) {
        QStandardItem *item = m_model.item(i);
        if (item->checkState() == Qt::Checked) {
            texts.append(item->text());
        }
    }
    return texts;
}

QList<int> MultiSelectComboBox::getSelectItemsIndex()
{
    QList<int> indexs;
    for (int i = 0; i < m_model.rowCount(); i++) {
        QStandardItem *item = m_model.item(i);
        if (item->checkState() == Qt::Checked) {
            indexs.append(i);
        }
    }
    return indexs;
}

QString MultiSelectComboBox::getItemText(int index)
{
    if (index < 0 || index >= m_model.rowCount()) {
        return QString("");
    }

    return m_model.item(index)->text();
}

void MultiSelectComboBox::setDefaultItem(int index)
{
    if (index < 0 || index >= m_model.rowCount()) {
        return;
    }

    QStandardItem *item = m_model.item(index);
    item->setCheckState(Qt::Checked);
    this->updateText();
}

void MultiSelectComboBox::setEnableItem(int index, bool enabled)
{
    if (index < 0 || index >= m_model.rowCount()) {
        return;
    }

    QStandardItem *item = m_model.item(index);
    item->setEnabled(enabled);
    this->updateText();
}

QString MultiSelectComboBox::getShowText()
{
    return m_lineEdit->text();
}

void MultiSelectComboBox::setCheckedItem(int index, bool checked)
{
    if (index < 0 || index >= m_model.rowCount()) {
        return;
    }

    QStandardItem *item = m_model.item(index);
    item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
    this->updateText();
}

void MultiSelectComboBox::uncheckAllItem()
{
    for (int i = 0; i < m_model.rowCount(); i++) {
        this->setCheckedItem(i);
    }
}

void MultiSelectComboBox::setPlaceholderText(const QString &placeholderText)
{
    m_lineEdit->setPlaceholderText(placeholderText);
    m_lineEdit->updateWidth();
}

void MultiSelectComboBox::setCountFromItemText(const QString &text, int count)
{
    for(int i = 0; i < m_model.rowCount(); i++) {
        QStandardItem *item = m_model.item(i);
        if (item->text() == text) {
            item->setData(count, Qt::UserRole + 1);
            break;
        }
    }
}

void MultiSelectComboBox::setCountFromItemIndex(int index, int count)
{
    QStandardItem *item = m_model.item(index);
    if (item) {
        item->setData(count, Qt::UserRole + 1);
    }
}

void MultiSelectComboBox::showPopup()
{
    Q_EMIT showingPopup();
    QComboBox::showPopup();
}

void MultiSelectComboBox::hidePopup()
{
    QRect rectView = this->view()->geometry();
    rectView.moveTopLeft(this->view()->mapToGlobal(QPoint(0, 0)));

    QPoint globalPos = QCursor::pos();

    if (!rectView.contains(globalPos)) {
        Q_EMIT hidingPopup();
        QComboBox::hidePopup();
    }
}

void MultiSelectComboBox::mousePressEvent(QMouseEvent *event)
{
    QComboBox::mousePressEvent(event);
    event->accept();
}

void MultiSelectComboBox::mouseReleaseEvent(QMouseEvent *event)
{
    QComboBox::mouseReleaseEvent(event);
    event->accept();
}

void Peony::MultiSelectComboBox::mouseMoveEvent(QMouseEvent *event)
{
    QComboBox::mouseMoveEvent(event);
    event->accept();
}

bool MultiSelectComboBox::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_lineEdit && event->type() == QEvent::MouseButtonRelease) {
        showPopup();
        return true;
    }
    return false;
}

void MultiSelectComboBox::updateText()
{
    QList<QPair<QString, QColor>> items;
    for (int i = 0; i < m_model.rowCount(); i++) {
        QStandardItem *item = m_model.item(i);
        if (item->checkState() == Qt::Checked) {
            QString text = item->text();
            QColor color = item->data(Qt::UserRole + 2).value<QColor>();
            items.append(qMakePair(text, color));
        }
    }

    // 将选中项及颜色传递给自定义 LineEdit
    dynamic_cast<MultiSelectLineEdit *>(m_lineEdit)->setItems(items);
}

void MultiSelectComboBox::slotActivated(int index)
{
    QStandardItem *item = m_model.item(index);
    if (nullptr == item) {
        return;
    }

    Qt::CheckState state = (item->checkState() == Qt::Checked) ? Qt::Unchecked : Qt::Checked;
    item->setCheckState(state);

    this->updateText();
    Q_EMIT currentItemStateChanged();
}

void MultiSelectComboBox::updateMultiComboBoxItemWidth()
{
    int maximumWidth = 0;
    for (int i = 0; i < m_model.rowCount(); i++) {
        QStandardItem *item = m_model.item(i);
        QString text = item->text();
        QColor color = item->data(Qt::UserRole + 2).value<QColor>();
        int offset = (color.isValid() && color != Qt::transparent)  ? 25 : 5;
        //加上间距
        offset += 10;
        offset += fontMetrics().horizontalAdvance(text);
        //加上后面数字宽度
        offset += 80;
        if (maximumWidth == 0 || offset > maximumWidth) {
            maximumWidth = offset;
        }
    }

    m_listView->setMinimumWidth(maximumWidth);
}

MultiSelectComboBoxDelegate::MultiSelectComboBoxDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{

}

void MultiSelectComboBoxDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();

    // 获取数据
    QString text = index.data(Qt::DisplayRole).toString();
    int count = index.data(Qt::UserRole + 1).toInt();
    QColor color = index.data(Qt::UserRole + 2).value<QColor>();
    bool isChecked = index.data(Qt::CheckStateRole).toInt() == Qt::Checked;
    QRect rect = option.rect;
    int checkIconWidth = 16;
    int circleDiameter = 12;
    int countTextWidth = 60;

    // 绘制选中项背景为圆角矩形
    if (option.state & QStyle::State_Selected) {
        QPainterPath path;
        path.addRoundedRect(rect.adjusted(2, 2, -2, -2), 6, 6);  // 圆角矩形
        painter->fillPath(path, option.palette.highlight());
    }

    // 绘制勾选图标
    if (isChecked) {
        QRect checkRect(rect.left() + 5, rect.top() + (rect.height() - checkIconWidth) / 2, checkIconWidth, checkIconWidth);  // 勾选图标区域
        QPixmap checkIcon = QApplication::style()->standardPixmap(QStyle::SP_DialogApplyButton);
        painter->drawPixmap(checkRect, checkIcon);
    }

    int offset = (color.isValid() && color != Qt::transparent)  ? 25 : 5;
    QRect circleRect(rect.left() + offset, rect.top() + (rect.height() - circleDiameter) / 2, circleDiameter, circleDiameter);
    painter->setBrush(color);
    painter->setPen(Qt::NoPen);
    if (color.isValid())
        painter->drawEllipse(circleRect);

    // 绘制文字
    int textX = circleRect.right() + 10;  // 文字起始位置
    int textWidth = rect.width() - textX - countTextWidth;  // 文字区域宽度
    QRect textRect(textX, rect.top(), textWidth, rect.height());
    painter->setPen(option.palette.text().color());
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text);

    // 绘制数量
    QRect countRect(rect.right() - countTextWidth - 10, rect.top(), countTextWidth, rect.height());
    painter->setPen(QColor(0xA6A6A6));
    painter->drawText(countRect, Qt::AlignVCenter | Qt::AlignRight, QString::number(count));

    painter->restore();
}

QSize MultiSelectComboBoxDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(36);
    return size;
}

MultiSelectLineEdit::MultiSelectLineEdit(QWidget *parent)
    : QLineEdit(parent), m_items()
{

}

MultiSelectLineEdit::~MultiSelectLineEdit()
{

}

void MultiSelectLineEdit::setItems(const QList<QPair<QString, QColor> > &items)
{
    m_items = items;
    updateWidth();
}

int MultiSelectLineEdit::calculateContentWidth() const
{
    int width = 5;
    QFontMetrics metrics(font());

    if (m_items.isEmpty()) {
        width += metrics.horizontalAdvance(this->placeholderText()) + 10;
    } else {
        for(int i = 0; i < m_items.size(); i++) {
            if (m_items[i].second.isValid() && m_items[i].second != Qt::transparent) {
                width += m_radius + 5;
            }
            width += metrics.horizontalAdvance(m_items[i].first) + 5;

            const QColor &color = m_items[i].second;
            bool isValid = (color.isValid() && color != Qt::transparent);
            if (i != m_items.size() - 1) {
                QString separator = isValid ? " " : ", ";
                width += fontMetrics().horizontalAdvance(separator);
            }
        }
    }

    return width;
}

void MultiSelectLineEdit::updateWidth()
{
    int width = calculateContentWidth();
    setFixedWidth(width + 10);
    update();

    if (parentWidget()) {
        parentWidget()->setFixedWidth(width + 35);
    }
}

void MultiSelectLineEdit::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int x = 5; // 起始位置
    int y = (height() - fontMetrics().height()) / 2; // 垂直居中

    for (int i = 0; i < m_items.size(); i++) {
        const QString &text = m_items[i].first;
        const QColor &color = m_items[i].second;

        bool isValid = (color.isValid() && color != Qt::transparent);
        if (isValid) {
            int topY = (height() - m_radius) / 2;
            painter.setBrush(color);
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(x, topY, m_radius, m_radius);
            x += m_radius + 5; // 移动位置
        }

        // 绘制文字
        painter.setPen(palette().text().color());
        painter.drawText(x, y + fontMetrics().ascent(), text);

        x += fontMetrics().horizontalAdvance(text) + 5; // 文字宽度 + 间隔

        if (i != m_items.size() - 1) {
            QString separator = isValid ? " " : ", ";
            painter.drawText(x, y + fontMetrics().ascent(), separator);
            x += fontMetrics().horizontalAdvance(separator);
        }
    }

    if (m_items.isEmpty()) {
        if (!this->placeholderText().isEmpty()) {
            QString text = this->placeholderText();
            painter.setPen(palette().placeholderText().color());
            painter.drawText(x, y + fontMetrics().ascent(), text);
        }
    }
}
