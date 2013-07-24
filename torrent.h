#ifndef _tor_torrent
#define _tor_torrent


#include <libtorrent/entry.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/bencode.hpp>

#include <string>

struct Torrents_alert_handler;

class Torrents {
public:

  class TorrentFile;
  class Torrent {
  public:
    // libtorrent::torrent_info info;


    Torrent(Torrents*);
    TorrentFile *lookupFile(std::string);

  private:
    std::multimap<int, std::function<void(int)>> m_alertCallbacks;
    void alert(int);
    Torrents* parent;
    libtorrent::torrent_handle handle;
    friend class TorrentFile;
    friend class Torrents;
    friend struct Torrents_alert_handler;
  };

  class TorrentFile {
  public:
    bool has(size_t offset, size_t length);
    void get(size_t offset, size_t length, std::function<void(size_t, size_t)> callback);
    TorrentFile();
  private:
    Torrent* m_parent;
    unsigned int m_size;
    unsigned int m_offset;
    unsigned int m_block_size;
    TorrentFile(Torrent*, int, int, int);
    friend class Torrent;
  };

  Torrents(char *_target_dir, char *_watch_dir);

  void Start();
  void Stop();

  Torrent* lookupTorrent(std::string hash);

protected:
  void Configure();

  libtorrent::session session; // TODO: set the client id

private:
  std::string target_dir;
  std::string watch_dir;
  bool running = false;

  // just for testing atm
  Torrent torrent = Torrent(this);

  friend struct Torrents_alert_handler;
};

#endif
