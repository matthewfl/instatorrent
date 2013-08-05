#include "torrent.h"

#include <assert.h>
#include <iostream>
#include <thread>
using namespace std;

#include <dirent.h>
#include <stdlib.h>

#include <libtorrent/extensions/ut_pex.hpp>
#include <libtorrent/extensions/metadata_transfer.hpp>
#include <libtorrent/extensions/ut_metadata.hpp>
#include <libtorrent/alert.hpp>
#include <libtorrent/alert_types.hpp>

using namespace libtorrent;


struct Torrents_alert_handler {

  Torrents *torrents;

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

  void operator()(piece_finished_alert const &a) const {
    cerr << "piece finished alert " << a.message() << endl;
    const torrent_handle &handle = a.handle;
    torrents->m_torrents.find(handle.info_hash())->second->alert(a.piece_index);

    // TODO: lookup which torrent this is
    //torrents->torrent.alert(a.piece_index);
  }
};

Torrents::Torrents(char *_target_dir, char *_watch_dir) : target_dir(_target_dir), watch_dir(_watch_dir) {

}

void Torrents::Configure() {

  char *env = getenv("env");
  if(env && string(env) == "production") {
    cerr << "Running with production settings\n";
    session_settings settings = high_performance_seed();
    settings.announce_to_all_trackers = true;
    settings.announce_to_all_tiers = true;
    settings.inactivity_timeout = 90;
    settings.choking_algorithm = session_settings::bittyrant_choker;
    settings.max_metadata_size = 4 * 1024 * 1024; // TODO: might up to 8mb
    settings.share_ratio_limit = 1.3;
    settings.strict_end_game_mode = false; // allow requesting a piece more than once earlier on
    settings.seeding_outgoing_connections = false; // do not make out going connections when seeding

    session.set_settings(settings);
  }


  session.set_alert_mask(alert::progress_notification | alert::port_mapping_notification |
			 alert::tracker_notification | alert::status_notification);

  pe_settings con_settings;
  con_settings.out_enc_policy = pe_settings::forced;
  con_settings.in_enc_policy = pe_settings::forced;
  con_settings.allowed_enc_level = pe_settings::rc4;
  session.set_pe_settings(con_settings);

  // peer exchange
  session.add_extension(&libtorrent::create_ut_pex_plugin);
  // metadata
  session.add_extension(&libtorrent::create_metadata_plugin);
  session.add_extension(&libtorrent::create_ut_metadata_plugin);

  session.start_dht();
  //  session.start_lsd(); // local peers
  session.start_natpmp();
  session.start_upnp();

}

void Torrents::Start() {
  running = true;
  cerr << "torrent downloader starting\n";

  Configure();

  Torrents_alert_handler alert_handler;
  alert_handler.torrents = this;

  std::thread watch_thread([this](){
      Watch();
    });

  while(running) {
    // TODO: make these global stats
    // torrent_status stat = torrent.handle.status();

    // cerr << "progress:" << stat.progress << " trac:" << stat.current_tracker << " connected:" << stat.num_peers
    //	 << " cons:" << stat.num_connections << " seeds:" << stat.num_seeds <<	endl;

    std::auto_ptr<alert> alert = session.pop_alert();
    while(alert.get()) {
      try {
	handle_alert<
	  portmap_error_alert,
	  tracker_warning_alert,
	  torrent_finished_alert,
	  piece_finished_alert
	  > hands(alert, alert_handler);
      } catch (unhandled_alert e) {
	// ignore stuff we don't care about
	//cerr << "unhandled alert: " << e.what() << " " << alert->message() << endl;
      }
      alert = session.pop_alert();
    }

    sleep(100); // make this smaller to read the pieces off faster
  }

  watch_thread.join();
}

void Torrents::Stop() {
  running = false;
}

void Torrents::Watch () {
  dirent dirEntry;
  dirent *dirResult;
  while(running) {
    // session status stuff
    session_status stats = session.status();
    cerr << "stats: " << stats.upload_rate << "\t" << stats.download_rate << "\t" << stats.total_download << "\t" << stats.total_upload << "\t"
	 << stats.num_peers << endl;

    DIR* dir = opendir(watch_dir.c_str());
    while(true) {
      readdir_r(dir, &dirEntry, &dirResult);
      if(!dirResult) break;
      if(dirEntry.d_name[0] == '.') continue; // no hidden files
      boost::intrusive_ptr<torrent_info> info;
      try {
	info = new torrent_info(watch_dir + "/" + dirEntry.d_name);
      } catch(libtorrent_exception e) {
	cerr << "watch dir " << e.what() << endl;
	continue;
      }
      if(m_torrents.find(info->info_hash()) == m_torrents.end()) {
	add_torrent_params params;
	params.ti = info;
	params.save_path = target_dir + "/" + to_hex(info->info_hash().to_string());
	params.storage_mode = storage_mode_allocate;

	m_torrents[info->info_hash()] = new Torrent(this, session.add_torrent(params));

	cerr << "adding torrent " << info->name() << " size:" << info->total_size() << " hash:" << to_hex(info->info_hash().to_string()) << endl;
      }
      cerr << "watch dir: " << dirEntry.d_name << endl;
    }
    closedir(dir);
    sleep(2000);
  }
}

Torrents::Torrent::Torrent(Torrents* par, torrent_handle hand) : parent(par), handle(hand) {
  //torrent_info info("testing.torrent");
  return;
  boost::intrusive_ptr<torrent_info> info(new torrent_info("testing.torrent"));

  add_torrent_params params;		    // write fast resume data
  // ...
  params.ti = info;
  params.save_path = parent->target_dir + "/" + to_hex(info->info_hash().to_string());
  params.storage_mode = storage_mode_allocate;
  handle = parent->session.add_torrent(params);

  cerr << "adding torrent " << info->name() << " size:" << info->total_size() << endl;

}


Torrents::Torrent* Torrents::lookupTorrent(std::string hash) {
  assert(hash.size() == 40);
  sha1_hash sha;
  from_hex(hash.c_str(), 40, (char*)&sha[0]);
  auto tor = m_torrents.find(sha);
  if(tor == m_torrents.end()) return NULL;
  return tor->second;

  //  return &torrent; // currently only one torrent
}


Torrents::TorrentFile *Torrents::Torrent::lookupFile(std::string name) {
  cerr << "=================" << name << endl;
  const libtorrent::torrent_info &info = handle.get_torrent_info();
  for(auto it = info.begin_files(), end = info.end_files(); it != end; it++) {
    string itname = "/";
    itname += it->filename();
    cerr << "it file --- " << itname << " " << it->offset << " " << it->size << endl;
    // should just make this not return a pointer
    if(itname == name) {
      return new TorrentFile(this, info.piece_length(), it->offset, it->size);
    }
  }
  return NULL;
}

void Torrents::Torrent::alert(int index) {
  auto msg = m_alertCallbacks.equal_range(index);
  for(auto it = msg.first; it != msg.second; it++) {
    it->second(index);
  }
  m_alertCallbacks.erase(msg.first, msg.second);
}

void Torrents::TorrentFile::get(size_t offset, size_t length, std::function<void(size_t, size_t)> callback) {
  assert(m_parent);
  assert(offset + length <= m_size);
  //assert(length <= m_block_size); // TODO: remove
  int start_block = (m_offset + offset) / m_block_size;
  int end_block = (m_offset + offset + length) / m_block_size;
  //assert(start_block == end_block);


  //  function<void(int)> f = [callback] (int block) -> void {};

  // if we capture the variable then it will reference to the location on the stack
  int *count = new int;
  *count = end_block - start_block;

  function<void(int)> func = [callback, offset, length, count](int block) -> void{
    cerr << "!!!!!!!!!!!!!!!!! callback: " << block << endl;
    if(*count == 0) {
      callback(offset, length);
      delete count;
      return;
    }
    (*count)--;
  };

  for(int block = start_block; block <= end_block; block++) {
    m_parent->m_alertCallbacks.insert(pair<int, function<void(int)>>(block, func));
  }

  for(int block = start_block; block <= end_block; block++) {
    m_parent->handle.piece_priority(block, 7);
  }

  cerr << "@@@@@@@@@@@ requesting: " << start_block << ".." << end_block << endl;
}

bool Torrents::TorrentFile::has(size_t offset, size_t length) {
  if(!m_parent) return true;
  //assert(m_parent);
  int start_block = (m_offset + offset) / m_block_size;
  int end_block = (m_offset + offset + length) / m_block_size;
  //assert(length <= m_block_size); // TODO: remove
  for(int block = start_block; block <= end_block; block++) {
    if(!m_parent->handle.have_piece(start_block))
      return false;
  }
  return true;
  //return m_parent->handle.have_piece(start_block);
  // parent->handle.have_piece()
}


Torrents::TorrentFile::TorrentFile(Torrent *par, int block_size, int offset, int size) : m_parent(par), m_size(size), m_offset(offset), m_block_size(block_size) {

}

Torrents::TorrentFile::TorrentFile() : m_parent(NULL) {

}

Torrents::TorrentFile Torrents::EmptyTorrentFile = TorrentFile();
