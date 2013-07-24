#include <iostream>

#include "fuse.h"
#include "torrent.h"
#include "magnet.h"

#include <thread>
#include <string>


using namespace std;

#define usage					\
cerr << "Usage: " << argv[0] << " download [fuse dir] [target dir] [watch dir]" << endl \
<< argv[0] << " magnet [target dir]" << endl;



int main(int argc, char **argv) {

  cerr << "Version: " << GIT_VERSION << endl;

  if(argc == 1) {
    usage;
    return 1;
  }

  string mode = argv[1];
  if(mode == "magnet") {
    if(argc != 3) {
      usage;
      return 1;
    }

    Magnet magnet(argv[2]);

    magnet.Start();

    return 0;
  }else if(mode == "download") {
    if(argc != 5) {
      usage;
      return 1;
    }
    Torrents torrent(argv[3], argv[4]);

    thread downloader ([&torrent]() {
	torrent.Start();
      });


    Fuse fuse (argv[2], argv[3], &torrent);

    fuse.Start();

    torrent.Stop();

    downloader.join();

    return 0;
  }else{
    cerr << "mode not found\n";
    usage;
    return 1;
  }
}
