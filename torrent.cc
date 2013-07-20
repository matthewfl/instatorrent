#include "torrent.h"

#include <assert.h>
#include <iostream>
using namespace std;


#include <libtorrent/extensions/ut_pex.hpp>

using namespace libtorrent;

Torrents::Torrents(char *_target_dir, char *_watch_dir) : target_dir(_target_dir), watch_dir(_watch_dir) {

}


void Torrents::Start() {
  running = true;
  cerr << "torrent downloader starting\n";

  pe_settings con_settings;
  con_settings.out_enc_policy = pe_settings::forced;
  con_settings.in_enc_policy = pe_settings::forced;
  con_settings.allowed_enc_level = pe_settings::rc4;
  session.set_pe_settings(con_settings);

  // peer exchange
  session.add_extension(&libtorrent::create_ut_pex_plugin);

  session.start_dht();
  //  session.start_lsd(); // local peers
  session.start_natpmp();
  session.start_upnp();


  while(running) {
    torrent_status stat = torrent.handle.status();

    cerr << "progress:" << stat.progress << " trac:" << stat.current_tracker << " connected:" << stat.num_peers
	 << " cons:" << stat.num_connections << " seeds:" << stat.num_seeds <<  endl;
    sleep(1000);
  }

}

void Torrents::Stop() {
  running = false;
}

Torrents::Torrent::Torrent(Torrents* par) : parent(par) {
  //torrent_info info("testing.torrent");
  boost::intrusive_ptr<torrent_info> info(new torrent_info("testing.torrent"));

  add_torrent_params params;
  params.ti = info;
  params.save_path = parent->target_dir + "/" + to_hex(info->info_hash().to_string());
  params.storage_mode = storage_mode_allocate;
  handle = parent->session.add_torrent(params);

  cerr << "adding torrent " << info->name() << " size:" << info->total_size() << endl;

}


Torrents::Torrent* Torrents::lookupTorrent(std::string hash) {
  return &torrent; // currently only one torrent
}
