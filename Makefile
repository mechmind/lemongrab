CC=g++
CFLAGS=-c -Wall -std=c++11
LDFLAGS=-lgloox -lpthread

SRC=$(wildcard *.cpp)
OBJ=$(SRC:.cpp=.o)
EXE=lemongrab

all: $(SRC) $(EXE)

clean:
	rm $(EXE)
	rm $(OBJ)

install:
	cp config.ini.default config.ini

$(EXE): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@
