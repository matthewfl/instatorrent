#include "torrent.h"

#include <assert.h>
#include <iostream>
using namespace std;


#include <libtorrent/extensions/ut_pex.hpp>
#include <libtorrent/alert.hpp>
#include <libtorrent/alert_types.hpp>

using namespace libtorrent;


struct Torrents_alert_handler {

  void operator()(portmap_error_alert const &a) const {
    std::cerr << "Portmapper error: " << a.message() << std::endl;
  }

  void operator()(portmap_alert const &a) const {
    std::cerr << "Portmapper: " << a.message() << endl;
  }

  void operator()(tracker_warning_alert const &a) const {
    std::cerr << "Tracker warning: " << a.message() << std::endl;
  }

  void operator()(torrent_finished_alert const &a) const {
    std::cerr << a.handle.get_torrent_info().name() << " completed"
	      << std::endl;
  }

  void operator()(piece_finished_alert const &a) {
    cerr << "piece finished alert " << a.message() << endl;
    const torrent_handle &handle = a.handle;

  }
};

Torrents::Torrents(char *_target_dir, char *_watch_dir) : target_dir(_target_dir), watch_dir(_watch_dir) {

}


void Torrents::Start() {
  running = true;
  cerr << "torrent downloader starting\n";

  session_settings settings = high_performance_seed();
  settings.announce_to_all_trackers = true;
  settings.announce_to_all_tiers = true;
  settings.inactivity_timeout = 90;
  settings.choking_algorithm = session_settings::bittyrant_choker;
  settings.max_metadata_size = 4 * 1024 * 1024;

  session.set_settings(settings);

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

  Torrents_alert_handler alert_handler;

  while(running) {
    // TODO: make these global stats
    torrent_status stat = torrent.handle.status();

    cerr << "progress:" << stat.progress << " trac:" << stat.current_tracker << " connected:" << stat.num_peers
	 << " cons:" << stat.num_connections << " seeds:" << stat.num_seeds <<  endl;


    std::auto_ptr<alert> alert = session.pop_alert();
    while(alert.get()) {
      handle_alert<portmap_error_alert,
		   tracker_warning_alert,
		   torrent_finished_alert
		   > hands(alert, alert_handler);
      alert = session.pop_alert();
    }



    sleep(1000);
  }

}

void Torrents::Stop() {
  running = false;
}

Torrents::Torrent::Torrent(Torrents* par) : parent(par) {
  //torrent_info info("testing.torrent");
  boost::intrusive_ptr<torrent_info> info(new torrent_info("testing.torrent"));

  add_torrent_params params;                // write fast resume data
                // ...
  params.ti = info;
  params.save_path = parent->target_dir + "/" + to_hex(info->info_hash().to_string());
  params.storage_mode = storage_mode_allocate;
  handle = parent->session.add_torrent(params);

  cerr << "adding torrent " << info->name() << " size:" << info->total_size() << endl;

}


Torrents::Torrent* Torrents::lookupTorrent(std::string hash) {
  return &torrent; // currently only one torrent
}


Torrents::TorrentFile *Torrents::Torrent::lookupFile(std::string name) {
  cerr << "=================" << name << endl;
  const libtorrent::torrent_info &info = handle.get_torrent_info();
  for(auto it = info.begin_files(), end = info.end_files(); it != end; it++) {
    string itname = "/";
    itname += it->filename();
    cerr << "it file --- " << itname << " " << it->offset << " " << it->size << endl;
    if(itname == name) {
      return new TorrentFile(this, info.piece_length(), it->offset, it->size);
    }
  }
  return NULL;
}

void Torrents::TorrentFile::get(size_t offset, size_t length, std::function<void(size_t, size_t)> callback) {
  assert(m_parent);
  assert(offset + length <= m_size);



}

bool Torrents::TorrentFile::has(size_t offset, size_t length) {
  assert(m_parent);
  int start_block = (m_offset + offset) / m_block_size;
  assert(length <= m_block_size); // TODO: remove

  return m_parent->handle.have_piece(start_block);
  // parent->handle.have_piece()
}


Torrents::TorrentFile::TorrentFile(Torrent *par, int block_size, int offset, int size) : m_parent(par), m_size(size), m_offset(offset), m_block_size(block_size) {

}

Torrents::TorrentFile::TorrentFile() : m_parent(NULL) {

}
