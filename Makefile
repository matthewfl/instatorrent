TARGET= tor-runner

SRC= main.cc fuse.cc torrent.cc magnet.cc

OBJ= $(SRC:.cc=.o)

GIT_VER=$(shell git describe --always --long --abbrev=20 --dirty)

# fml std::auto_ptr depreciated == so many warnings
FLAGS_BASE= -std=c++11 -DGIT_VERSION=\"$(GIT_VER)\" -w
FLAGS= $(FLAGS_BASE) -ggdb

TORRENT=deps/lib/libtorrent-rasterbar.a

PKG_CONFIG_PATH=./deps/lib/pkgconfig

INCLUDE= $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config fuse libtorrent-rasterbar --cflags)
LIB= ./deps/lib/libtorrent-rasterbar.a -lboost_system -lssl -lcrypto $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config fuse --libs --static)


LD= g++
CXX= g++

.PHONY: all clean

all: $(TARGET)

release: FLAGS= $(FLAGS_BASE) -s -O2
release: clean all

clean:
	rm -f $(TARGET) $(OBJ)

clean-all: clean
	cd deps/libtorrent && make clean
	rm -rf deps/lib deps/include

_dirs:
	mkdir -p /tmp/torr
	mkdir -p /tmp/torr-target
	mkdir -p /tmp/torr-watch


run: $(TARGET) _dirs
	./$(TARGET) download /tmp/torr /tmp/torr-target /tmp/torr-watch

debug: $(TARGET) _dirs
	gdb $(TARGET) "--eval-command=run /tmp/torr /tmp/torr-target /tmp/torr-watch" --eval-command=bt

$(TARGET): $(OBJ) $(TORRENT)
	$(LD) $(FLAGS) -o $(TARGET) $(OBJ) $(LIB)

.cc.o:
	$(CXX) $(FLAGS) $(INCLUDE) -o $@ -c $<


$(TORRENT): $(wildcard lib/libtorrent/include/*)
	cd deps/libtorrent && make clean
	cd deps/libtorrent && ./configure --prefix=`pwd`/../ --disable-geoip
	cd deps/libtorrent && make V=1
	cd deps/libtorrent && make install
	cd deps/libtorrent && make clean


depend:
	makedepend -Y -- $(FLAGS) -- $(SRC)
# DO NOT DELETE

main.o: fuse.h torrent.h magnet.h
fuse.o: fuse.h torrent.h
torrent.o: torrent.h
magnet.o: magnet.h torrent.h
