#ifdef KY_FILE_DIALOG

#include "kyfileoperationdialog.h"

KyFileOperationDialog::KyFileOperationDialog(QWidget *parent) : kdk::KDialog(parent)
{
    setWindowIcon(nullptr);
    minimumButton()->setVisible(true);
    setFixedWidth(536);
    setFixedHeight(192);
}

#endif
