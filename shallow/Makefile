OBJDIR = ./objs/
EXEDIR = ./exe/
OBJS = $(OBJDIR)shallow.o $(OBJDIR)Thinking.o $(OBJDIR)xboard.o $(OBJDIR)xparser.o
HDRS = Thinking.h xparser.h xboard.h xcommands.h
TARGET = $(EXEDIR)shallow.exe

CCC = x86_64-w64-mingw32-g++.exe

CCFLAGS = -static -s -O2 -I../engine

shallow: $(OBJS)
	$(CCC) -s -O2 -static $(OBJS) -L../engine/lib/ -lengine -o $(TARGET)

$(OBJDIR)shallow.o: shallow.cpp $(HDRS)
	$(CCC) $(CCFLAGS) shallow.cpp -c -o $(OBJDIR)shallow.o

$(OBJDIR)Thinking.o: Thinking.cpp $(HDRS)
	$(CCC) $(CCFLAGS) Thinking.cpp -c -o $(OBJDIR)Thinking.o

$(OBJDIR)xboard.o: xboard.cpp $(HDRS)
	$(CCC) $(CCFLAGS) xboard.cpp -c -o $(OBJDIR)xboard.o

$(OBJDIR)xparser.o: xparser.cpp $(HDRS)
	$(CCC) $(CCFLAGS) xparser.cpp -c -o $(OBJDIR)xparser.o
	
clean:
	rm $(OBJS) $(TARGET)
