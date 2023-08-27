QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Singleton.cpp \
    common.cpp \
    consumer.cpp \
    dataprogress.cpp \
    des.c \
    downloadlayout.cpp \
    downloadtask.cpp \
    filepropertyinfo.cpp \
    login.cpp \
    main.cpp \
    main_closewidget.cpp \
    main_topwidget.cpp \
    mainwindow.cpp \
    mainwindows_rightwidget.cpp \
    myfilewg.cpp \
    mymenu.cpp \
    rankinglist.cpp \
    sharelist.cpp \
    titlewg.cpp \
    transfer.cpp \
    uploadlayout.cpp \
    uploadtask.cpp    

HEADERS += \
    Singleton.h \
    common.h \
    consumer.h \
    dataprogress.h \
    des.h \
    downloadlayout.h \
    downloadtask.h \
    filepropertyinfo.h \
    login.h \
    main_closewidget.h \
    main_topwidget.h \
    mainwindow.h \
    mainwindows_rightwidget.h \
    myfilewg.h \
    mymenu.h \
    rankinglist.h \
    sharelist.h \
    titlewg.h \
    transfer.h \
    uploadlayout.h \
    uploadtask.h

FORMS += \
    dataprogress.ui \
    filepropertyinfo.ui \
    login.ui \
    main_closewidget.ui \
    main_topwidget.ui \
    mainwindow.ui \
    mainwindows_rightwidget.ui \
    myfilewg.ui \
    rankinglist.ui \
    sharelist.ui \
    titlewg.ui \
    transfer.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    qrc.qrc


DISTFILES +=
