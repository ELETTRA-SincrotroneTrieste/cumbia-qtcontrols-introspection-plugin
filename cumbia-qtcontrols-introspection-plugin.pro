isEmpty(INSTALL_ROOT) {
    INSTALL_ROOT = /usr/local/cumbia-libs
}

include($${INSTALL_ROOT}/include/cumbia-qtcontrols/cumbia-qtcontrols.pri)

SOURCES += \
    src/cuintrospectionplugin.cpp

HEADERS += \
    src/cuintrospectionplugin.h

DISTFILES += cumbia-qtcontrols-introspection-plugin.json  \
    README.md

QT       += core gui

TARGET = cumbia-qtcontrols-introspection-plugin
TEMPLATE = lib
CONFIG += plugin c++17
VERSION = 1.1.0


CONFIG += debug
DEFINES -= QT_NO_DEBUG_OUTPUT

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


INC_PATH = $${INSTALL_ROOT}/include/qumbia-plugins
inc.files = src/cuintrospectionplugin.h
inc.path = $${INC_PATH}


unix {
    target.path = $${DEFINES_CUMBIA_QTCONTROLS_PLUGIN_DIR}
    INSTALLS += target inc
}

message("cumbia-qtcontrols-introspection-plugin: plugin installation dir:  $${DEFINES_CUMBIA_QTCONTROLS_PLUGIN_DIR}")
message("cumbia-qtcontrols-introspection-plugin: include installation dir: $${INC_PATH}")

