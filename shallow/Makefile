PREFIX = gcc-x64
LIBDIR = $(PREFIX)/lib/
OBJDIR = ./$(PREFIX)/objs/
OUTDIR = ./$(PREFIX)/release/
ENGINE_DIR = ../engine/
ENGINE_LIBDIR = $(ENGINE_DIR)$(LIBDIR)
ENGINE_LIB = engine
CPP_FILES = $(wildcard *.cpp, *.cpp)
HDRS = $(wildcard *.h, *.h)
OBJ_FILES = $(patsubst %.cpp, %.o, $(CPP_FILES))
OBJS = $(addprefix $(OBJDIR), $(OBJ_FILES))
PROJECT = shallow
TARGET = $(OUTDIR)$(PROJECT)-$(PREFIX).exe


CCC = x86_64-w64-mingw32-g++.exe
CCFLAGS = -static -s -c -O3 -D NDEBUG -I$(ENGINE_DIR)
COMPILE = $(CCC) $(CCFLAGS) 

.PHONY : $(PROJECT)
$(PROJECT): $(TARGET)

$(TARGET): $(OBJS) $(ENGINE_LIBDIR)lib$(ENGINE_LIB).a
	$(CCC) -s -O3 -static $(OBJS) -L$(ENGINE_LIBDIR) -l$(ENGINE_LIB) -o $(TARGET)

$(OBJS): |$(OBJDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(TARGET): |$(OUTDIR)

$(OUTDIR):
	mkdir -p $(OUTDIR)

$(OBJDIR)%.o: %.cpp $(HDR)
	$(COMPILE) $< -o $@

.PHONY : clean	
clean:
	rm -f $(OBJS) $(TARGET)
