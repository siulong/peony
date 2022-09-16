#ifdef KY_FILE_DIALOG

#include "kyfiledialogrename.h"

#include <QStackedWidget>
#include <QGridLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTextEdit>

#include <QApplication>
#include <QFontMetrics>
#include "file-utils.h"

KyFileDialogRename::KyFileDialogRename(QWidget *parent) : KyFileOperationDialog(parent), Peony::FileOperationErrorHandler()
{
    setFixedWidth(600);
    setFixedHeight(225);
}

void KyFileDialogRename::handle(Peony::FileOperationError &error)
{
    Peony::ExceptionResponse responseCode = Peony::ExceptionResponse::Cancel;
    QString newName = Peony::FileUtils::getUriBaseName(error.destDirUri);
    newName = Peony::FileUtils::urlDecode(newName);

    auto stack = new QStackedWidget(this);
    stack->setContentsMargins(20, 0, 20, 20);
    mainLayout()->addWidget(stack);

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

    QStringList messages;
    line1 = qApp->fontMetrics().elidedText(line1, Qt::ElideMiddle, 500);
    messages.append(line1);
    line2 = qApp->fontMetrics().elidedText(line2, Qt::ElideMiddle, 500);
    messages.append(line2);
    line3 = qApp->fontMetrics().elidedText(line3, Qt::ElideMiddle, 500);
    messages.append(line3);

    auto labelText = messages.join('\n');

    // page1
    auto page1 = new QWidget(this);
    auto gridLayout1 = new QGridLayout;
    gridLayout1->setSpacing(20);
    auto labelIcon = new QLabel;
    labelIcon->setPixmap(QIcon::fromTheme("dialog-warning").pixmap(24, 24));
    gridLayout1->addWidget(labelIcon, 0, 0, Qt::AlignTop|Qt::AlignLeft);
    auto content = new QLabel;
    content->setWordWrap(true);
    //int textWidth = (this->width() - 40 - 40 - 24) * 3;
    //QString elidedString = qApp->fontMetrics().elidedText(error.errorStr, Qt::ElideMiddle, textWidth);
    content->setText(labelText);
    gridLayout1->addWidget(content, 0, 1, Qt::AlignTop);
    auto buttonBox = new QDialogButtonBox;
    buttonBox->setStandardButtons(QDialogButtonBox::NoButton);
    auto cancel = buttonBox->addButton(tr("Cancel"), QDialogButtonBox::ActionRole);
    auto skip = buttonBox->addButton(tr("Skip"), QDialogButtonBox::ActionRole);
    auto skipAll = buttonBox->addButton(tr("Skip All"), QDialogButtonBox::ActionRole);
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
    labelIcon = new QLabel;
    labelIcon->setPixmap(QIcon::fromTheme("dialog-warning").pixmap(24, 24));
    gridLayout2->addWidget(labelIcon, 0, 0, Qt::AlignTop|Qt::AlignLeft);
    auto label2 = new QLabel;
    label2->setText(tr("Please enter a new name"));
    gridLayout2->addWidget(label2, 0, 1, Qt::AlignLeft);
    auto textEdit = new QTextEdit;
    textEdit->setBackgroundRole(QPalette::Button);
    textEdit->setAutoFillBackground(true);
    textEdit->viewport()->setBackgroundRole(QPalette::Button);
    textEdit->viewport()->setAutoFillBackground(true);
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

    stack->setCurrentWidget(page1);
    connect(rename, &QPushButton::clicked, this, [=]{
        setFixedHeight(300);
        stack->setCurrentWidget(page2);
        ensure2->setFocus();
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

    if (!exec()) {
        error.respCode = responseCode;
    }
}

#endif
