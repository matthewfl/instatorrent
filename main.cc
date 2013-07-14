#include <iostream>

#include "fuse.h"
#include "torrent.h"

#include <thread>


using namespace std;



int main(int argc, char **argv) {

  if(argc != 4) {
    cerr << "Usage: " << argv[0] << " [fuse dir] [target dir] [watch dir]";
      return 1;
  }


  Fuse fuse (argv[1], argv[2]);

  Torrent torrent(argv[2], argv[3]);

  thread downloader ([&torrent]() {
      torrent.Start();
    });

  fuse.Start();

  downloader.join();

  return 0;
}
