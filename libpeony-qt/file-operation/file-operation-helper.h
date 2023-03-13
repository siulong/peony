#ifndef FILEOPERATIONHELPER_H
#define FILEOPERATIONHELPER_H

#ifdef KY_UDF_BURN

#include "file-operation.h"
#include <QObject>

class FileOperationHelper : public QObject
{
    Q_OBJECT
public:
    explicit FileOperationHelper(const QString destDir,QObject *parent = nullptr);
    ~FileOperationHelper();

    void judgeSpecialDiscOperation();

    QString getDiscType();

    QString dealDVDReduce();

    bool isUnixCDDevice();

    bool discWriteOperation(const QStringList &sourUrisList, const QString &destUri);

    void discRenameOperation(const QString &oldName,const QString &newName);

    void discDeleteOperation(const QStringList &srcUris);

    QString getDestName(const QString &destUri);

    QString getDiscError();
protected:
    /**
     * @brief matchingUnixDevice
     * @param uri
     * @return
     * Matches the device mount point
     */
    QString matchingUnixDevice(QString uri);

private:
    bool m_is_disk_work = false;
    QString m_unix_device = nullptr;
    QString m_disc_media_type = nullptr;
    QString m_disc_system_type = nullptr;
    QString m_disc_error_msg = nullptr;

};

#endif

#endif // FILEOPERATIONHELPER_H
