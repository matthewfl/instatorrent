TARGET= tor-runner

SRC= main.cc fuse.cc torrent.cc

OBJ= $(SRC:.cc=.o)

FLAGS= -std=c++11

TORRENT=deps/lib/libtorrent-rasterbar.a

INCLUDE= $(shell pkg-config fuse --cflags) $(shell pkg-config libtorrent-rasterbar --cflags --static)
#-Ideps/include
LIB= $(shell pkg-config fuse --libs --static) $(shell pkg-config libtorrent-rasterbar --libs --static)
#$(TORRENT) -lboost_system -lpthread -lssl -lcrypto
#-lboost_system

LD= g++
CXX= g++

.PHONY: all clean

all: $(TARGET)

clean:
	rm -f $(TARGET) $(OBJ)

clean-all: clean
	cd lib/libtorrent && make clean
	rm -rf lib/lib lib/include

run: $(TARGET)
	mkdir -p /tmp/torr
	mkdir -p /tmp/torr-target
	mkdir -p /tmp/torr-watch
	./$(TARGET) /tmp/torr /tmp/torr-target /tmp/torr-watch

$(TARGET): $(OBJ) $(TORRENT)
	$(LD) $(FLAGS) -o $(TARGET) $(OBJ) $(LIB)

.cc.o:
	$(CXX) $(FLAGS) $(INCLUDE) -o $@ -c $<


$(TORRENT): $(wildcard lib/libtorrent/include/*)
	cd deps/libtorrent && make clean
	cd deps/libtorrent && ./configure --prefix=`pwd`/../
	cd deps/libtorrent && make
	cd deps/libtorrent && make install
