#ifdef KY_FILE_DIALOG

#ifndef KYFILEOPERATIONDIALOG_H
#define KYFILEOPERATIONDIALOG_H

#include <QWidget>
#include <kysdk/applications/kdialog.h>

class KyFileOperationDialog : public kdk::KDialog
{
    Q_OBJECT
public:
    explicit KyFileOperationDialog(QWidget *parent = nullptr);
};

#endif // KYFILEOPERATIONDIALOG_H

#endif
