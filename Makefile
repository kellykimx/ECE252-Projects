CC = gcc
CFLAGS = -Wall -g -std=c99 -Iinclude
LDLIBS = -lz

SRC_DIR = src

TARGETS = catpng findpng

OBJS_CATPNG = $(SRC_DIR)/catpng.o $(SRC_DIR)/pnginfo.o $(SRC_DIR)/zutil.o $(SRC_DIR)/crc.o
OBJS_FINDPNG = $(SRC_DIR)/findpng.o $(SRC_DIR)/pnginfo.o $(SRC_DIR)/zutil.o $(SRC_DIR)/crc.o

all: $(TARGETS)

catpng: $(OBJS_CATPNG)
	$(CC) -o $@ $^ $(LDLIBS)

findpng: $(OBJS_FINDPNG)
	$(CC) -o $@ $^ $(LDLIBS)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SRC_DIR)/*.o $(TARGETS)
