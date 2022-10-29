OBJ_DIR=objs
SRC_DIR=src

ALGO_DIR=algos

CFLAGS=-O3 -lm -msse4

BIN_DIR=bin

SRC_FILES := $(wildcard $(SRC_DIR)/$(ALGO_DIR)/*.c)
OBJ_FILES := $(patsubst $(SRC_DIR)/$(ALGO_DIR)/%.c,$(OBJ_DIR)/$(ALGO_DIR)/%.o,$(SRC_FILES))
BIN_FILES := $(patsubst $(SRC_DIR)/$(ALGO_DIR)/%, $(BIN_DIR)/$(ALGO_DIR)/%, $(SRC_FILES:.c=))

$(OBJ_DIR)/$(ALGO_DIR)/%.o: $(SRC_DIR)/$(ALGO_DIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)


$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)/algos/%: $(OBJ_DIR)/algos/%.o
	$(CC) -o $@ $< $(CFLAGS)

all: algos executables

algos: $(BIN_FILES)
executables: $(BIN_DIR)/smart $(BIN_DIR)/test $(BIN_DIR)/show $(BIN_DIR)/select $(BIN_DIR)/textgen   
		
$(BIN_DIR)/%: $(OBJ_DIR)/%.o
	$(CC) -o $@ $< $(CFLAGS)



.PHONY: clean debug
	clean:
		rm -rf $(OBJ_DIR)/$(ALGO_DIR)/*.o $(OBJ_DIR)/*.o