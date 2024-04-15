#include <QSettings>
#include <QStandardPaths>
#include <QPoint>
#include <QSize>
#include <QProcess>
#include <QDebug>

#undef signals

#include <gio/gio.h>

QHash <QString, QPoint> metaPoses;
QHash <QString, QPoint> gridPoses;
QList <QPoint> points;

void updateMetaInfoGridConfig()
{
    QProcess p;
    p.setProgram("/usr/bin/meta-set");
    for (auto item : gridPoses.keys()) {
        auto gridPos = gridPoses.value(item);
        if (item.startsWith("/")) {
            p.setArguments(QStringList()<<"-l"<<item<<"peony-qt-desktop-item-grid-pos"<<QString::number(gridPos.x())<<QString::number(gridPos.y()));
            p.start();
            p.waitForFinished();
        } else if (item.startsWith("computer")) {
            p.setArguments(QStringList()<<"-tl"<<"computer:"<<"/"<<"peony-qt-desktop-item-grid-pos"<<QString::number(gridPos.x())<<QString::number(gridPos.y()));
            p.start();
            p.waitForFinished();
        } else if (item.startsWith("trash")) {
            p.setArguments(QStringList()<<"-tl"<<"trash:"<<"/"<<"peony-qt-desktop-item-grid-pos"<<QString::number(gridPos.x())<<QString::number(gridPos.y()));
            p.start();
            p.waitForFinished();
        }
    }
    p.close();
}

int main(int argc, char *argv[])
{
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    //QCoreApplication a(argc, argv);

    QString configPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.peony-desktop-grid.conf";
    auto keyfile = g_key_file_new();
    // 待更新的groups
    QStringList groupsList;
    if (g_key_file_load_from_file(keyfile, configPath.toUtf8().constData(), G_KEY_FILE_NONE, nullptr)) {
        gchar **groups = g_key_file_get_groups(keyfile, nullptr);
        gchar **p = groups;
        while (*p) {
            int x = g_key_file_get_integer(keyfile, *p, "x", nullptr);
            int y = g_key_file_get_integer(keyfile, *p, "y", nullptr);
            //qDebug()<<"group"<<*p<<"x"<<x<<"y"<<y;

            QPoint pos = QPoint(x, y);
            metaPoses.insert(*p, pos);
            if (!pos.isNull()) {
                groupsList << *p;
                if (!points.contains(pos)) {
                    points.append(pos);
                }
            } else {
                // 按照未分配的网格设置(-1, -1)
                g_key_file_set_integer(keyfile, *p, "gridX", -1);
                g_key_file_set_integer(keyfile, *p, "gridY", -1);
                gridPoses.insert(*p, QPoint(-1, -1));
            }
            p++;
        }
        if (groups)
            g_strfreev (groups);
        //qDebug()<<groupsList;
    } else {
        // 存在问题
        return -1;
    }

    // 尝试读取配置
    // 读取当前peony的网格配置（2303以后的版本有网格配置，作为网格大小），如果不存在此配置，则使用网格算法1对网格进行计算后排列
    QSize gridSize;
    QSettings peonyCurrentSettings(QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.config/org.ukui/peony-qt-preferences.conf", QSettings::NativeFormat);
    if (peonyCurrentSettings.childKeys().contains("default-grid-size")) {
        gridSize = peonyCurrentSettings.value("default-grid-size").toSize();
        qDebug()<<"read config"<<gridSize;
    }

    // 尝试计算网格
    // 网格算法1，排除在0，0的点（0，0一般是桌面满屏时放置的点），点任取一点，和其它点作差，得到和其它点x、y的差值，最小的值为网格的width、height，如果存在差值为0也排除（多屏场景可能有图标的位置相同）。此算法较为简单，但是对于图标比较少和分散或者存在异常图标位置的场景处理较差
    // 基于网格算法1，如果存在异常点，则需要加入验算流程，如果无法正常处理或者网格大小偏差太大，则抛弃网格算法1
    if (gridSize.isEmpty() && points.count() > 1) {
        auto anchorPoint = points.takeFirst();
        int gridWidth = INT_MAX;
        int gridHeigt = INT_MAX;
        while (!points.isEmpty()) {
            auto currentPoint = points.takeFirst();
            QPoint offset = anchorPoint - currentPoint;
            int offsetX = qAbs(offset.x());
            int offsetY = qAbs(offset.y());
            if (offsetX > 0) {
                gridWidth = qMin(offsetX, gridWidth);
            }
            if (offsetY > 0) {
                gridHeigt = qMin(offsetY, gridHeigt);
            }
        }
    }
    qDebug()<<"grid size"<<gridSize;

    // 无法计算网格的情况下，根据当前图标大小，给定一个合适的网格，根据现有位置顺序对图标进行重排

    // 按照计算的网格大小更新各个item的网格位置
    for (QString group : groupsList) {
        if (gridPoses.contains(group)) {
            g_key_file_set_integer(keyfile, group.toUtf8().constData(), "gridX", -1);
            g_key_file_set_integer(keyfile, group.toUtf8().constData(), "gridY", -1);
            continue;
        }
        auto pos = metaPoses.value(group);
        int gridX = pos.x()/gridSize.width();
        int gridY = pos.y()/gridSize.height();
        g_key_file_set_integer(keyfile, group.toUtf8().constData(), "gridX", gridX);
        g_key_file_set_integer(keyfile, group.toUtf8().constData(), "gridY", gridY);
        gridPoses.insert(group, QPoint(gridX, gridY));
    }

    g_key_file_save_to_file(keyfile, configPath.toUtf8().constData(), nullptr);
    g_key_file_free (keyfile);

    qDebug()<<metaPoses;
    qDebug()<<gridPoses;

    updateMetaInfoGridConfig();

    return 0;
}
