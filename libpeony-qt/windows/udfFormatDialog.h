#ifndef UDFFORMATDIALOG_H
#define UDFFORMATDIALOG_H

#include <QDialog>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QProgressBar>
#include <QGridLayout>
#include <QCloseEvent>

#include "peony-core_global.h"

class DiscControl;
class QThread;

class PEONYCORESHARED_EXPORT  UdfFormatDialog : public QDialog
{
    Q_OBJECT
public:
    UdfFormatDialog(const QString &uri, DiscControl *discControl,QWidget *parent = nullptr);
    virtual ~UdfFormatDialog();

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

public Q_SLOTS:
    void slot_udfFormat();
    void slot_udfCancel();
    void slot_formatFinished(bool successful, QString errorInfo);

protected:
    void closeEvent(QCloseEvent *) override;

private:
    bool udfFormatEnsureMsgBox();
    void setButtonState(bool state);
};

#endif // UDFFORMATDIALOG_H
