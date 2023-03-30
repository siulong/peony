#ifndef RENAMEEDITOR_H
#define RENAMEEDITOR_H

#include <QTextEdit>

class RenameEditor: public QTextEdit
{
    Q_OBJECT
public:
    explicit RenameEditor(QWidget *parent = nullptr);

Q_SIGNALS:
    void returnPressed();

protected:

    void keyPressEvent(QKeyEvent *e) override;

};

#endif // RENAMEEDITOR_H
