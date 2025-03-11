#ifndef PROGRESSBARHELPER_H
#define PROGRESSBARHELPER_H
#include <qobject.h>

class progressBarHelper : public QObject
{
    Q_OBJECT
public:
    explicit progressBarHelper(QObject *parent = nullptr);
    static double calculateSpeed(const qint64 &currentSize, const double &elapsedSeconds);
    static int calculateEstimatedTime(const qint64 &residualSize ,const double &currentSpeed);
    static QString timeToString(const int &time);
};

#endif // PROGRESSBARHELPER_H
