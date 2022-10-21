#ifdef KY_FILE_DIALOG

#ifndef KYFILEDIALOGRENAME_H
#define KYFILEDIALOGRENAME_H

#include "file-operation-error-handler.h"
#include "kyfileoperationdialog.h"

class KyFileDialogRename : public KyFileOperationDialog, public Peony::FileOperationErrorHandler
{
    Q_OBJECT
public:
    explicit KyFileDialogRename(QWidget *parent = nullptr);

    void handle(Peony::FileOperationError &error) override;
};

#endif // KYFILEDIALOGRENAME_H


#endif
