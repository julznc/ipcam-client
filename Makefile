PROGRAM  := ipcam-client

SOURCES = main.c utils.c bubble.c session.c media.c display.c

CFLAGS   := -I. -Wall -O2 -g
LFLAGS   := -L/usr/local/lib -lavcodec -lavutil -lswscale -lX11

OBJECTS := $(patsubst %.c, %.o, $(SOURCES))

all : $(OBJECTS)
	$(CXX) -o $(PROGRAM) $^ $(LFLAGS)

clean:
	rm -rf *.o *.exe
