include ../Makefile.vars

TARGETDIR = ./$(LIBDIR)
PROJECT = engine
TARGET = $(TARGETDIR)lib$(ENGINE_LIB).a

COMPILE = $(CCC) $(CCFLAGS) 

include ../Makefile.rules

$(TARGET): $(OBJ)
	ar.exe rcs $(TARGET) $(OBJ)
