PROGRAM  := ipcam-client

SOURCES = main.c utils.c session.c

CFLAGS   := -I. -Wall -O2 -g
LFLAGS   :=

OBJECTS := $(patsubst %.c, %.o, $(SOURCES))

all : $(OBJECTS)
	$(CXX) -o $(PROGRAM) $^

clean:
	rm -rf *.o *.exe
