CC := clang
CFLAGS := -fvisibility=hidden -std=c99 -O2 -Isrc -g

LDFLAGS :=
LIBRARIES := `pkg-config --cflags --libs allegro-5 allegro_ttf-5 allegro_font-5 allegro_color-5 allegro_main-5 allegro_primitives-5`

SOURCES := $(wildcard src/*.c)
OBJECTS := $(SOURCES:.c=.o)
EDITOR_EXEC := ../editor

.PHONY: all clean

all: $(EDITOR_EXEC)

run: $(EDITOR_EXEC)
	exec ./test.sh

clean:
	$(RM) $(OBJECTS) $(EDITOR_EXEC)

$(EDITOR_EXEC): $(OBJECTS)
	$(CC) $(LDFLAGS) $(CFLAGS) $(OBJECTS) -o $@ $(LIBRARIES)

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@
