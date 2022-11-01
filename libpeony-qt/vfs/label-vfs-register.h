#ifndef LABELVFSREGISTER_H
#define LABELVFSREGISTER_H

#include "peony-core_global.h"
#include "vfs-plugin-iface.h"

namespace Peony {

class LabelVFSInternalPlugin : public VFSPluginIface
{
public:
    LabelVFSInternalPlugin () {}

    virtual PluginType pluginType () override {return VFSPlugin;}

    virtual void setEnable (bool enable) {}
    virtual bool isEnable () {return true;}
    virtual const QIcon icon () override {return QIcon();}
    virtual const QString name () override {return "favorite vfs";}
    virtual const QString description () override {return QObject::tr("Default favorite vfs of peony");}

    void initVFS () override;
    bool holdInSideBar () override {return false;}
    QString uriScheme () override {return "labels://";}
    void* parseUriToVFSFile (const QString &uri) override;
    CustomErrorHandler *customErrorHandler() override {return nullptr;}
};

class PEONYCORESHARED_EXPORT LabelVFSRegister
{
public:
    static void registLabelVFS ();

private:
    LabelVFSRegister ();
};

}
#endif // LABELVFSREGISTER_H
