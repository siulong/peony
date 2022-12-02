#include "rename-editor.h"
#include <QKeyEvent>

RenameEditor::RenameEditor(QWidget *parent) : QTextEdit(parent)
{
}

void RenameEditor::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        Q_EMIT returnPressed();
        return;
    }
    QTextEdit::keyPressEvent(e);
}
