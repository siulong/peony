#ifndef FORMAT_DLG_CREATE_DELEGATE_H
#define FORMAT_DLG_CREATE_DELEGATE_H

#include <QObject>
#include <QMap>
#include <QDialog>
#include "peony-core_global.h"

class Format_Dialog;
namespace Peony {
    class SideBarAbstractItem;
}

#ifdef KY_UDF_BURN
namespace UdfBurn {
    class UdfFormatDialog;
    class DiscControl;
    class UdfFormatDialogWrapper;
}
#else
class UdfFormatDialog;
class DiscControl;
#endif

class PEONYCORESHARED_EXPORT FormatDlgCreateDelegate : public QObject
{
    Q_OBJECT
public:
    static FormatDlgCreateDelegate *getInstance();
    Format_Dialog *createUDiskDlg(const QString &uris, Peony::SideBarAbstractItem *item, QWidget *parent = nullptr);
    void removeFromUdiskMap(QString &uris);
#ifndef KY_UDF_BURN
    UdfFormatDialog *createUdfDlg(const QString &uris, DiscControl *discControl, QWidget *parent = nullptr);
    void removeFromUdfMap(QString &uris);
#else
    UdfBurn::UdfFormatDialogWrapper *createUdfDlgWrapper(const QString &uris, UdfBurn::DiscControl *discControl, QWidget *parent = nullptr);
    void removeFromUdfWrapperMap(QString &uris);
#endif
private:
    explicit FormatDlgCreateDelegate(QObject *parent = nullptr);
    ~FormatDlgCreateDelegate();

private:
    QMap<QString, Format_Dialog *> m_udiskDlgMap;
#ifndef KY_UDF_BURN
    QMap<QString, UdfFormatDialog *> m_udfDlgMap;
#else
    QMap<QString, UdfBurn::UdfFormatDialogWrapper *> m_udfDlgWrapperMap;
#endif
};

#endif // FORMAT_DLG_CREATE_DELEGATE_H
