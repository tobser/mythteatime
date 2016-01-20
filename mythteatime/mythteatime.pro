include ( ../mythconfig.mak )
include ( ../libs.pro )

QT += network xml sql widgets concurrent
TEMPLATE = lib
CONFIG += debug plugin
TARGET = mythteatime
target.path = $${LIBDIR}/mythtv/plugins
INSTALLS += target

uifiles.path = $${PREFIX}/share/mythtv/themes/default
uifiles.files = theme/teatime-ui.xml theme/alarm_clock.svg


INSTALLS += uifiles

# Input
HEADERS += teatimeui.h data.h timerdata.h
HEADERS += edittimerui.h  selectreoccurrenceui.h

SOURCES += main.cpp teatimeui.cpp data.cpp timerdata.cpp
SOURCES += edittimerui.cpp selectreoccurrenceui.cpp

doxy.target = doc
doxy.commands = doxygen Doxyfile;
doxy.depends =

QMAKE_EXTRA_TARGETS += doxy

macx {
    QMAKE_LFLAGS += -flat_namespace -undefined suppress
}
