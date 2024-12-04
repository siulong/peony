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

#ifndef MULTISELECTCOMBOBOX_H
#define MULTISELECTCOMBOBOX_H

#include "peony-core_global.h"
#include <QObject>
#include <QComboBox>
#include <QListView>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QLineEdit>

namespace Peony {

class PEONYCORESHARED_EXPORT MultiSelectComboBoxDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit MultiSelectComboBoxDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class PEONYCORESHARED_EXPORT MultiSelectLineEdit :  public QLineEdit
{
    Q_OBJECT
public:
    explicit MultiSelectLineEdit(QWidget *parent = nullptr);
    ~MultiSelectLineEdit();

    void setItems(const QList<QPair<QString, QColor>> &items);
    int calculateContentWidth() const;
    void updateWidth();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QList<QPair<QString, QColor>> m_items; // 存储选项及其颜色
    int m_radius = 12;
};

class PEONYCORESHARED_EXPORT MultiSelectComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit MultiSelectComboBox(QWidget *parent = nullptr);
    ~MultiSelectComboBox();
    void addItem(const QString &text, bool checked = false, const int count = 0, const QColor &color = Qt::transparent);
    void addItems(const QStringList &texts);
    void removeItem(int index);
    void clear();
    QStringList getSelectItemsText();
    QList<int> getSelectItemsIndex();
    QString getItemText(int index);
    void setDefaultItem(int index);
    void setEnableItem(int index, bool enabled = true);
    QString getShowText();
    void setCheckedItem(int index, bool checked = false);
    void uncheckAllItem();
    void setPlaceholderText(const QString &placeholderText);
    void setCountFromItemText(const QString &text, int count);
    void setCountFromItemIndex(int index, int count);

Q_SIGNALS:
    void showingPopup();
    void hidingPopup();
    void currentItemStateChanged();

protected:
    void showPopup();
    void hidePopup();
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual bool eventFilter(QObject *watched, QEvent *event);

private:
    void updateText();

private Q_SLOTS:
    void slotActivated(int index);
    void updateMultiComboBoxItemWidth();

private:
    MultiSelectLineEdit *m_lineEdit;
    QListView *m_listView;
    QStandardItemModel m_model;
};

}
#endif // MULTISELECTCOMBOBOX_H
