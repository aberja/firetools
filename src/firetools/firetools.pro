QMAKE_CXXFLAGS += $$(CXXFLAGS) -fstack-protector-all -D_FORTIFY_SOURCE=2 -fPIE -pie -Wformat -Wformat-security
QMAKE_CFLAGS += $$(CFLAGS) -fstack-protector-all -D_FORTIFY_SOURCE=2 -fPIE -pie -Wformat -Wformat-security
QMAKE_LFLAGS += $$(LDFLAGS) -Wl,-z,relro -Wl,-z,now
QT += widgets
 HEADERS       = mainwindow.h ../common/utils.h ../common/common.h applications.h \
		  firetools.h
 SOURCES       = mainwindow.cpp \
                 main.cpp \
                  ../common/utils.cpp \
                  ../common/pid.cpp \
                  applications.cpp
RESOURCES = firetools.qrc
TARGET=../../build/firetools
