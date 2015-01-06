LIB = -lswscale -lavformat -lavcodec -lswresample -lm -lz -lSDL -L/usr/local/ffmpeg/lib -L/usr/local/SDL/lib
INC = -I/usr/local/ffmpeg/include -I/usr/local/SDL/include/SDL 
CC = gcc
CXX = g++
CFLAGS = -g

all:
	$(CC) $(CFLAGS) -o splay_avsync splay_avsync.c $(LIB) $(INC)
clean:
	rm splay_avsync
