CC = gcc
LIBJIT_DIR = libjit/include
LIBJIT_BIN = libjit/jit/.libs/libjit.a

BIN = bin

PARSER_SRC += $(BIN)/parser.c
PARSER_SRC += $(BIN)/parser.h
PARSER_SRC += $(BIN)/lexer.c

OBJECTS += $(BIN)/parser.o
OBJECTS += $(BIN)/tokens.o
OBJECTS += $(BIN)/constants.o
OBJECTS += $(BIN)/regexjit.o

RESULT += bin/libregexjit.a

EXTERN_LIBS += -lm
EXTERN_LIBS += -ldl
EXTERN_LIBS += -lpthread
EXTERN_LIBS += -lfl
EXTERN_LIBS += $(LIBJIT_BIN)

CFLAGS = -I$(LIBJIT_DIR) -std=c99

all: CFLAGS += -O2 -g
all: $(RESULT)

debug: CFLAGS += -g
debug: $(RESULT)

release: CFLAGS += -O2
release: $(RESULT)

clean:
	if [ -d $(BIN) ]; then rm -r $(BIN); fi

clean-deps:
	$(MAKE) -C libjit clean

$(RESULT): $(LIBJIT_BIN) $(BIN) $(OBJECTS)
	ar rcs $(BIN)/libregexjit.a $(OBJECTS)

example: $(RESULT)
	$(CC) -g example.c $(RESULT) $(LIBJIT_BIN) -o bin/example $(EXTERN_LIBS)

$(BIN):
	mkdir $(BIN)

$(LIBJIT_BIN):
	cd libjit && ./bootstrap
	cd libjit && ./configure
	$(MAKE) -C libjit

bin/parser.o bin/tokens.o: src/parser.y src/tokens.lex
	bison src/parser.y --defines=src/parser.h --output=src/parser.c
	flex --outfile=src/tokens.c --header-file=src/tokens.h src/tokens.lex
	$(CC) $(CFLAGS) -c src/parser.c -o bin/parser.o
	$(CC) $(CFLAGS) -c src/tokens.c -o bin/tokens.o
	rm src/parser.c src/tokens.c

$(BIN)/%.o: src/%.c $(LIBJIT_BIN)
	$(CC) $(CFLAGS) -c $< -o $@
