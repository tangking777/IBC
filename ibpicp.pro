QT += qml quick core widgets printsupport
include(src/Xlsx/qtxlsx.pri)
QMAKE_CXXFLAGS += /utf-8
SOURCES += main.cpp \
    src/CustomPlot/qcustomplot.cpp \
    src/FileFuncs.cpp \
    src/IbpicpDataControl.cpp \
    src/qmlplot.cpp
HEADERS += \
    src/CustomPlot/qcustomplot.h \
    src/FileFuncs.h \
    src/IbpicpDataControl.h \
    src/qmlplot.h
RESOURCES += ibpicp.qrc
RC_FILE = myapp.rc
