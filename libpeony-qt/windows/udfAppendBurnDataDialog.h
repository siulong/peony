#ifndef UDFAPPENDBURNDATADIALOG_H
#define UDFAPPENDBURNDATADIALOG_H

#ifdef KY_UDF_BURN

#include <QDialog>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QProgressBar>
#include <QGridLayout>
#include <QCloseEvent>

#include "peony-core_global.h"

class QThread;

namespace UdfBurn {

class DiscControl;

class PEONYCORESHARED_EXPORT  UdfAppendBurnDataDialog : public QDialog
{
    Q_OBJECT
public:
    UdfAppendBurnDataDialog(const QString &uri, DiscControl *discControl,QWidget *parent = nullptr);
    virtual ~UdfAppendBurnDataDialog();

private:
    QString m_uri;
    DiscControl *m_discControl = nullptr;
    QThread *m_thread = nullptr;

    const int m_widgetWidth = 424;
    const int m_widgetHeight = 300;

    QLabel* m_discTypeLabel = nullptr;
    QLabel* m_discNameLabel = nullptr;

    QLineEdit* m_discTypeEdit = nullptr;
    QLineEdit* m_discNameEdit = nullptr;

    QPushButton* m_okBtn = nullptr;
    QPushButton* m_cancelBtn = nullptr;

    QProgressBar* m_progress = nullptr;
    QGridLayout* m_mainLayout = nullptr;

    QStringList  m_urisOfBurningCachedData;


public Q_SLOTS:
    void slot_udfAppendBurnData();
    void slot_udfAppendBurnDataCancel();
    void slot_appendBurnDataFinished(bool successful, QString errorInfo);

protected:
    void closeEvent(QCloseEvent *) override;

private:
    bool udfAppendBurnDataEnsureMsgBox();
    void setButtonState(bool state);
};

}

#endif

#endif // UDFAPPENDBURNDATADIALOG_H
