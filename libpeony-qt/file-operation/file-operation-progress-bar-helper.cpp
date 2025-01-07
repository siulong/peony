#include "file-operation-progress-bar-helper.h"
#include <QDebug>
#define MEGABYTE 1048576.0

progressBarHelper::progressBarHelper(QObject *parent) : QObject(parent)
{

}

double progressBarHelper::calculateSpeed(const qint64 &currentSize, const double &elapsedSeconds)
{
    double currentSpeed = 0.0;
    currentSpeed = currentSize / MEGABYTE / elapsedSeconds;
    qDebug() << currentSpeed << "Mb/s";
    return currentSpeed;
}

int progressBarHelper::calculateEstimatedTime(const qint64 &residualSize ,const double &currentSpeed)
{
    int estimatedTime = 0;
    estimatedTime = residualSize / MEGABYTE / currentSpeed;
    qDebug()<< "estimated time" << estimatedTime;
    return estimatedTime;
}

QString progressBarHelper::timeToString(const int &time)
{
    int days = time / 86400;
    int hours = (time % 86400) / 3600;
    int minutes = (time % 3600) / 60;
    int remainingSeconds = time % 60;
    QString currentEstimatedTime = tr("Calculating time");
    if(days > 31) {
        //计算时长超过一个月认为数据有误，不进行展示
        return currentEstimatedTime;
    }
    if (days > 0) {
        currentEstimatedTime = QString(tr("%1day%2hrs%3mins%4sec"))
                         .arg(days)
                         .arg(hours, 2, 10, QLatin1Char('0'))
                         .arg(minutes, 2, 10, QLatin1Char('0'))
                         .arg(remainingSeconds, 2, 10, QLatin1Char('0'));
    } else if (hours > 0) {
        currentEstimatedTime = QString(tr("%1hrs%2mins%3sec"))
                         .arg(hours, 2, 10, QLatin1Char('0'))
                         .arg(minutes, 2, 10, QLatin1Char('0'))
                         .arg(remainingSeconds, 2, 10, QLatin1Char('0'));
    } else if (minutes > 0) {
        currentEstimatedTime = QString(tr("%1 mins%2sec"))
                         .arg(minutes)
                         .arg(remainingSeconds, 2, 10, QLatin1Char('0'));
    } else if (remainingSeconds > 0){
        currentEstimatedTime = QString(tr("%1sec")).arg(remainingSeconds);
    }
    return currentEstimatedTime;
}
