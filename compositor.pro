DEFINES += QT_COMPOSITOR_QUICK

LIBS += -L ../../lib

QT += quick qml v8
QT += quick-private

QT += compositor


SOURCES += main.cpp


RESOURCES += \
    qml.qrc


