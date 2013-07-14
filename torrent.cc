#include "torrent.h"

#include <assert.h>
#include <iostream>
using namespace std;


Torrent::Torrent(char *_target_dir, char *_watch_dir) : target_dir(_target_dir), watch_dir(_watch_dir) {

}


void Torrent::Start() {
  cerr << "torrent downloader starting\n";
}
