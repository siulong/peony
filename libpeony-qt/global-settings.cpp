/*
 * Peony-Qt's Library
 *
 * Copyright (C) 2020, KylinSoft Co., Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Authors: Yue Lan <lanyue@kylinos.cn>
 *
 */

#include "global-settings.h"
//ukui-interface
#ifdef KYLIN_COMMON
#include <ukuisdk/kylin-com4cxx.h>
#endif

#include <QtConcurrent>

#include <QGSettings>

#include <QApplication>
#include <QPalette>
#include <QScreen>
#ifdef KY_SDK_DATE
#include <kysdk/kysdk-system/libkydate.h>
#endif

#ifdef KY_SDK_SYSINFO
#include <kysdk/kysdk-system/libkysysinfo.h>
#endif

using namespace Peony;

static GlobalSettings *global_instance = nullptr;

GlobalSettings *GlobalSettings::getInstance()
{
    if (!global_instance) {
        global_instance = new GlobalSettings;
    }
    return global_instance;
}

GlobalSettings::GlobalSettings(QObject *parent) : QObject(parent)
{
    m_settings = new QSettings("org.ukui", "peony-qt-preferences", this);
    m_peonyGSettings = new QGSettings("org.ukui.peony.settings", "/org/ukui/peony/settings/", this);
    //set default allow parallel
    if (! m_settings->allKeys().contains(ALLOW_FILE_OP_PARALLEL)) {
        qDebug() << "default ALLOW_FILE_OP_PARALLEL:true";
        setValue(ALLOW_FILE_OP_PARALLEL, true);
    }
    //if local languege is chinese, set chinese first as default
    if (QLocale::system().name().contains("zh") && !m_settings->allKeys().contains(SORT_CHINESE_FIRST))
        setValue(SORT_CHINESE_FIRST, true);
    for (auto key : m_settings->allKeys()) {
        m_cache.insert(key, m_settings->value(key));
    }

    m_cache.insert(DISPLAY_STANDARD_ICONS, true);
    if (QGSettings::isSchemaInstalled("org.ukui.peony.settings")) {
        connect(m_peonyGSettings, &QGSettings::changed, this, [=] (const QString &key) {
            m_cache.remove(key);
            m_cache.insert(key, m_peonyGSettings->get(key));
            Q_EMIT this->valueChanged(key);
        });

        for (auto key : m_peonyGSettings->keys()) {
            m_cache.remove(key);
            m_cache.insert(key, m_peonyGSettings->get(key));
        }
    }

    m_cache.insert(TRASH_MOBILE_FILES, false);
    if (QGSettings::isSchemaInstalled("org.ukui.peony.settings")) {
        m_peonyGSettings = new QGSettings("org.ukui.peony.settings", "/org/ukui/peony/settings/", this);
        connect(m_peonyGSettings, &QGSettings::changed, this, [=] (const QString &key) {
            m_cache.remove(key);
            m_cache.insert(key, m_peonyGSettings->get(key));
            Q_EMIT this->valueChanged(key);
        });

        for (auto key : m_peonyGSettings->keys()) {
            m_cache.remove(key);
            m_cache.insert(key, m_peonyGSettings->get(key));
        }
    }

    m_date_format = tr("yyyy/MM/dd");
    m_time_format = tr("HH:mm:ss");
    if (QGSettings::isSchemaInstalled("org.ukui.control-center.panel.plugins")) {
        m_control_center_plugin = new QGSettings("org.ukui.control-center.panel.plugins", QByteArray(), this);
        connect(m_control_center_plugin, &QGSettings::changed, this, [=](const QString &key) {
            QString value = m_control_center_plugin->get(key).toString();
            if ("hoursystem" == key) {
                m_cache.remove(UKUI_CONTROL_CENTER_PANEL_PLUGIN_TIME);
                m_cache.insert(UKUI_CONTROL_CENTER_PANEL_PLUGIN_TIME, value);
                Q_EMIT this->valueChanged(UKUI_CONTROL_CENTER_PANEL_PLUGIN_TIME);
                setTimeFormat(value);
            }
            else if (key == "date")
            {
                m_cache.remove(UKUI_CONTROL_CENTER_PANEL_PLUGIN_DATE);
                m_cache.insert(UKUI_CONTROL_CENTER_PANEL_PLUGIN_DATE, value);
                Q_EMIT this->valueChanged(UKUI_CONTROL_CENTER_PANEL_PLUGIN_DATE);
                setDateFormat(value);
            }
        });

        if (m_control_center_plugin->keys().contains("hoursystem")) {
            QString timeValue = m_control_center_plugin->get("hoursystem").toString();
            m_cache.insert(UKUI_CONTROL_CENTER_PANEL_PLUGIN_TIME, timeValue);
            setTimeFormat(timeValue);
        } else {
            setTimeFormat("24");
        }
        if (m_control_center_plugin->keys().contains("date")) {
            QString dateValue = m_control_center_plugin->get("date").toString();
            m_cache.insert(UKUI_CONTROL_CENTER_PANEL_PLUGIN_DATE, dateValue);
            setDateFormat(dateValue);
        } else {
            setDateFormat("cn");
        }
    }

    m_cache.insert(SHOW_TRASH_DIALOG, true);
    m_cache.insert(SHOW_HIDDEN_PREFERENCE, false);
    m_cache.insert(SHOW_FILE_EXTENSION, true); /* 默认显示文件扩展名 */
    m_cache.insert(SEND_URIS_OF_COPY_DSPS, false);
    m_cache.insert(DOC_IS_OCCUPIED_BY_WPS, false);
    m_cache.insert(USE_GLOBAL_DEFAULT_SORTING, true);
    m_cache.insert(SHOW_CREATE_TIME, false);
    if (QGSettings::isSchemaInstalled("org.ukui.peony.settings")) {
        m_peony_gsettings = new QGSettings("org.ukui.peony.settings", QByteArray(), this);

        /* hotfix bug#101227:解决兼容升级后对应设置项恢复gsetting默认值问题。判断字段（INIT_FOR_FIRST_TIME）不存在，则为首次初始化，反之不是 */
        if(!isExist(INIT_FOR_FIRST_TIME)){
            setValue(INIT_FOR_FIRST_TIME, false);
            /* /usr/share/glib-2.0/schemas/org.ukui.peony.settings.gschema.xml文件首次初始化时，
             * SHOW_HIDDEN_PREFERENCE字段为 "org.ukui/peony-qt-preferences" 文件中"show-hidden"的值 */
            if(isExist("show-hidden")){
                bool value = getValue("show-hidden").toBool();
                m_cache.insert(SHOW_HIDDEN_PREFERENCE, value);
                setGSettingValue(SHOW_HIDDEN_PREFERENCE, value);
            }
        }

        connect(m_peony_gsettings, &QGSettings::changed, this, [=](const QString &key) {
            bool sendChanged = false;
            if ((SHOW_HIDDEN_PREFERENCE == key) || (SHOW_FILE_EXTENSION == key) || key == DISPLAY_STANDARD_ICONS || key == USE_GLOBAL_DEFAULT_SORTING || SHOW_CREATE_TIME == key)) {
                if (m_cache.value(key) != m_peony_gsettings->get(key).toBool())
                {
                    m_cache.remove(key);
                    m_cache.insert(key, m_peony_gsettings->get(key).toBool());
                }
                /* Solve the problem: When opening multiple document management, check "Show hidden files" in one document management,
                 *  but the other document management does not take effect in real time.modified by 2021/06/15  */
                sendChanged = true;
            } else {
                m_cache.remove(key);
                m_cache.insert(key, m_peony_gsettings->get(key));
            }
            if (sendChanged) {
                Q_EMIT this->valueChanged(key);
            }
        });

        for (auto key : m_peony_gsettings->keys()) {
            m_cache.remove(key);
            m_cache.insert(key, m_peony_gsettings->get(key));
        }
    }

    m_cache.insert(SIDEBAR_BG_OPACITY, 100);
    if (QGSettings::isSchemaInstalled(PERSONAL_EFFECT_SCHEMA)) {
        m_gsettings = new QGSettings(PERSONAL_EFFECT_SCHEMA, QByteArray(), this);

        connect(m_gsettings, &QGSettings::changed, this, [=](const QString &key) {
            if (key == PERSONAL_EFFECT_TRANSPARENCY || key == PERSONAL_EFFECT_ENABLE) {
                qreal opacity = m_gsettings->get(PERSONAL_EFFECT_TRANSPARENCY).toReal() * 100;
                m_cache.remove(SIDEBAR_BG_OPACITY);
                m_cache.insert(SIDEBAR_BG_OPACITY, opacity);
                qApp->paletteChanged(qApp->palette());
            }
        });

        if (m_gsettings->keys().contains(PERSONAL_EFFECT_ENABLE)) {
            if (m_gsettings->get(PERSONAL_EFFECT_ENABLE).toBool()) {
                qreal opacity = m_gsettings->get(PERSONAL_EFFECT_TRANSPARENCY).toReal() * 100;
                m_cache.remove(SIDEBAR_BG_OPACITY);
                m_cache.insert(SIDEBAR_BG_OPACITY, opacity);
            }
        }
    }

    getUkuiStyle();
    getDualScreenMode();
    getMachineMode();

    if (m_cache.value(DEFAULT_WINDOW_WIDTH).isNull()
        || m_cache.value(DEFAULT_WINDOW_HEIGHT).isNull()
        || m_cache.value(DEFAULT_SIDEBAR_WIDTH) <= 0)
    {
        QScreen *screen=qApp->primaryScreen();
        if (screen) {
            QRect geometry = screen->availableGeometry();
            int default_width = geometry.width() * 2/3;
            int default_height =  geometry.height() * 4/5;
            if (default_width < 850)
                default_width = 850;
            if (default_height < 850 *0.618)
                default_height = 850 *0.618;
            setValue(DEFAULT_WINDOW_WIDTH, default_width);
            setValue(DEFAULT_WINDOW_HEIGHT, default_height);
            setValue(DEFAULT_SIDEBAR_WIDTH, 292);
            qDebug() << "default set DEFAULT_SIDEBAR_WIDTH:"<<210;
        }
    }

    if (m_cache.value(DEFAULT_VIEW_ID).isNull()) {
        setValue(DEFAULT_VIEW_ID, "Icon View");
    }

    if (m_cache.value(SORT_ORDER).isNull()){
        setValue(SORT_ORDER, Qt::DescendingOrder);
    }

    if (m_cache.value(SORT_COLUMN).isNull()){
        setValue(SORT_COLUMN, 0);
    }

    if (m_cache.value(DEFAULT_VIEW_ZOOM_LEVEL).isNull()) {
        setValue(DEFAULT_VIEW_ZOOM_LEVEL, 70);
    }

    if (m_cache.value(REMOTE_SERVER_REMOTE_IP).isNull()) {
        setValue(REMOTE_SERVER_REMOTE_IP, QVariant(QList<QString>()));
    }


    if (m_cache.value (SORT_TYPE).isNull()) {
        setValue (SORT_TYPE, 0);
    }

    if (m_cache.value (SORT_ORDER).isNull()) {
        setValue (SORT_ORDER, 0);
    }

    if (m_cache.value(DEFAULT_GRID_SIZE).isNull()) {
        setValue(DEFAULT_GRID_SIZE, QSize());
    }

#ifdef KY_SDK_SYSINFO
    auto machine = kdk_system_get_hostCloudPlatform();
    if (machine) {
        if (qstrcmp(machine, "none") == 0) {
            m_cache.insert(IS_GUESTOS_MACHINE, false);
        } else {
            m_cache.insert(IS_GUESTOS_MACHINE, true);
        }
        free(machine);
    } else {
        m_cache.insert(IS_GUESTOS_MACHINE, false);
    }
#endif
    initDateFormatDBus();
}

GlobalSettings::~GlobalSettings()
{

}

void GlobalSettings::getUkuiStyle()
{
    if (QGSettings::isSchemaInstalled(UKUI_CONTROL_CENTER_PERSONALISE)) {
        m_gsettings = new QGSettings(UKUI_CONTROL_CENTER_PERSONALISE);
        connect(m_gsettings, &QGSettings::changed, [this](const QString &key) {
            if (key == PERSONALISE_EFFECT) {
                qreal opacity = 100.0;
                if (m_gsettings->get(PERSONALISE_EFFECT).toBool()) {
                    opacity *= m_gsettings->get(PERSONALISE_TRANSPARENCY).toReal();
                }
                m_cache.remove(SIDEBAR_BG_OPACITY);
                m_cache.insert(SIDEBAR_BG_OPACITY, opacity);
            }
        });
        qreal opacity = 100.0;
        if (m_gsettings->keys().contains(PERSONALISE_EFFECT) && m_gsettings->keys().contains(PERSONALISE_TRANSPARENCY)) {
            if (m_gsettings->get(PERSONALISE_EFFECT).toBool()) {
                opacity *= m_gsettings->get(PERSONALISE_TRANSPARENCY).toReal();
            }
        }
        m_cache.remove(SIDEBAR_BG_OPACITY);
        m_cache.insert(SIDEBAR_BG_OPACITY, opacity);
    } else {
        if (QGSettings::isSchemaInstalled("org.ukui.style")) {
            m_gsettings = new QGSettings("org.ukui.style", QByteArray(), this);
            connect(m_gsettings, &QGSettings::changed, this, [=](const QString &key) {
                if (key == "peonySideBarTransparency") {
                    m_cache.remove(SIDEBAR_BG_OPACITY);
                    m_cache.insert(SIDEBAR_BG_OPACITY, m_gsettings->get(key).toString());
                    qApp->paletteChanged(qApp->palette());
                }
            });
            m_cache.remove(SIDEBAR_BG_OPACITY);
            m_cache.insert(SIDEBAR_BG_OPACITY, m_gsettings->get("peonySideBarTransparency").toString());
        }
    }
}

void GlobalSettings::getMachineMode()
{
    m_cache.insert(TABLET_MODE, "false");
    if (QGSettings::isSchemaInstalled("org.ukui.SettingsDaemon.plugins.tablet-mode")) {
        m_gsettings_tablet_mode = new QGSettings("org.ukui.SettingsDaemon.plugins.tablet-mode", QByteArray(), this);
        m_cache.remove(TABLET_MODE);
        m_cache.insert(TABLET_MODE, m_gsettings_tablet_mode->get("tablet-mode").toString());
        connect(m_gsettings_tablet_mode, &QGSettings::changed, this, [=](const QString &key) {
            if (key == "tabletMode") {
                m_cache.remove(TABLET_MODE);
                m_cache.insert(TABLET_MODE, m_gsettings_tablet_mode->get(key).toString());
                qApp->paletteChanged(qApp->palette());
            }
        });
    }
}

void GlobalSettings::getDualScreenMode()
{
    m_cache.insert(DUAL_SCREEN_MODE, DUAL_SCREEN_EXPAND_MODE);
    if(QGSettings::isSchemaInstalled(SETTINGS_DAEMON_SCHEMA_XRANDR)) {
        m_gsettings_dual_screen_mode = new QGSettings(SETTINGS_DAEMON_SCHEMA_XRANDR, QByteArray(), this);
        m_cache.remove(DUAL_SCREEN_MODE);
        if (m_gsettings_dual_screen_mode->keys().contains(DUAL_SCREEN_MODE)) {
            m_cache.insert(DUAL_SCREEN_MODE, m_gsettings_dual_screen_mode->get(DUAL_SCREEN_MODE).toString());
        }
        connect(m_gsettings_dual_screen_mode, &QGSettings::changed, this, [=](const QString &key){
           if (key == DUAL_SCREEN_MODE) {
               m_cache.remove(DUAL_SCREEN_MODE);
               m_cache.insert(DUAL_SCREEN_MODE, m_gsettings_dual_screen_mode->get(key).toString());
               qApp->paletteChanged(qApp->palette());
           }
        });
    }
}

const QVariant GlobalSettings::getValue(const QString &key)
{
    return m_cache.value(key);
}

bool GlobalSettings::initDateFormatDBus()
{
#ifdef KY_SDK_DATE
    QDBusConnection conn = QDBusConnection::sessionBus();
    if (! conn.isConnected()) {
        qCritical()<<"failed to init mDbusDateServer, can not connect to session dbus";
        return false;
    }

    mDbusDateServer = new QDBusInterface(SDK_DATE_SERVER_SERVICE,
                                         SDK_DATE_SERVER_PATH,
                                         SDK_DATE_SERVER_INTERFACE,
                                         QDBusConnection::sessionBus());

    if (! mDbusDateServer->isValid()){
        qCritical() << "Create /com/kylin/kysdk/Date Interface Failed " << QDBusConnection::systemBus().lastError();
        return false;
    }

    QDBusConnection::sessionBus().connect(SDK_DATE_SERVER_SERVICE,
                                          SDK_DATE_SERVER_PATH,
                                          SDK_DATE_SERVER_INTERFACE,
                                          "ShortDateSignal",
                                          this,
                                          SLOT(sendShortDataFormat(QString)));

    QDBusConnection::sessionBus().connect(SDK_DATE_SERVER_SERVICE,
                                          SDK_DATE_SERVER_PATH,
                                          SDK_DATE_SERVER_INTERFACE,
                                          "LongDateSignal",
                                          this,
                                          SLOT(sendLongDataFormat(QString)));

    return true;
#endif

    return false;
}

void GlobalSettings::sendShortDataFormat(const QString &format)
{
    Q_EMIT this->updateShortDataFormat(format);
}

void GlobalSettings::sendLongDataFormat(const QString &format)
{
    Q_EMIT this->updateLongDataFormat(format);
}

bool GlobalSettings::isExist(const QString &key)
{
    return !m_cache.value(key).isNull();
}

void GlobalSettings::reset(const QString &key)
{
    m_cache.remove(key);
    QtConcurrent::run([=]() {
        if (m_mutex.tryLock(1000)) {
            m_settings->remove(key);
            m_settings->sync();
            m_mutex.unlock();
        }
    });
    Q_EMIT this->valueChanged(key);
}

void GlobalSettings::resetAll()
{
    QStringList tmp = m_cache.keys();
    m_cache.clear();
    for (auto key : tmp) {
        Q_EMIT this->valueChanged(key);
    }
    QtConcurrent::run([=]() {
        if (m_mutex.tryLock(1000)) {
            m_settings->clear();
            m_settings->sync();
            m_mutex.unlock();
        }
    });
}

void GlobalSettings::setValue(const QString &key, const QVariant &value)
{
    if (key == REMOTE_SERVER_REMOTE_IP || key == DEFAULT_WINDOW_SIZE || !m_peonyGSettings->keys().contains(key)) {
        m_cache.remove(key);
        m_cache.insert(key, value);
        QtConcurrent::run([=]() {
            if (m_mutex.tryLock(1000)) {
                m_settings->setValue(key, value);
                m_settings->sync();
                m_mutex.unlock();
            }
        });
    } else {
        setGSettingValue(key, value);
    }
}

void GlobalSettings::forceSync(const QString &key)
{
    m_settings->sync();
    if (key.isNull()) {
        m_cache.clear();
        for (auto key : m_settings->allKeys()) {
            m_cache.insert(key, m_settings->value(key));
        }

        if (m_peony_gsettings) {
            for (auto key : m_peony_gsettings->keys()) {
                m_cache.insert(key, m_peony_gsettings->get(key));
            }
        }
    } else {
        m_cache.remove(key);
        if (m_settings->allKeys().contains(key)) {
            m_cache.insert(key, m_settings->value(key));
        } else {
            m_cache.insert(key, m_peony_gsettings ? m_peony_gsettings->get(key) : QVariant());
        }
    }
}

void GlobalSettings::slot_updateRemoteServer(const QString& server, bool add)
{
    Q_EMIT signal_updateRemoteServer(server, add);
}

bool GlobalSettings::isGuestOSMachine()
{
    return m_cache.value(IS_GUESTOS_MACHINE).toBool();
}

void GlobalSettings::setTimeFormat(const QString &value)
{
    if (value == "12"){
        m_time_format = tr("AP hh:mm:ss");
    }
    else{
        m_time_format = tr("HH:mm:ss");
    }
    m_system_time_format = m_date_format + " " + m_time_format;
}

void GlobalSettings::setDateFormat(const QString &value)
{
    if (value == "cn"){
        m_date_format = tr("yyyy/MM/dd");
    }
    else{
        m_date_format = tr("yyyy-MM-dd");
    }
    m_system_time_format = m_date_format + " " + m_time_format;
}

QString GlobalSettings::getSystemTimeFormat()
{
    //m_system_time_format = m_date_format + " " + m_time_format;
    return m_system_time_format;
}

QString GlobalSettings::transToSystemTimeFormat(guint64 mtime, bool longFormat)
{
    QDateTime dateTime = QDateTime::fromMSecsSinceEpoch(mtime *1000);
    QString systemTimeFormat = GlobalSettings::getInstance()->getSystemTimeFormat();

#ifdef KY_SDK_DATE
    struct tm *m_tm;
    time_t lt;
    lt = time(NULL);
    m_tm = localtime(&lt);

    QDate date = dateTime.date();
    QTime qtime = dateTime.time();

    m_tm->tm_year = date.year();
    m_tm->tm_mon = date.month();
    m_tm->tm_mday = date.day();

    m_tm->tm_hour = qtime.hour();
    m_tm->tm_min = qtime.minute();
    m_tm->tm_sec = qtime.second();
    //qDebug() << "year:"<<date.year()<<"month:"<<date.month()<<"day:"<<date.day();
    //set date and time show format, task #101605
    auto ret = kdk_system_timeformat_transform(m_tm);
    auto formatDate = kdk_system_shortformat_transform(m_tm);
    //sdk接口会改变结构体数据，需要重初始化要使用的日期数据
    //属于接口缺陷，已跟SDK接口负责人沟通，先使用此方式
    m_tm->tm_year = date.year();
    m_tm->tm_mon = date.month();
    m_tm->tm_mday = date.day();
    if (longFormat)
        formatDate = kdk_system_longformat_transform(m_tm);
    if (ret && formatDate){
        QString dateStr = g_strdup_printf("%s %s", formatDate, ret->timesec);
        //qDebug() << "transToSystemTimeFormat:"<<dateStr<<systemTimeFormat;
        //释放结构体
        kdk_free_timeinfo(ret);

        //use sdk interface
        if (dateStr.trimmed().length() > 0)
            return dateStr;
    }
#endif

    //old way of date, processed by self
    return dateTime.toString(systemTimeFormat);
}

void GlobalSettings::setGSettingValue(const QString &key, const QVariant &value)
{
    if (!m_peony_gsettings)
        return;

    const QStringList list = m_peony_gsettings->keys();
    if (!list.contains(key)) {
        qWarning()<<"set gsettings failed"<<key<<value<<"not existed!";
        return;
    }

    m_peony_gsettings->set(key, value);
    m_cache.remove(key);
    m_cache.insert(key, m_peony_gsettings->get(key));
}

QString GlobalSettings::getProjectName()
{
#ifdef KYLIN_COMMON
    return QString::fromStdString(KDKGetPrjCodeName());
#else
    return "unknown-project-name";
#endif // KYLIN_COMMON
}
