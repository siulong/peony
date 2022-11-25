#ifndef DISCSCSI_H
#define DISCSCSI_H

#include <sys/ioctl.h>
#undef __STRICT_ANSI__
#include <linux/cdrom.h>
#define __STRICT_ANSI__
#include <scsi/sg.h>

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include <QString>

enum DiscScsiTransferDirection{
    SCSI_READ,  //读设备
    SCSI_WRITE
};

/* disk(硬盘) disc(光盘)都属于scsi设备
 * 本类提供的transport()主要用于从设备中读取信息
 */
class DiscScsi
{
public:


public:
    DiscScsi();
    DiscScsi(QString device);
    ~DiscScsi();
    uchar& operator[](ulong i);//重载[]，用于修改mCmd
    bool transport(DiscScsiTransferDirection dir=SCSI_READ,
                   void* buffer=0, ulong length=0);//提供给外部的数据传输接口
private:
    void initMembers();        //初始化成员
private:
    int mDeviceHandle;         //句柄，文件描述符
    QString mDevice;

    struct sg_io_hdr mSgIo;
    struct request_sense mSense;
    struct cdrom_generic_command mCmd;

};
#endif // DISCSCSI_H
