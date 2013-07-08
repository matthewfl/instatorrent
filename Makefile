TARGET= tor-runner

SRC= main.cc fuse.cc

OBJ= $(SRC:.cc=.o)

FLAGS= -std=c++11

INCLUDE= $(shell pkg-config fuse --cflags)
LIB= $(shell pkg-config fuse --libs --static)

LD= g++
CXX= g++

.PHONY: all clean

all: $(TARGET)

clean:
	rm -f $(TARGET) $(OBJ)

run: $(TARGET)
	mkdir -p /tmp/torr
	./$(TARGET) -f /tmp/torr

$(TARGET): $(OBJ)
	$(LD) $(FLAGS) -o $(TARGET) $(LIB) $(OBJ)

.cc.o:
	$(CXX) $(FLAGS) $(INCLUDE) -o $@ -c $<
