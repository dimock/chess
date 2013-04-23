OBJDIR = ./gcc-x64/objs/
LIBDIR = ./gcc-x64/lib/
OBJS = $(OBJDIR)Board.o $(OBJDIR)CapsGenerator.o $(OBJDIR)CheckDetector.o $(OBJDIR)ChecksGenerator.o $(OBJDIR)EscapeGenerator.o $(OBJDIR)Evaluator.o $(OBJDIR)FastGenerator.o $(OBJDIR)Figure.o $(OBJDIR)FigureDirs.o $(OBJDIR)fpos.o $(OBJDIR)Helpers.o $(OBJDIR)MoveMaker.o $(OBJDIR)MovesGenerator.o $(OBJDIR)MovesTable.o $(OBJDIR)Player.o $(OBJDIR)PlayerSearch.o $(OBJDIR)PlayerVerify.o $(OBJDIR)see.o $(OBJDIR)UsualGenerator.o
HDRS = BasicTypes.h Board.h Evaluator.h Field.h Figure.h FigureDirs.h fpos.h HashTable.h Helpers.h Move.h MovesGenerator.h MovesTable.h Player.h
TARGET = $(LIBDIR)libengine.a

CCC = x86_64-w64-mingw32-g++.exe

CCFLAGS = -O3 -D NDEBUG

engine: $(OBJS)
	mkdir -p $(LIBDIR)
	ar.exe rcs $(TARGET) $(OBJS)

$(OBJDIR)Board.o: Board.cpp $(HDRS)
	mkdir -p $(OBJDIR)
	$(CCC) $(CCFLAGS) Board.cpp -c -o $(OBJDIR)Board.o

$(OBJDIR)CapsGenerator.o: CapsGenerator.cpp $(HDRS)
	$(CCC) $(CCFLAGS) CapsGenerator.cpp -c -o $(OBJDIR)CapsGenerator.o

$(OBJDIR)CheckDetector.o: CheckDetector.cpp $(HDRS)
	$(CCC) $(CCFLAGS) CheckDetector.cpp -c -o $(OBJDIR)CheckDetector.o

$(OBJDIR)ChecksGenerator.o: ChecksGenerator.cpp $(HDRS)
	$(CCC) $(CCFLAGS) ChecksGenerator.cpp -c -o $(OBJDIR)ChecksGenerator.o

$(OBJDIR)EscapeGenerator.o: EscapeGenerator.cpp $(HDRS)
	$(CCC) $(CCFLAGS) EscapeGenerator.cpp -c -o $(OBJDIR)EscapeGenerator.o

$(OBJDIR)Evaluator.o: Evaluator.cpp $(HDRS)
	$(CCC) $(CCFLAGS) Evaluator.cpp -c -o $(OBJDIR)Evaluator.o

$(OBJDIR)FastGenerator.o: FastGenerator.cpp $(HDRS)
	$(CCC) $(CCFLAGS) FastGenerator.cpp -c -o $(OBJDIR)FastGenerator.o

$(OBJDIR)Figure.o: Figure.cpp $(HDRS)
	$(CCC) $(CCFLAGS) Figure.cpp -c -o $(OBJDIR)Figure.o

$(OBJDIR)FigureDirs.o: FigureDirs.cpp $(HDRS)
	$(CCC) $(CCFLAGS) FigureDirs.cpp -c -o $(OBJDIR)FigureDirs.o

$(OBJDIR)fpos.o: fpos.cpp $(HDRS)
	$(CCC) $(CCFLAGS) fpos.cpp -c -o $(OBJDIR)fpos.o

$(OBJDIR)Helpers.o: Helpers.cpp $(HDRS)
	$(CCC) $(CCFLAGS) Helpers.cpp -c -o $(OBJDIR)Helpers.o

$(OBJDIR)MoveMaker.o: MoveMaker.cpp $(HDRS)
	$(CCC) $(CCFLAGS) MoveMaker.cpp -c -o $(OBJDIR)MoveMaker.o

$(OBJDIR)MovesGenerator.o: MovesGenerator.cpp $(HDRS)
	$(CCC) $(CCFLAGS) MovesGenerator.cpp -c -o $(OBJDIR)MovesGenerator.o

$(OBJDIR)MovesTable.o: MovesTable.cpp $(HDRS)
	$(CCC) $(CCFLAGS) MovesTable.cpp -c -o $(OBJDIR)MovesTable.o

$(OBJDIR)Player.o: Player.cpp $(HDRS)
	$(CCC) $(CCFLAGS) Player.cpp -c -o $(OBJDIR)Player.o

$(OBJDIR)PlayerSearch.o: PlayerSearch.cpp $(HDRS)
	$(CCC) $(CCFLAGS) PlayerSearch.cpp -c -o $(OBJDIR)PlayerSearch.o

$(OBJDIR)PlayerVerify.o: PlayerVerify.cpp $(HDRS)
	$(CCC) $(CCFLAGS) PlayerVerify.cpp -c -o $(OBJDIR)PlayerVerify.o

$(OBJDIR)see.o: see.cpp $(HDRS)
	$(CCC) $(CCFLAGS) see.cpp -c -o $(OBJDIR)see.o

$(OBJDIR)UsualGenerator.o: UsualGenerator.cpp $(HDRS)
	$(CCC) $(CCFLAGS) UsualGenerator.cpp -c -o $(OBJDIR)UsualGenerator.o

clean:
	rm $(OBJS) $(TARGET)
