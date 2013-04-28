ENGINE_DIR = ./engine/
SHALLOW_DIR = ./shallow/
CC =
PREFIX =
ifeq ($(cc), x64)
	CC = x86_64-w64-mingw32-g++.exe
	PREFIX = gcc-x64
else
	CC = g++.exe
	PREFIX = gcc-x86
endif

subsystem:
	$(MAKE) -C $(ENGINE_DIR) CCC=$(CC) PREFIX=$(PREFIX)
	$(MAKE) -C $(SHALLOW_DIR) CCC=$(CC) PREFIX=$(PREFIX)
	
.PHONY: clean
clean:
	$(MAKE) -C $(ENGINE_DIR) CCC=$(CC) PREFIX=$(PREFIX) clean
	$(MAKE) -C $(SHALLOW_DIR) CCC=$(CC) PREFIX=$(PREFIX) clean
