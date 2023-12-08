#ifndef TESTVFSREGISTER_H
#define TESTVFSREGISTER_H

#include "file-watcher.h"
#include <peony-core_global.h>
#include <vfs-plugin-iface.h>
#include <vfs-info-plugin-iface.h>
#include <file-tmp-info.h>
#include <QMap>

namespace Peony {

class TestVFSInternalPlugin : public VFSPluginIface {
public:
    TestVFSInternalPlugin(QString scheme = "");

    virtual PluginType pluginType() override {return VFSPlugin;}

    virtual const QString name() override {return "test simplify vfs";}
    virtual const QString description() override {return QObject::tr("test simplify vfs plugin");}
    virtual const QIcon icon() override {return QIcon();}
    virtual void setEnable(bool enable) {Q_UNUSED(enable);}
    virtual bool isEnable() {return true;}

    void initVFS() override;
    QString uriScheme() override;
    bool holdInSideBar() override;
    void *parseUriToVFSFile(const QString &uri) override;
    CustomErrorHandler *customErrorHandler() override;

private:
    QString m_scheme;
};

class PEONYCORESHARED_EXPORT TestVFSRegister
{
public:
    static void registTestVFS(QString scheme = "");

private:
    TestVFSRegister();
};

#ifndef VFS_CUSTOM_PLUGIN
class CustomVFSInfoInernalPlugin : public VFSInfoPluginIface {
public:
    CustomVFSInfoInernalPlugin();

    virtual PluginType pluginType() override {return VFSINFOPlugin;}

    virtual const QString name() override {return "custom vfs info";}
    virtual const QString description() override {return QObject::tr("Default custom vfs info of peony");}
    virtual const QIcon icon() override {return QIcon();}
    virtual void setEnable(bool enable) {}
    virtual bool isEnable() {return true;}

    QString pathScheme() override;

    bool holdInSideBar() override {return true;}

    CustomErrorHandler *customErrorHandler() override {return nullptr;}

    QStringList fileEnumerator(const QString &path) override;

    std::shared_ptr<FileTmpInfo> queryFile(const QString &path) override;

    VFSError* handerMakeDirectory(const QString &uri, bool &ret) override;

    VFSError* handerFileCreate(const QString &uri, bool &ret) override;

    VFSError* handerFileDelete(const QString &uri, bool &ret) override;

    VFSError* handerFileCopy(const QString &sourceUri, const QString &destUri, bool &ret) override;

    VFSError* handerFileMove(const QString &sourceUri, const QString &destUri, bool &ret) override;

    VFSError* handerFileRename(const QString &sourceUri, const QString &newName) override;

    void handerMonitorDirectory(HanderTransfer *transfer, const QString &uri) override;

private:
    //存储目录，当前目录下所有的子文件和子文件夹
    QMap<QString, QStringList> m_allInfoMap;
};


class LocalVFSInfoInternalPlugin2 : public VFSInfoPluginIface {
public:
    LocalVFSInfoInternalPlugin2();

    virtual PluginType pluginType() override {return VFSINFOPlugin;}

    virtual const QString name() override {return "local vfs info";}
    virtual const QString description() override {return QObject::tr("Default local vfs info of peony");}
    virtual const QIcon icon() override {return QIcon();}
    virtual void setEnable(bool enable) {Q_UNUSED(enable)}
    virtual bool isEnable() {return true;}

    QString pathScheme() override;

    bool holdInSideBar() override {return true;}

    CustomErrorHandler *customErrorHandler() override {return nullptr;}

    QStringList fileEnumerator(const QString &path) override;

    std::shared_ptr<FileTmpInfo> queryFile(const QString &path) override;

    VFSError* handerMakeDirectory(const QString &uri, bool &ret) override;

    VFSError* handerFileCreate(const QString &uri, bool &ret) override;

    VFSError* handerFileDelete(const QString &uri, bool &ret) override;

    VFSError* handerFileCopy(const QString &sourceUri, const QString &destUri, bool &ret) override;

    VFSError* handerFileMove(const QString &sourceUri, const QString &destUri, bool &ret) override;

    VFSError* handerFileRename(const QString &sourceUri, const QString &newName) override;

    void handerMonitorDirectory(HanderTransfer *transfer, const QString &uri) override;
};
#endif

}


#endif // TESTVFSREGISTER_H
