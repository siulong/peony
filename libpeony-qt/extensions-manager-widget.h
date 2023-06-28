#ifndef EXTENSIONSMANAGERWIDGET_H
#define EXTENSIONSMANAGERWIDGET_H

#include <QObject>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include "plugin-iface.h"
#include "peony-core_global.h"

#define EXTENSIONS_SHOW_WIDTH   537
#define EXTENSIONS_SHOW_HEIGHT  860
#define DISABLED_EXTENSIONS         "disabledExtensions"

namespace Peony {

class PEONYCORESHARED_EXPORT ExtensionsManagerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ExtensionsManagerWidget(QWidget *parent = nullptr);
    ~ExtensionsManagerWidget();
    void initUI();
    void initTableWidget();
    void initExtensionInfo();
    bool updateCheckBox(const QString &path);
    void addSeparator();

private:
    QVBoxLayout *m_mainLayout = nullptr;
    QTableWidget *m_tableWidget = nullptr;
    QLabel *m_useLabel = nullptr;
    QPushButton *m_okBtn = nullptr;
    QPushButton *m_cancelBtn = nullptr;

    QMap<QString, PluginInterface*> m_pluginMap;
    QStringList m_disabledList;
};
}
#endif // EXTENSIONSMANAGERWIDGET_H
