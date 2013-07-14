#ifndef _tor_torrent
#define _tor_torrent

//#define BOOST_ASIO_SEPARATE_COMPILATION

#include <libtorrent/entry.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/bencode.hpp>


class Torrents {
public:

  class Torrent {
  public:
    libtorrent::torrent_info info;
    libtorrent::torrent_handle handle;

    Torrent();
  };

  Torrents(char *_target_dir, char *_watch_dir);

  void Start();


private:
  char *target_dir;
  char *watch_dir;

  libtorrent::session session; // TODO: set the client id

  Torrent torrent;
};

#endif
