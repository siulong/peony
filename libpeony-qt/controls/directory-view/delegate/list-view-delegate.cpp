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

#include "list-view-delegate.h"
#include "file-operation-manager.h"
#include "file-rename-operation.h"
#include "file-item-model.h"

#include "list-view.h"
#include "clipboard-utils.h"

#include "file-info.h"
#include "file-info-job.h"
#include "emblem-provider.h"

#include <QTimer>
#include <QPushButton>

#include <QPainter>

#include <QKeyEvent>
#include <QItemDelegate>
#include <file-label-model.h>
#include <QHeaderView>
#include <QApplication>

using namespace Peony;

ListViewDelegate::ListViewDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
    m_styled_button = new QPushButton;
}

ListViewDelegate::~ListViewDelegate()
{
    m_styled_button->deleteLater();
}

void ListViewDelegate::initIndexOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
    return initStyleOption(option, index);
}

void ListViewDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    opt.displayAlignment = Qt::Alignment(Qt::AlignLeft|Qt::AlignVCenter);

    auto view = qobject_cast<DirectoryView::ListView *>(parent());
    opt.decorationSize = view->iconSize();
    /* 此处以中文命名的文件保护箱标记实时同步还存在问题，是由于uri编码（尽管使用FileUtils::urlEncoded进行转换）与底层(info的uri)不匹配 */
    QString uri = index.data(Qt::UserRole).toString();
    auto info = FileInfo::fromUri(uri);
    auto colors = info->getColors();
    if(uri.startsWith("favorite://")){/* 快速访问须特殊处理 */
        //快速访问目录，颜色标记设置后更新不及时问题单独处理,修复bug#118015
        uri =FileUtils::getEncodedUri(FileUtils::getTargetUri(uri));
        auto matchInfo = FileInfo::fromUri(uri);
        colors = matchInfo->getColors();
    }
    auto rect = view->visualRect(index);
    if (index.column() == 0 && colors.count() >0) {
        if (!view->isDragging() || !view->selectionModel()->selectedIndexes().contains(index)) {
            //修改标记个数最多为3个，以及标记位置
            const int MAX_LABEL_NUM = 3;
            const int LABEL_SIZE = 12;
            int startIndex = (colors.count() > MAX_LABEL_NUM ? colors.count() - MAX_LABEL_NUM : 0);
            int num =  colors.count() - startIndex;
            auto lineSpacing = option.fontMetrics.lineSpacing();

            int xOffSet = rect.topRight().x() - LABEL_SIZE/2 - 20;
            int yOffSet = rect.height()/2 - LABEL_SIZE/2;
            int width = rect.width();
            if(num > 0){
                //bug#94242 修改标记位置后和名称重叠，设置标记位置的背景颜色
                QRect markRect = opt.rect;
                markRect.setLeft(rect.width() - (num+1)*LABEL_SIZE/2 );
                bool isHover = (opt.state & QStyle::State_MouseOver) && (opt.state & ~QStyle::State_Selected);
                bool isSelected = opt.state & QStyle::State_Selected;
                bool enable = opt.state & QStyle::State_Enabled;
                QColor color = opt.palette.color(enable? QPalette::Active: QPalette::Disabled,
                                                     QPalette::Highlight);

                if (isSelected) {
                    color.setAlpha(255);
                } else if (isHover) {
                    color = opt.palette.color(QPalette::Active, QPalette::BrightText);
                    color.setAlphaF(0.05);
                } else {
                    color.setAlpha(0);
                }

                painter->save();
                painter->fillRect(markRect, color);
                painter->restore();
                width = width - (num+1)*LABEL_SIZE/2 - 20;
            }
            for (int i = startIndex; i < colors.count(); ++i) {
                auto color = colors.at(i);
                painter->save();
                painter->setRenderHint(QPainter::Antialiasing);
                painter->translate(0, opt.rect.topLeft().y());
                painter->translate(2, 2);
                painter->setPen(opt.palette.highlightedText().color());
                painter->setBrush(color);
                painter->drawEllipse(QRectF(xOffSet, yOffSet, LABEL_SIZE, LABEL_SIZE));
                painter->restore();

                xOffSet -= LABEL_SIZE/2;
            }
            //bug#94242 修改标记位置后和名称重叠，设置汉字宽度
            opt.rect.setWidth(width);
        }
    }

    if (ClipboardUtils::isClipboardHasFiles() &&
        FileUtils::isSamePath(ClipboardUtils::getClipedFilesParentUri(), view->getDirectoryUri())) {
        if (ClipboardUtils::isPeonyFilesBeCut() && ClipboardUtils::isClipboardFilesBeCut()) {
            auto clipedUris = ClipboardUtils::getClipboardFilesUris();
            if (clipedUris.contains(FileUtils::urlEncode(index.data(Qt::UserRole).toString()))) {
                painter->setOpacity(0.5);
                qDebug()<<"cut item in list view"<<index.data();
            }
            else
                painter->setOpacity(1.0);
        }
    }
    else
       painter->setOpacity(1.0);
    if (!opt.state.testFlag(QStyle::State_Selected)) {
        if (opt.state & QStyle::State_Sunken) {
            opt.palette.setColor(QPalette::Highlight, opt.palette.button().color());
        }
        if (opt.state & QStyle::State_MouseOver) {
            opt.palette.setColor(QPalette::Highlight, opt.palette.mid().color());
        }
    }

    if (index.column() == 0 && !m_regFindKeyWords.isEmpty()) {
        QString text1 = opt.text;
        opt.text = QString();
        opt.widget->style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget); //绘制非文本区域内容
        opt.text = text1;
        painter->save();

        QString text = opt.text;
        QFont font = opt.font;
        QFontMetrics fontMetrics = opt.fontMetrics;
        int lineSpacing = fontMetrics.lineSpacing();
        //计算text的长度
        QRect textRect = opt.widget->style()->subElementRect(QStyle::SE_ItemViewItemText, &opt, opt.widget);
        QString elidedText = fontMetrics.elidedText(opt.text, Qt::ElideRight, textRect.width());
        painter->translate(textRect.topLeft());
        int yoffset = (textRect.height() - lineSpacing)/2;
        painter->translate(0, yoffset);
        textRect.moveTo(0,0);

        painter->setPen(opt.palette.highlightedText().color());
        QTextOption textOpt;
        textOpt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        textOpt.setAlignment(Qt::AlignVCenter|Qt::AlignLeft);

        QTextDocument document;
        document.setDefaultTextOption(textOpt);
        document.setTextWidth(textRect.width());
        document.setDefaultFont(font);
        document.setIndentWidth(0);
        document.setDocumentMargin(0);
        //此处应该设置elidled text
        document.setPlainText(elidedText);

        QTextCursor highlightCursor(&document);
        QTextCursor cursor(&document);
        cursor.beginEditBlock();
        QTextCharFormat plainFormat(highlightCursor.charFormat());
        QTextCharFormat colorFormat = plainFormat;
        colorFormat.setBackground(QBrush(QColor(95,208,101)));
        if (opt.state.testFlag(QStyle::State_Selected)) {
            QTextCharFormat selectColorFormat(cursor.charFormat());
            selectColorFormat.setForeground(Qt::white);
            cursor.select(QTextCursor::Document);
            cursor.mergeCharFormat(selectColorFormat);
        }

        while (!highlightCursor.isNull() && !highlightCursor.atEnd()) {
            highlightCursor = document.find(m_regFindKeyWords, highlightCursor);
            if (!highlightCursor.isNull()) {
                highlightCursor.mergeCharFormat(colorFormat);
            }
        }

        cursor.endEditBlock();
        document.drawContents(painter, textRect);
        painter->restore();
    } else {
        opt.widget->style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);
    }
    if(view->isEnableMultiSelect()) {
        int selectBox = 0;
        //get current checkbox positon and draw them.
        selectBox = view->getCurrentCheckboxColumn();
        int selectBoxPosion = view->viewport()->width()+view->viewport()->x()-view->header()->sectionViewportPosition(selectBox)-48;
        if(index.column() == selectBox)
        {
            if(view->selectionModel()->selectedIndexes().contains(index))
            {
                QIcon icon = QIcon(":/icons/icon-selected.png");
                icon.paint(painter, rect.x()+selectBoxPosion, rect.y()+rect.height()/2-8, 16, 16, Qt::AlignCenter);
            }
            else
            {
                QIcon icon = QIcon(":/icons/icon-select.png");
                icon.paint(painter, rect.x()+selectBoxPosion, rect.y()+rect.height()/2-8, 16, 16, Qt::AlignCenter);
            }
        }
    }
    QList<int> emblemPoses = {4, 3, 2, 1}; //bottom right, bottom left, top right, top left

    //add link and read only icon support
    if (index.column() == 0) {
        auto rect = view->visualRect(index);
        auto iconSize = view->iconSize();
        auto size = iconSize.width()/2;
        bool isSymbolicLink = info->isSymbolLink();
        auto loc_x = rect.x() + iconSize.width() - size/2;
        auto loc_y = rect.y();
        auto iconSizeHeight = iconSize.height();
        //paint symbolic link emblems
        if (isSymbolicLink) {
            emblemPoses.removeOne(3);
            QIcon icon = QIcon::fromTheme("emblem-link-symbolic");
            //qDebug()<<info->symbolicIconName();
            //icon.paint(painter, loc_x, loc_y, size, size);
            //Adjust link emblem to topLeft.link story#8354
            loc_x = rect.x();
            //Special calculation emblems coordinates
            if(iconSize.height() < 28){
                iconSizeHeight = 28;
            }
            icon.paint(painter, loc_x, loc_y + iconSizeHeight - size/2 - 5, size, size, Qt::AlignCenter);
            //painter->restore();
        }

        //paint access emblems
        //NOTE: we can not query the file attribute in smb:///(samba) and network:///.
        loc_x = rect.x();
        if (info->uri().startsWith("file:")) {
            if (!info->canRead()) {
                emblemPoses.removeOne(1);
                QIcon icon = QIcon::fromTheme("emblem-unreadable");
                icon.paint(painter, loc_x, loc_y, size, size);
            } else if (!info->canWrite()/* && !info->canExecute()*/) {
                //只读图标对应可读不可写情况，与可执行权限无关，link to bug#99998
                emblemPoses.removeOne(1);
                QIcon icon = QIcon::fromTheme("emblem-readonly");
                icon.paint(painter, loc_x, loc_y, size, size);
            }
        }

    // paint extension emblems, FIXME: adjust layout, and implemet on indexwidget, other view.
        auto extensionsEmblems = EmblemProviderManager::getInstance()->getAllEmblemsForUri(info->uri());

        //Special calculation emblems coordinates
        if(iconSize.height() < 28){
            iconSizeHeight = 28;
        }

        for (auto extensionsEmblem : extensionsEmblems) {
            if (emblemPoses.isEmpty()) {
               break;
            }

            QIcon icon = QIcon::fromTheme(extensionsEmblem);
            if (!icon.isNull()) {
                int pos = emblemPoses.takeFirst();
                switch (pos) {
                case 1: {
                   icon.paint(painter, loc_x, loc_y, size, size, Qt::AlignCenter);
                   break;
                }
                case 2: {
                   icon.paint(painter, loc_x + iconSize.width() - size/2, loc_y, size, size, Qt::AlignCenter);
                   break;
                }
                case 3: {
                   icon.paint(painter, loc_x, loc_y + iconSizeHeight - size/2 - 5, size, size, Qt::AlignCenter);
                   break;
                }
                case 4: {
                   icon.paint(painter, loc_x + iconSize.width() - size/2, loc_y + iconSizeHeight - size/2 - 5, size, size, Qt::AlignCenter);
                   break;
                }
                default:
                   break;
                }
            }
        }
    }
}

QWidget *ListViewDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    TextEdit *edit = new TextEdit(parent);
    edit->setAcceptRichText(false);
    //edit->setContextMenuPolicy(Qt::CustomContextMenu);
    edit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    edit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    edit->setWordWrapMode(QTextOption::NoWrap);

//    QTimer::singleShot(1, parent, [=]() {
//        this->updateEditorGeometry(edit, option, index);
//    });

//    connect(edit, &TextEdit::textChanged, this, [=]() {
//        updateEditorGeometry(edit, option, index);
//    });

    connect(edit, &TextEdit::textChanged, this, [=]() {
        updateEditorGeometry(edit, option, index);
    });

    connect(edit, &TextEdit::finishEditRequest, this, &ListViewDelegate::slot_finishEdit);

    connect(edit, &QWidget::destroyed, this, [=]() {
        Q_EMIT isEditing(false);
    });

    return edit;
}

void ListViewDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    TextEdit *edit = qobject_cast<TextEdit *>(editor);
    if (!edit)
        return;

    Q_EMIT isEditing(true);
    edit->setText(index.data(Qt::DisplayRole).toString());
    auto cursor = edit->textCursor();
    cursor.setPosition(0, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    bool isDir = FileUtils::getFileIsFolder(index.data(Qt::UserRole).toString());
    bool isDesktopFile = index.data(Qt::UserRole).toString().endsWith(".desktop");
    bool isSoftLink = FileUtils::getFileIsSymbolicLink(index.data(Qt::UserRole).toString());
    if (!isDesktopFile && !isSoftLink && !isDir && edit->toPlainText().contains(".") && !edit->toPlainText().startsWith(".")) {
        int n = 1;
        if(index.data(Qt::DisplayRole).toString().contains(".tar.")) //ex xxx.tar.gz xxx.tar.bz2
            n = 2;
        while(n){
            cursor.movePosition(QTextCursor::WordLeft, QTextCursor::KeepAnchor, 1);
            cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 1);
            --n;
        }
    }
    //qDebug()<<cursor.anchor();
    edit->setTextCursor(cursor);
}

//void ListViewDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
//{
//    QStyledItemDelegate::updateEditorGeometry(editor, option, index);
//    TextEdit *edit = qobject_cast<TextEdit*>(editor);
//    edit->setFixedHeight(editor->height());
//    edit->resize(edit->document()->size().width(), -1);
//}

void ListViewDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    TextEdit *edit = qobject_cast<TextEdit*>(editor);
    if (!edit)
        return;

    auto text = edit->toPlainText();
    if (text.isEmpty())
        return;
    if (text == index.data(Qt::DisplayRole).toString())
        return;
    //process special name . or .. or only space
    if (text == "." || text == ".." || text.trimmed() == "")
        return;

    if (! index.isValid())
        return;

    auto view = qobject_cast<DirectoryView::ListView *>(parent());
    auto oldName = index.data(Qt::DisplayRole).toString();
    if (text == oldName)
    {
        //create new file, should select the file or folder
        auto flags = QItemSelectionModel::Select|QItemSelectionModel::Rows;
        view->selectionModel()->select(index, flags);
        view->setFocus();
        return;
    }

    auto fileOpMgr = FileOperationManager::getInstance();
    auto renameOp = new FileRenameOperation(index.data(FileItemModel::UriRole).toString(), text);

    connect(renameOp, &FileRenameOperation::operationFinished, view, [=](){
        auto info = renameOp->getOperationInfo().get();
        auto uri = info->target();
        QTimer::singleShot(100, view, [=](){
            view->setSelections(QStringList()<<uri);
            //after rename will nor sort immediately, comment to fix bug#60482
            //view->scrollToSelection(uri);
            view->setFocus();
        });
    }, Qt::BlockingQueuedConnection);

    fileOpMgr->startOperation(renameOp, true);
}

//not comment this bug to fix bug#93314
QSize ListViewDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    auto view = qobject_cast<DirectoryView::ListView *>(parent());
    int expectedHeight = view->iconSize().height() + 4;
    size.setHeight(qMax(expectedHeight, size.height()));
    return size;
}

void ListViewDelegate::slot_finishEdit()
{
    auto edit = qobject_cast<QWidget *>(sender());
    commitData(edit);
    closeEditor(edit, QAbstractItemDelegate::SubmitModelCache);
    if(edit){
        delete edit;
        edit = nullptr;
    }
}

void ListViewDelegate::setSearchKeyword(QString regFindKeyWords)
{
    m_regFindKeyWords = regFindKeyWords;
}

//TextEdit
TextEdit::TextEdit(QWidget *parent) : QTextEdit (parent)
{
    this->setContentsMargins(0,0,0,0);
}

void TextEdit::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        Q_EMIT finishEditRequest();
        return;
    }
    return QTextEdit::keyPressEvent(e);
}
