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

ifndef AR
AR=ar.exe
endif

ifndef ARFLAGS
ARFLAGS=rcs
endif

ifeq "$(CFG)" "Release"
CFLAGS=  -O2 -W -fno-inline  -mcpu=pentiumpro -D_M_IX86=500 -fexceptions -D_THREAD_SAFE -D_LINUX  -DNDEBUG -D_LIB -D_CRT_SECURE_NO_WARNINGS
ILFLAGS= 
TARGET=engine.lib
endif

ifeq "$(CFG)" "Debug"
CFLAGS= -g -O0 -W -fno-inline  -mcpu=pentiumpro -D_M_IX86=500 -fexceptions -D_THREAD_SAFE -D_LINUX  -D_DEBUG -D_LIB -D_CRT_SECURE_NO_WARNINGS
ILFLAGS= 
TARGET=engine.lib
endif

SOURCE_FILES= 	\
	BasicTypes.cpp	\
	ChessAlg.cpp	\
	ChessBoard.cpp	\
	ChessMove.cpp	\
	ChessVerify.cpp	\
	debut.cpp	\
	Evaluation.cpp	\
	Figure.cpp	\
	FigureDirs.cpp	\
	Helpers.cpp	\
	MovesPrecounter.cpp

HEADER_FILES= 	\
	BasicTypes.h	\
	ChessAlg.h	\
	ChessBoard.h	\
	debut.h	\
	Field.h	\
	Figure.h	\
	FigureDirs.h	\
	HashTable.h	\
	Helpers.h	\
	MovesPrecounter.h

OBJS=$(patsubst %.cpp, %.obj,$(SOURCE_FILES))

.PHONY:all
all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

%.obj: %.cpp
	$(COMPILER) $(CFLAGS) $(ILFLAGS) $(GLOBALS) -o $@ -c $<

.PHONY:clean
clean:
	 -rm -f $(OBJS) $(TARGET)

