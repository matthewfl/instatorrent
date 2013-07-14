#include "torrent.h"

#include <assert.h>
#include <iostream>
using namespace std;

using namespace libtorrent;

Torrents::Torrents(char *_target_dir, char *_watch_dir) : target_dir(_target_dir), watch_dir(_watch_dir) {

}


void Torrents::Start() {
  cerr << "torrent downloader starting\n";

  //  torrent.info = torrent_info("

}


Torrents::Torrent::Torrent() : info ("testing.torrent") {

}
