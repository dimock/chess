include ../Makefile.vars

TARGETDIR = ./$(PREFIX)/release/
ENGINE_DIR = ../engine/
ENGINE_LIBDIR = $(ENGINE_DIR)$(LIBDIR)
PROJECT = shallow
TARGET = $(TARGETDIR)$(PROJECT)-$(PREFIX).exe

COMPILE = $(CCC) $(CCFLAGS) -I$(ENGINE_DIR)

include ../Makefile.rules

$(TARGET): $(OBJ) $(ENGINE_LIBDIR)lib$(ENGINE_LIB).a
	$(CCC) -s -O3 -static $(OBJ) -L$(ENGINE_LIBDIR) -l$(ENGINE_LIB) -o $(TARGET)
