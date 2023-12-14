ASAN    := -fsanitize=address -fsanitize=undefined
CC      := clang 
DEBUGGER:= lldb
CFLAGS  :=  -g $(ASAN) -Wall -Wextra -Werror -pedantic -Wshadow -Wno-error=shadow -Wno-unused-parameter -Wno-error=unused-variable \
				-Wno-format-security
BIN     :=  main
LIBS    := -lncurses
LDFLAGS := $(LIBS) $(ASAN) -rdynamic
OBJPATH := obj

ifndef VERBOSE
.SILENT:
endif

cfiles:=$(shell echo src/*.c)
objfiles:=$(cfiles:src/%.c=obj/%.o)

.PHONY: clean run all debug release 

all:CFLAGS:=$(CFLAGS) -DDEBUG
all:$(OBJPATH) run testcode
run:$(BIN)
	./$(BIN)
	

release: CFLAGS:=$(CFLAGS) -O2
release: clean
release: $(BIN) 

debug: CFLAGS:=$(CFLAGS) -DDEBUG
debug: clean
debug: $(BIN) 
debug:
	$(DEBUGGER) $(BIN)

$(OBJPATH):
	mkdir $(OBJPATH)

$(OBJPATH)/%.o:src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN):$(objfiles) 
	$(CC) $(objfiles) $(LDFLAGS) -o $@
bear:
	bear -- make 
clean:
	rm -f $(OBJPATH)/*.o
cleanall: clean bear
