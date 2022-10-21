#ifndef DISCCOMMAND_H
#define DISCCOMMAND_H

#include <QObject>
#include <QProcess>

#include <peony-core_global.h>

#ifdef signals
#undef signals
#endif
class PEONYCORESHARED_EXPORT DiscCommand : public QObject
{
    Q_OBJECT
public:
    explicit DiscCommand(QObject *parent = nullptr);
    void setCmd(const QString&, const QStringList&);
    void startCmd();
    bool startAndWaitCmd(QString&);
    ~DiscCommand();
private:
    void initMembers();

Q_SIGNALS:
    void cmdFinished(QString& finishedInfo);        //命令执行完成的信号(报错信息)

private Q_SLOTS:
    void cmdFinishSlot(int, QProcess::ExitStatus);
    void readOutputSlot();
    void readErrorSlot();
private:
    QProcess *mProcess;
    QString mCommand;
    bool mSuccess;
    QString mFinishedInfo;
};

#endif // DISCCOMMAND_H

/**
  1. linux下目录不可以创建硬链接
  2. 目录创建软连接后，xorriso直接使用软连接后的目录名进行刻录，不报错，但是未刻录进光盘
  3. 硬链接没有方法通过代码实现，只能使用"ln origin dest"命令实现
*/

/**

*/
