OBJ_DIR=objs
SRC_DIR=src

ALGO_DIR=algos

CFLAGS=-O3 -lm -msse4 -ldl

BIN_DIR=bin

SRC_FILES=$(wildcard $(SRC_DIR)/$(ALGO_DIR)/*.c)
OBJ_FILES=$(patsubst $(SRC_DIR)/$(ALGO_DIR)/%.c,$(OBJ_DIR)/$(ALGO_DIR)/%.o,$(SRC_FILES))
ALGO_LIBS=$(patsubst $(SRC_DIR)/$(ALGO_DIR)/%, $(BIN_DIR)/$(ALGO_DIR)/%, $(SRC_FILES:.c=.so))

dir_guard=@mkdir -p $(@D)

all: algos smart

smart: $(BIN_DIR)/smart
algos: $(ALGO_LIBS)


$(OBJ_DIR)/$(ALGO_DIR)/%.o: $(SRC_DIR)/$(ALGO_DIR)/%.c
	$(dir_guard)
	$(CC) -c -o $@ -fPIC $< $(CFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(dir_guard)
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)/smart.o: $(SRC_DIR)/smart.c $(SRC_DIR)/*.h
	$(dir_guard)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)/$(ALGO_DIR)/%.so: $(OBJ_DIR)/$(ALGO_DIR)/%.o
	$(dir_guard)
	$(CC) -shared -o $@ $< $(CFLAGS)
 
$(BIN_DIR)/smart: $(OBJ_DIR)/smart.o $(OBJ_DIR)/string_set.o
	$(dir_guard)
	$(CC) -g -o $@ $^ $(CFLAGS)


.PHONY: clean

clean:
	rm -rf $(OBJ_DIR)