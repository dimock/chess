PREFIX = gcc-x64
LIBDIR = $(PREFIX)/lib/
OBJDIR = ./$(PREFIX)/objs/
OLIBDIR = ./$(LIBDIR)
ENGINE_LIB = engine
CPP_FILES = $(wildcard *.cpp, *.cpp)
HDRS = $(wildcard *.h, *.h)
OBJ_FILES = $(patsubst %.cpp, %.o, $(CPP_FILES))
OBJS = $(addprefix $(OBJDIR), $(OBJ_FILES))
PROJECT = engine
TARGET = $(OLIBDIR)lib$(ENGINE_LIB).a


CCC = x86_64-w64-mingw32-g++.exe
CCFLAGS = -static -s -c -O3 -D NDEBUG
COMPILE = $(CCC) $(CCFLAGS) 

.PHONY : $(PROJECT)
$(PROJECT): $(TARGET)

$(TARGET): $(OBJS)
	ar.exe rcs $(TARGET) $(OBJS)

$(OBJS): |$(OBJDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(TARGET): |$(OLIBDIR)

$(OLIBDIR):
	mkdir -p $(OLIBDIR)

$(OBJDIR)%.o: %.cpp $(HDR)
	$(COMPILE) $< -o $@

.PHONY : clean	
clean:
	rm -f $(OBJS) $(TARGET)

