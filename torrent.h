#ifndef _tor_torrent
#define _tor_torrent

#define BOOST_ASIO_SEPARATE_COMPILATION

#include <libtorrent/entry.hpp>

class Torrent {
public:

  Torrent(char *_target_dir, char *_watch_dir);

  void Start();


private:
  char *target_dir;
  char *watch_dir;

};

#endif
