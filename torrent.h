#ifndef _tor_torrent
#define _tor_torrent

#include <libtorrent/entry.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/bencode.hpp>

#include <string>

class Torrents {
public:

  class Torrent {
  public:
    // libtorrent::torrent_info info;
    libtorrent::torrent_handle handle;

    Torrent(Torrents*);
  private:
    Torrents* parent;
  };

  Torrents(char *_target_dir, char *_watch_dir);

  void Start();
  void Stop();

private:
  std::string target_dir;
  std::string watch_dir;
  bool running = false;

  libtorrent::session session; // TODO: set the client id

  // just for testing atm
  Torrent torrent = Torrent(this);
};

#endif
