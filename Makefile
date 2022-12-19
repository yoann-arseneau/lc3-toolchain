.PHONY: all clean hello link

# Setup Variables
CFLAGS =  -std=c18
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Wpointer-arith
CFLAGS += -Wcast-align
CFLAGS += -Wwrite-strings
CFLAGS += -Wswitch-default
CFLAGS += -Wunreachable-code
CFLAGS += -Winit-self
CFLAGS += -Wmissing-field-initializers
CFLAGS += -Wno-unknown-pragmas
CFLAGS += -Wstrict-prototypes
CFLAGS += -Wundef
CFLAGS += -Wold-style-definition
CFLAGS += -Wno-misleading-indentation
CFLAGS += -Wfatal-errors
CFLAGS += -pedantic

CC = gcc $(CFLAGS)
LNK = gcc
SRC = src
OUT = out

# PHONY Targets
all: $(OUT)/lc3asm
clean:
	@rm -rf ./$(OUT)/*
	@find $(SRC) -name '*.gch' -type f -delete
	@echo Cleaned \'./$(OUT)\' and pre-compiled header files
hello: $(OUT)/hello.obj
	@hexdump -C $(OUT)/hello.obj
link: $(OUT)/lc3link $(OUT)/main.obj $(OUT)/data.obj
	$(OUT)/lc3link $^

# LC3 Object Files
$(OUT)/hello.obj: examples/hello/hello.asm $(OUT)/lc3asm
$(OUT)/main.obj:  examples/link/main.asm $(OUT)/lc3asm
$(OUT)/data.obj:  examples/link/data.asm $(OUT)/lc3asm
$(OUT)/hello.obj $(OUT)/main.obj $(OUT)/data.obj:
	@mkdir -p $(OUT)
	$(OUT)/lc3asm $< >$@

# Tool-Chain Artifacts
$(OUT)/lc3asm: $(OUT)/lc3asm.o $(OUT)/lc3log.o $(OUT)/lc3lex.o $(OUT)/lc3tok.o $(OUT)/lc3cu.o
$(OUT)/lc3asm:
	@mkdir -p $(OUT)
	$(LNK) $^ -o $@

# Tool-Chain Object Files
$(OUT)/lc3asm.o: $(SRC)/lc3asm.c $(SRC)/lc3asm.h.gch
$(OUT)/lc3log.o: $(SRC)/lc3log.c $(SRC)/lc3asm.h.gch
$(OUT)/lc3lex.o: $(SRC)/lc3lex.c $(SRC)/lc3asm.h.gch
$(OUT)/lc3tok.o: $(SRC)/lc3tok.c $(SRC)/lc3asm.h.gch
$(OUT)/lc3cu.o:  $(SRC)/lc3cu.c $(SRC)/lc3asm.h.gch
$(OUT)/lc3asm.o $(OUT)/lc3link.o $(OUT)/lc3log.o $(OUT)/lc3lex.o $(OUT)/lc3tok.o $(OUT)/lc3cu.o:
	@mkdir -p $(OUT)
	$(CC) $< -c -o $@

# Pre-Compiled Header
$(SRC)/lc3asm.h.gch: $(SRC)/lc3asm.h $(SRC)/lc3log.h $(SRC)/lc3lex.h $(SRC)/lc3tok.h $(SRC)/lc3cu.h
	$(CC) $<

