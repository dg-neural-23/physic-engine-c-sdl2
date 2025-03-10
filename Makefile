CC = gcc
CFLAGS = -Wall -g $(shell PKG_CONFIG_PATH=/usr/lib/pkgconfig pkg-config --cflags SDL2_ttf)
LIBS = -lSDL2 -lm $(shell PKG_CONFIG_PATH=/usr/lib/pkgconfig pkg-config --libs SDL2_ttf)
TARGET = raytracing
SRC = raytracing.c

all: $(TARGET)

$(TARGET): $(SRC)
		$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LIBS)

clean:
		rm -f $(TARGET)