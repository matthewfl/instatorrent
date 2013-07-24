#ifndef _tor_magnet
#define _tor_magnet

#include "torrent.h"


/*
 * 1. read from stdin 1 line at a time of a magnet link
 * 2. drop the torrent in the target dir in the form of ***info hash***.torrent
 * 3. profit?
 */

struct Magnet_alert_handler;

class Magnet : protected Torrents {
public:
  Magnet(char *dir);
  void Start();
  void Stop() { running = false; }
private:
  void Watch();

  bool running = false;

  std::string path;
  friend struct Magnet_alert_handler;
};

#endif // _tor_magnet
