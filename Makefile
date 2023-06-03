CC=gcc
CFLAGS=-O2
BIN=maze

$(BIN): $(BIN).c
	$(CC) $(BIN).c -o $(BIN) $(CFLAGS)

clean: $(BIN)
	rm $(BIN)

run: $(BIN)
	./$(BIN) $(MAP)
