TARGET= tor-runner

SRC= main.cc fuse.cc

OBJ= $(SRC:.cc=.o)

FLAGS= -std=c++11

INCLUDE= $(shell pkg-config fuse --cflags)
LIB= $(shell pkg-config fuse --libs --static) -lpthread

LD= g++
CXX= g++

.PHONY: all clean

all: $(TARGET)

clean:
	rm -f $(TARGET) $(OBJ)

clean-all: clean
	cd lib/uv && make clean

run: $(TARGET)
	mkdir -p /tmp/torr
	mkdir -p /tmp/torr-target
	mkdir -p /tmp/torr-watch
	./$(TARGET) /tmp/torr /tmp/torr-target /tmp/torr-watch

$(TARGET): $(OBJ)
	$(LD) $(FLAGS) -o $(TARGET) $(OBJ) $(LIB)

.cc.o:
	$(CXX) $(FLAGS) $(INCLUDE) -o $@ -c $<
