#
# ICD AS (C) 2007 - MAKEFILE CREATED USING CDP MAKEFILE WIZARD
#
# Make sure that the environment variable CDPBase exists. This can
# be carried out by typing echo $CDPBase at the command promt, where
# the Makefile should be run by make. Note that the Makefile Wizard
# is created for the purpose of CDP Projects. It is, however, fully
# possible to use it for other project types as well.
#
# - To add a profiler in your executable, compile and link with, -pg.
# - To open/use shared libs at runtime, compile -rdynamic and link -ldl.
# - Make sure that a valid CDPLicense.key resides in $CDPBase/License.
# - Run CDP executables as root.


ifndef CFG
CFG=Release
endif

ifndef COMPILER
COMPILER=g++.exe
endif

ifeq "$(CFG)" "Release"
CFLAGS=  -O2 -W -fno-inline  -mcpu=pentiumpro -D_M_IX86=500 -fexceptions -D_THREAD_SAFE -D_LINUX  -DNDEBUG -D_CONSOLE -D_CRT_SECURE_NO_WARNINGS
LIBS= -lpthread -lm -lengine
ILFLAGS= -I../engine -L../engine
TARGET=tequilla.exe
endif

ifeq "$(CFG)" "Debug"
CFLAGS= -g -O0 -W -fno-inline  -mcpu=pentiumpro -D_M_IX86=500 -fexceptions -D_THREAD_SAFE -D_LINUX  -D_DEBUG -D_CONSOLE -D_CRT_SECURE_NO_WARNINGS
LIBS= -lCDP_Debug -lrt -lpthread -lm 
ILFLAGS= -I../engine
TARGET=tequilla.exe
endif

SOURCE_FILES= 	\
	StopTimer.cpp	\
	tequilla.cpp	\
	Thinking.cpp	\
	xparser.cpp

HEADER_FILES= 	\
	resource.h	\
	StopTimer.h	\
	Thinking.h	\
	xcommands.h	\
	xparser.h

OBJS=$(patsubst %.cpp, %.obj,$(SOURCE_FILES))

.PHONY:all
all: $(TARGET)

$(TARGET): $(OBJS)
	$(COMPILER) $(CFLAGS) $(ILFLAGS) $(GLOBALS) $(OBJS) -o $(TARGET) $(LIBS)

%.obj: %.cpp
	$(COMPILER) $(CFLAGS) $(ILFLAGS) $(GLOBALS) -o $@ -c $<

.PHONY:clean
clean:
	 -rm -f $(OBJS) $(TARGET)

