#ifndef CONVENIENTUTILS_H
#define CONVENIENTUTILS_H

#include <QStringList>
#include <QObject>

#include "peony-core_global.h"

namespace Peony {

class PEONYCORESHARED_EXPORT ConvenientUtils: public QObject
{
    Q_OBJECT
public:
    static ConvenientUtils *getInstance();

    QStringList getFileUrisInSequence(const QString& uri) const; /* 传入和输出都是encoded uri */
    QStringList getFilePathsInSequence(const QString& filePath) const;/* 传入和输出都是decoded absolute path，目前只用于‘file://’开头的目录 */

private:
    explicit ConvenientUtils(QObject *parent = nullptr);
    ~ConvenientUtils(){}

    static ConvenientUtils *global_instance;
};
}

#endif // CONVENIENTUTILS_H
