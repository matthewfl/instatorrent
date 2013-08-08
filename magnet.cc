#include "magnet.h"


#include <libtorrent/create_torrent.hpp>
#include <libtorrent/alert.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/magnet_uri.hpp>


#include <thread>

using namespace libtorrent;
using namespace std;



struct Magnet_alert_handler {
  Magnet *magnet;

  void operator()(metadata_received_alert const &a) const {
    const torrent_handle &h = a.handle;
    string hash = to_hex(h.info_hash().to_string());
    if (h.is_valid()) {
      torrent_info const& ti = h.get_torrent_info();
      create_torrent ct(ti);
      entry te = ct.generate();
      std::vector<char> buffer;
      bencode(std::back_inserter(buffer), te);
      string fname = magnet->path;
      fname += (to_hex(ti.info_hash().to_string()) + ".torrent");
      FILE* f = fopen(fname.c_str(), "w+");
      if (f) {
	fwrite(&buffer[0], 1, buffer.size(), f);
	fclose(f);
      }
    }
    magnet->session.remove_torrent(h, session::delete_files);
    cerr << "done with: " << hash << endl;
  }

};

Magnet::Magnet (char *dir) : Torrents(dir, "") {
  // read from stdin
  path = dir;
  path += '/';
}

void Magnet::Start() {
  running = true;
  Configure();

  std::thread alert_handler([this](){
      Watch();
    });

  while(running) {
    string link;
    add_torrent_params params;
    error_code ec;
    getline(cin, link);
    parse_magnet_uri(link, params, ec);
    if(ec.value() != 0) {
      cerr << "problem adding torrent: " << link << endl;
      continue;
    }
    session.add_torrent(params);

  }

  alert_handler.join();

}

void Magnet::Watch () {
  Magnet_alert_handler alert_handler;
  alert_handler.magnet = this;

  while(running) {
    std::auto_ptr<alert> alert = session.pop_alert();
    while(alert.get()) {
      try {
	handle_alert<
	  metadata_received_alert
	  > hands(alert, alert_handler);
      } catch(unhandled_alert e) {
	// this is me caring
	 cerr << "unhandled alert: " << alert->message() << endl;
      }
      alert = session.pop_alert();
    }

    session_status stats = session.status();
    cerr << "stats: " << stats.upload_rate << "\t" << stats.download_rate << "\t" << stats.total_download << "\t" << stats.total_upload << "\t"
	 << stats.num_peers << endl;

    sleep(250);

  }
}
