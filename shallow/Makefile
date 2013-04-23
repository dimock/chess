OBJDIR = ./gcc-x64/objs/
OUTDIR = ./gcc-x64/
OBJS = $(OBJDIR)shallow.o $(OBJDIR)Thinking.o $(OBJDIR)xboard.o $(OBJDIR)xparser.o
HDRS = Thinking.h xparser.h xboard.h xcommands.h
TARGET = $(OUTDIR)shallow-gcc-x64.exe

CCC = x86_64-w64-mingw32-g++.exe

CCFLAGS = -static -s -O3 -D NDEBUG -I../engine/

shallow: $(OBJS)
	mkdir -p $(OUTDIR)
	$(CCC) -s -O2 -static $(OBJS) -L../engine/gcc-x64/lib/ -lengine -o $(TARGET)

$(OBJDIR)shallow.o: shallow.cpp $(HDRS)
	mkdir -p $(OBJDIR)
	$(CCC) $(CCFLAGS) shallow.cpp -c -o $(OBJDIR)shallow.o

$(OBJDIR)Thinking.o: Thinking.cpp $(HDRS)
	$(CCC) $(CCFLAGS) Thinking.cpp -c -o $(OBJDIR)Thinking.o

$(OBJDIR)xboard.o: xboard.cpp $(HDRS)
	$(CCC) $(CCFLAGS) xboard.cpp -c -o $(OBJDIR)xboard.o

$(OBJDIR)xparser.o: xparser.cpp $(HDRS)
	$(CCC) $(CCFLAGS) xparser.cpp -c -o $(OBJDIR)xparser.o
	
clean:
	rm $(OBJS) $(TARGET)
