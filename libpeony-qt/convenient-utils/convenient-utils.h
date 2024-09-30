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

    QStringList getFileUrisInSequence(const QString& uri) const;

private:
    explicit ConvenientUtils(QObject *parent = nullptr);
    ~ConvenientUtils(){}

    static ConvenientUtils *global_instance;
};
}

#endif // CONVENIENTUTILS_H
