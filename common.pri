VERSION = 3.2.2
DEFINES += VERSION='\\"$${VERSION}\\"'

DEFINES += V10_SP1='\\"V10SP1\\"'
DEFINES += V10_SP1_EDU='\\"V10SP1-edu\\"'

DEB_VERSION = 3.14.5.7
DEFINES += DEB_VERSION='\\"$${DEB_VERSION}\\"'

exists(/usr/include/ukuisdk/kylin-com4cxx.h) {
    message("kylin common for cxx find.")
    DEFINES += KYLIN_COMMON=true
    LIBS += -lukui-com4cxx
}

exists(/usr/include/kysdk/applications/kdialog.h) {
    message("kyfiledialog find")
    DEFINES += KY_FILE_DIALOG
}

exists(/usr/include/kysdk/kysdk-system/libkysysinfo.h) {
    message("kysysinfo find")
    DEFINES += KY_SDK_SYSINFO
}

exists(/usr/include/kysdk/applications/kaboutdialog.h) {
    message("kyqtwidgets find")
    DEFINES += KY_SDK_QT_WIDGETS
}

exists(/usr/include/kysdk/applications/ukuistylehelper/ukuistylehelper.h) {
    message("kywaylandhelper find")
    DEFINES += KY_SDK_WAYLANDHELPER
}
