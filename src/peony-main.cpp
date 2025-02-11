/*
 * Peony-Qt
 *
 * Copyright (C) 2020, KylinSoft Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Authors: Yue Lan <lanyue@kylinos.cn>
 *
 */

#include "peony-application.h"

//#include "main-window.h"

#include <stdio.h>
#include <stdlib.h>
#include <QTime>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QDateTime>
#include <QTextStream>
#include <ukui-log4qt.h>
//#include "navigation-tab-bar.h"
//#include "tab-widget.h"

#include "global-settings.h"

#include "xdg-portal-helper.h"

#ifdef KY_SDK_KABASE
#include <kysdk/applications/kabase/log.hpp>
#endif

#include <QDebug>
#include <signal.h>
#include <execinfo.h>
#include <pthread.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <QThread>
#include <pthread.h>
#include <stdio.h>

pthread_t threadID = pthread_self(); // 获取当前线程ID
QString resolveAddress(void* addr);
#define BUFF_SIZE  1024

static void crashHandler(int sig) {
    signal(sig, SIG_IGN);

    // 使用纯 C 风格的时间戳记录时间
    time_t now = time(nullptr);
    struct tm* localTime = localtime(&now);

    // 打开日志文件
    char path[BUFF_SIZE] = {0};
    snprintf(path, BUFF_SIZE, "%s/.log/peony.log", getenv("HOME"));
    FILE* fp = fopen(path, "a");
    if (!fp) return;

    // 获取当前线程ID
    pthread_t threadID = pthread_self();
    fprintf(fp, "[PEONY TRACE LOG] %04d-%02d-%02d %02d:%02d:%02d\n",
            localTime->tm_year + 1900, localTime->tm_mon + 1, localTime->tm_mday,
            localTime->tm_hour, localTime->tm_min, localTime->tm_sec);
    fprintf(fp, "PID: %d\nSignal: %d (%s)\n", getpid(), sig, strsignal(sig));
    fprintf(fp, "Thread ID: %lu\n", threadID);  // 打印线程ID

    // 写入崩溃信息
    fprintf(fp, "[PEONY TRACE LOG] %04d-%02d-%02d %02d:%02d:%02d\n",
            localTime->tm_year + 1900, localTime->tm_mon + 1, localTime->tm_mday,
            localTime->tm_hour, localTime->tm_min, localTime->tm_sec);
    fprintf(fp, "PID: %d\nSignal: %d (%s)\n", getpid(), sig, strsignal(sig));

    // 获取堆栈信息
    void* array[20];
    int size = backtrace(array, 20);
    char** stackList = backtrace_symbols(array, size);
    if (stackList) {
        fprintf(fp, "Stack Trace:\n");
        for (int i = 0; i < size; i++) {
            fprintf(fp, "#%d %s\n", i, stackList[i]);

            // 调用 resolveAddress 函数解析地址
            QString resolved = resolveAddress(array[i]);
            if (!resolved.isEmpty()) {
                fprintf(fp, "  Resolved: %s\n", resolved.toUtf8().constData());
            } else {
                fprintf(fp, "  Resolved: <unresolved>\n");
            }
        }
        free(stackList);
    }

    fclose(fp);

    // 安全退出程序
    _exit(128 + sig);
}

static void registerSignals() {
    sigset_t mask;
    sigemptyset(&mask);

    // 定义常用崩溃信号
    const auto signalList = {SIGSEGV, SIGILL, SIGTERM, SIGHUP, SIGABRT};
    for (const auto &signal : signalList) {
        struct sigaction action;
        action.sa_handler = crashHandler; // 指定处理函数
        action.sa_flags = SA_RESTART;
        sigemptyset(&action.sa_mask);

        // 注册信号处理
        if (sigaction(signal, &action, nullptr) < 0) {
            qDebug() << "Failed to register signal handler for signal:" << signal << strerror(errno);
        }

        // 添加信号到屏蔽集
        if (sigaddset(&mask, signal) < 0) {
            qDebug() << "Failed to add signal to mask:" << signal << strerror(errno);
        }
    }

    // 解除信号阻塞
    if (sigprocmask(SIG_UNBLOCK, &mask, nullptr) < 0) {
        qDebug() << "Failed to unblock signals:" << strerror(errno);
    }
}

QString resolveAddress(void* addr) {
    // 获取当前程序路径
    QString programPath;
    QFile exeFile("/proc/self/exe");
    if (exeFile.exists()) {
        char exePath[1024] = {0};
        ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
        if (len != -1) {
            exePath[len] = '\0'; // Null-terminate
            programPath = QString::fromUtf8(exePath);
        } else {
            programPath = "<unknown>";
        }
    } else {
        programPath = "<unknown>";
    }

    // 使用 QProcess 调用 addr2line
    QStringList arguments;
    arguments << "-e" << programPath
              << QString::asprintf("%p", addr)
              << "-f" << "-p" << "-C";

    QProcess process;
    process.start("addr2line", arguments);
    if (!process.waitForFinished(3000)) { // 3秒超时
        qWarning() << "addr2line execution timed out or failed.";
        return QString("Unresolved address: %1").arg(reinterpret_cast<quintptr>(addr), 0, 16);
    }

    // 读取标准输出
    QString output = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    if (!output.isEmpty() && !output.contains("??")) {
        return output; // 返回解析结果
    }

    // 如果 addr2line 解析失败，尝试读取动态库信息
    QFile mapsFile("/proc/self/maps");
    if (mapsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        while (!mapsFile.atEnd()) {
            QByteArray line = mapsFile.readLine();
            unsigned long start, end;
            char libPath[512] = {0};
            if (sscanf(line.data(), "%lx-%lx %*s %*s %*s %*d %s", &start, &end, libPath) == 3) {
                if (reinterpret_cast<unsigned long>(addr) >= start &&
                    reinterpret_cast<unsigned long>(addr) < end) {
                    // 计算偏移地址
                    unsigned long offset = reinterpret_cast<unsigned long>(addr) - start;
                    return QString("Address %1 in %2 (offset 0x%3)")
                        .arg(reinterpret_cast<quintptr>(addr), 0, 16)
                        .arg(QString::fromUtf8(libPath))
                        .arg(offset, 0, 16);
                }
            }
        }
    }

    // 如果未找到动态库，直接返回地址
    return QString("Unresolved address: %1").arg(reinterpret_cast<quintptr>(addr), 0, 16);
}

void messageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    QByteArray currentTime = QTime::currentTime().toString().toLocal8Bit();

    bool showDebug = true;
    QString logFilePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/peony-qt.log";
    //屏蔽代码，自动生成日志，无需手动创建
//    if (!QFile::exists(logFilePath)) {
//        showDebug = false;
//    }
    FILE *log_file = nullptr;

    if (showDebug) {
        log_file = fopen(logFilePath.toLocal8Bit().constData(), "a+");
    }

    const char *file = context.file ? context.file : "";
    const char *function = context.function ? context.function : "";
    switch (type) {
    case QtDebugMsg:
        if (!log_file) {
            break;
        }
        fprintf(log_file, "Debug: %s: %s (%s:%u, %s)\n", currentTime.constData(), localMsg.constData(), file, context.line, function);
        break;
    case QtInfoMsg:
        fprintf(log_file? log_file: stdout, "Info: %s: %s (%s:%u, %s)\n", currentTime.constData(), localMsg.constData(), file, context.line, function);
        break;
    case QtWarningMsg:
        fprintf(log_file? log_file: stderr, "Warning: %s: %s (%s:%u, %s)\n", currentTime.constData(), localMsg.constData(), file, context.line, function);
        break;
    case QtCriticalMsg:
        fprintf(log_file? log_file: stderr, "Critical: %s: %s (%s:%u, %s)\n", currentTime.constData(), localMsg.constData(), file, context.line, function);
        break;
    case QtFatalMsg:
        fprintf(log_file? log_file: stderr, "Fatal: %s: %s (%s:%u, %s)\n", currentTime.constData(), localMsg.constData(), file, context.line, function);
        break;
    }

    if (log_file)
        fclose(log_file);
}

int main(int argc, char *argv[])
{
    Peony::XdgPortalHelper::getInstance()->tryUnusePortal();
    PeonyApplication::peony_start_time = QDateTime::currentMSecsSinceEpoch();
//    initUkuiLog4qt("peony");
//    qInstallMessageHandler(messageOutput);
#ifdef KY_SDK_KABASE
    qInstallMessageHandler(kdk::kabase::Log::logOutput);
#else
    initUkuiLog4qt("peony");
#endif

    qDebug() << "peony start in main time:" <<PeonyApplication::peony_start_time ;

    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif

    if (Peony::GlobalSettings::getInstance()->getProjectName() == V10_SP1_EDU) {
        // hide template file
        QFile file(QString("%1/.hidden").arg(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)));
        if (!file.exists()) {
            file.open(QFile::WriteOnly);
            QFileInfo templateFileInfo(g_get_user_special_dir(G_USER_DIRECTORY_TEMPLATES));
            file.write(templateFileInfo.baseName().toLocal8Bit().constData());
            file.close();
        }
    }

    PeonyApplication app(argc, argv, "peony-qt");
    qApp->setProperty("isPeony", true);

    registerSignals();

    Peony::XdgPortalHelper::getInstance()->tryResetPortal();
    if (app.isSecondary())
        return 0;

    return app.exec();
}
