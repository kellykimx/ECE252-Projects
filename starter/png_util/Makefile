CC = gcc       # compiler
CFLAGS = -Wall -g -std=c99 # compilation flags
LD = gcc       # linker
LDFLAGS = -g   # debugging symbols in build
LDLIBS = -lz   # link with libz

SRC_DIR = src
UTIL_DIR = starter/png_util

UTIL_OBJS = $(UTIL_DIR)/zutil.o $(UTIL_DIR)/crc.o

TARGETS = catpng findpng

all: $(TARGETS)

catpng: $(SRC_DIR)/catpng.o $(SRC_DIR)/pnginfo.o $(UTIL_OBJS)
	$(CC) -o $@ $^ $(LDLIBS)

findpng: $(SRC_DIR)/findpng.o $(UTIL_OBJS)
	$(CC) -o $@ $^ $(LDLIBS)   # ← This line starts with a tab, not spaces

# Pattern rule for object files
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(UTIL_DIR)/%.o: $(UTIL_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SRC_DIR)/*.o $(UTIL_DIR)/*.o $(TARGETS)
