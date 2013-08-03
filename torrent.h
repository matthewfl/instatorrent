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


    Torrent(Torrents*, libtorrent::torrent_handle);
    TorrentFile *lookupFile(std::string);

  private:
    std::multimap<int, std::function<void(int)>> m_alertCallbacks;
    void alert(int);
    Torrents* parent;
    libtorrent::torrent_handle handle;
    bool running;
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

  static TorrentFile EmptyTorrentFile;

protected:
  void Configure();

  // TODO: set the client id
  libtorrent::session session;//(libtorrent::fingerprint, //("LT", 0, 1, 0, 0),
				// std::pair<int, int> (15000, 18000));


private:
  std::string target_dir;
  std::string watch_dir;
  bool running = false;

  void Watch();

  std::map<libtorrent::sha1_hash, Torrent*> m_torrents;

  // just for testing atm
  //Torrent torrent;// = Torrent(this);

  friend struct Torrents_alert_handler;
};

#endif
