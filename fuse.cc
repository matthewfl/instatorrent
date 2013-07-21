#include "fuse.h"

#include <dirent.h>
#include <sys/mman.h>

#include <assert.h>
#include <iostream>
using namespace std;

static Fuse *Fuse_manager;


static void manager_lookup(fuse_req_t req, fuse_ino_t parent_ino, const char *name) {
  cerr << "fuse lookup "<< parent_ino << name << endl;
  struct fuse_entry_param e;

  Fuse::fileInfo *parent = Fuse_manager->lookupInode(parent_ino);

  if(!parent) {
    fuse_reply_err(req, ENOENT);
    return;
  }

  auto inter = parent->files.find(name);
  if(inter == parent->files.end()) {
    fuse_reply_err(req, ENOENT);
    return;
  }

  Fuse::fileInfo *file = inter->second;
  if(!file) {
    fuse_reply_err(req, ENOENT);
    return;
  }

  cerr << "--lookup " << file->name << " " << file->type << endl;

  memset(&e, 0, sizeof(e));
  e.ino = file->inode;
  e.attr_timeout = 1.0;
  e.entry_timeout = 1.0;
  e.attr.st_mode = 0444 | (file->type == Fuse::fileInfo::FILE ? S_IFREG : S_IFDIR);
  e.attr.st_nlink = 2;
  e.attr.st_size = file->file_size;

  fuse_reply_entry(req, &e);

}

static void manager_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
  cerr << "fuse getattr " << ino << endl;

  struct stat stbuf;

  (void) fi;

  memset(&stbuf, 0, sizeof(stbuf));
  stbuf.st_mode = S_IFDIR | 0444;
  stbuf.st_nlink = 2;

  fuse_reply_attr(req, &stbuf, 1.0);

  /*
  if (hello_stat(ino, &stbuf) == -1)
    fuse_reply_err(req, ENOENT);
  elseS_ISREG
    fuse_reply_attr(req, &stbuf, 1.0);
  */
}

static void manager_readdir(fuse_req_t req, fuse_ino_t ino, size_t size_limit, off_t off, struct fuse_file_info *fi) {
  cerr << "fuse readdir " << size_limit << " " << off << " " << ino << endl;

  cerr << "before lookup";
  Fuse::fileInfo *dir = Fuse_manager->lookupInode(ino);

  assert(dir);

  // TODO: move this into opendir
  if(off == 0) {
    // refresh the files list
    size_t bytes = 0;
    dirent dirEntry;
    dirent *dirResult;

    cerr << "before lock";
    dir->lock.lock();
    cerr << "after lock";
    for(auto it : dir->files) {
      it.second->access = false;
    }
    DIR* handle = dir->openDir();
    while(true) {
      cerr << '.';
      readdir_r(handle, &dirEntry, &dirResult);
      if(!dirResult) break;
      if(dirEntry.d_name[0] == '.') continue; // no hidden files
      cerr << "\t\t adding: " << dirEntry.d_name << endl;
      bytes += fuse_add_direntry(req, NULL, 0, dirEntry.d_name, NULL, 0);
      Fuse::fileInfo *child = dir->files[dirEntry.d_name];
      if(!child) {
	child = dir->files[dirEntry.d_name] = Fuse_manager->newInode(dir, dirEntry.d_type == DT_DIR ? Fuse::fileInfo::DIRECTORY : Fuse::fileInfo::FILE);
	child->name = dirEntry.d_name;
      }else{
	child->access = true;
      }
      cerr << "\n looking at " << dirEntry.d_name << " type:" << (int)dirEntry.d_type;
      //      child->type = ;

      if(child->type == Fuse::fileInfo::FILE) {
	struct stat stats;
	stat(child->getPath().c_str(), &stats);
	child->file_size = stats.st_size;
      }
    }
    closedir(handle);
    // deleting stuff with iterators can lead to issues
    for(auto it : dir->files) {
      if(it.second->access == false) {
	// delete
	// TODO: delete things that it can no longer find
      }
    }
    bytes += fuse_add_direntry(req, NULL, 0, ".", NULL, 0);
    bytes += fuse_add_direntry(req, NULL, 0, "..", NULL, 0);

    delete dir->dirEntryData;
    dir->dirEntryData = new char[bytes];
    dir->dirEntryData_size = bytes;

    struct stat stbuf;
    memset(&stbuf, 0, sizeof(stbuf));
    stbuf.st_ino = dir->inode; //dirEntry.d_ino + 15;
    stbuf.st_mode = 0444 | S_IFDIR; // dirEntry.d_type == DT_DIR ? S_IFDIR : S_IFREG;

    size_t at = 0;
    at = fuse_add_direntry(req, dir->dirEntryData, bytes, ".", &stbuf, bytes);
    stbuf.st_ino = dir->parent ? dir->parent->inode : 1;
    at = fuse_add_direntry(req, dir->dirEntryData + at, bytes, "..", &stbuf, bytes);
    for(auto it : dir->files) {
      if(it.second->access) { // TODO: take this out
	stbuf.st_ino = it.second->inode;
	stbuf.st_mode = 0444 | it.second->type == Fuse::fileInfo::FILE ? S_IFREG : S_IFDIR;
	at += fuse_add_direntry(req, dir->dirEntryData + at, bytes, it.second->name.c_str(), &stbuf, bytes);
      }
    }

    dir->lock.unlock();
  }


  size_t ret_size = dir->dirEntryData_size - off;
  if(ret_size > size_limit) ret_size = size_limit;
  if(ret_size > 0)
    fuse_reply_buf(req, dir->dirEntryData + off, ret_size);
  else
    fuse_reply_buf(req, NULL, 0);

}

static void manager_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
  cerr << "fuse open\n";
  Fuse::fileInfo *file = Fuse_manager->lookupInode(ino);
  if(!file) {
    fuse_reply_err(req, EISDIR);
    return;
  }
  if((fi->flags & 3) != O_RDONLY) {
    fuse_reply_err(req, EACCES);
    return;
  }
  if(file->type != Fuse::fileInfo::FILE) {
    fuse_reply_err(req, EISDIR);
    return;
  }
  fi->fh = (uint64_t)file;
  if(!file->fd) {
    cerr << "---- open file: " << file->getPath() << endl;
    file->fd = open(file->getPath().c_str(), O_RDONLY);
    file->mmap = mmap(NULL, file->file_size, PROT_READ, MAP_SHARED, file->fd, 0);
  }

  fuse_reply_open(req, fi);
}

static void manager_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi) {
  cerr << "fuse read " << size << " " << off << endl;
  Fuse::fileInfo *file = (Fuse::fileInfo*) fi->fh;
  file->lock.lock();

  const char *hello = "hello world";



  if(off == 0)
    fuse_reply_buf(req, hello, 12);
  else
    fuse_reply_buf(req, NULL, 0);

  file->lock.unlock();
}


// TODO: do this in a smarter way
static struct fuse_lowlevel_ops manager_ll_oper = {
  NULL, // init
  NULL, // destroy
  manager_lookup, // lookup
  NULL, // forget
  manager_getattr, // getattr
  NULL, // setattr
  NULL, // readlink
  NULL, // mknod
  NULL, // mkdir
  NULL, // unlink
  NULL, // rmdir
  NULL, // symlink
  NULL, // rename
  NULL, // linkbmap
  manager_open, // open
  manager_read, // read
  NULL, // write
  NULL, // flush
  NULL, // release
  NULL, // fsync
  NULL, // opendir
  manager_readdir, // readdir
  NULL, // releasedir
  NULL, // fsyncdir
  NULL, // statfs
  NULL, // setxattr
  NULL, // getxattr
  NULL, // listxattr
  NULL, // removexattr
  NULL, // access
  NULL, // create
  NULL, // getlk
  NULL, // setlk
  NULL, // bmap
  NULL, // ioctl
  NULL, // poll
  NULL, // write_buf
  NULL, // retreive_reply
  NULL, // forget_multi
  NULL, // flock
  NULL	// fallocate
};

Fuse::Fuse(char *_fuse_dir, char *_target_dir, Torrents *torr): fuse_dir(_fuse_dir), target_dir(_target_dir), torrent_manager(torr) {
  assert(!Fuse_manager);
  Fuse_manager = this;
  fileInfo* root = inoMap[1] = new fileInfo;
  root->inode = 1;
  root->name = target_dir;
  assert(sizeof(uint64_t) >= sizeof(void*));
}

int Fuse::Start() {
  assert(Fuse_manager);

  const char *fuse_argv[2];
  fuse_argv[0] = "-f";
  fuse_argv[1] = fuse_dir;

  struct fuse_args args = FUSE_ARGS_INIT(2, const_cast<char**>(fuse_argv));
  struct fuse_chan *ch;
  char *mountpoint;
  int err = -1;

  if (fuse_parse_cmdline(&args, &mountpoint, NULL, NULL) != -1 &&
      (ch = fuse_mount(mountpoint, &args)) != NULL) {
    struct fuse_session *se;

    se = fuse_lowlevel_new(&args, &manager_ll_oper,
			   sizeof(manager_ll_oper), NULL);
    if (se != NULL) {
      if (fuse_set_signal_handlers(se) != -1) {
	fuse_session_add_chan(se, ch);
	err = fuse_session_loop(se);
	fuse_remove_signal_handlers(se);
	fuse_session_remove_chan(ch);
      }
      fuse_session_destroy(se);
    }
    fuse_unmount(mountpoint, ch);
  }
  fuse_opt_free_args(&args);

  return err;
}

DIR* Fuse::openTargetDir() {
  return opendir(target_dir);
}


std::vector<Fuse::fileInfo> Fuse::listFiles(int inode) {
  if(inode == 1) {
    // this is the base inode
  }
}


time_t Fuse::time() {
  return ::time(NULL);
}


Fuse::fileInfo* Fuse::newInode(fileInfo *parent, fileInfo::type_t type) {
  fileInfo *info = new fileInfo;
  info->inode = ++inode_count;
  info->parent = parent;
  info->type = type;
  inoMap[info->inode] = info;
  if(type == fileInfo::FILE) {
    Torrents::Torrent *torrent = torrent_manager->lookupTorrent(info->getHash());
    assert(torrent); // TODO: something that is not assert
    info->torrent = torrent->lookupFile(info->getTorrentPath());
  }
  return info;
  //return ++inode_count;
}

Fuse::fileInfo* Fuse::lookupInode(fuse_ino_t ino) {
  //assert(ino < inode_count);
  if(ino > inode_count) return NULL;
  return inoMap[ino];
}


DIR* Fuse::fileInfo::openDir() {
  std::string path = getPath();
  //  cout << "open dir " << path;
  return opendir(path.c_str());
}

std::string Fuse::fileInfo::getPath() {
  if(inode == 1) {
    return name;
  }
  assert(parent);
  return parent->getPath() + "/" + name;
}

std::string Fuse::fileInfo::getHash() {
  assert(parent);
  if(parent->inode == 1) {
    return name; // the base dir name will be the hash
  }
  return parent->getHash();
}

std::string Fuse::fileInfo::getTorrentPath() {
  assert(parent);
  if(parent->inode == 1) {
    return "";
  }
  return parent->getTorrentPath() + "/" + name;
}
