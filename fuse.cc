#include "fuse.h"

#include <assert.h>
#include <iostream>
using namespace std;

static Fuse *Fuse_manager;


static void manager_lookup(fuse_req_t req, fuse_ino_t parent, const char *name) {
  cerr << "fuse lookup "<< name << endl;
  struct fuse_entry_param e;

  if (parent != 1) { // || strcmp(name, hello_name) != 0)
    fuse_reply_err(req, ENOENT);
  } else {
    memset(&e, 0, sizeof(e));
    e.ino = 2;
    e.attr_timeout = 1.0;
    e.entry_timeout = 1.0;
    e.attr.st_mode = S_IFREG | 0444;
    e.attr.st_nlink = 1;
    e.attr.st_size = 0; //strlen);
    //hello_stat(e.ino, &e.attr);

    fuse_reply_entry(req, &e);
  }
}

static void manager_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
  cerr << "fuse getattr \n";

  struct stat stbuf;

  (void) fi;

  memset(&stbuf, 0, sizeof(stbuf));
  stbuf.st_mode = S_IFDIR | 0555;
  stbuf.st_nlink = 2;

  fuse_reply_attr(req, &stbuf, 1.0);

  /*
  if (hello_stat(ino, &stbuf) == -1)
    fuse_reply_err(req, ENOENT);
  else
    fuse_reply_attr(req, &stbuf, 1.0);
  */
}

static void manager_readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi) {
  cerr << "fuse readdir \n";
}

static void manager_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
  cerr << "fuse open\n";
}

static void manager_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi) {
  cerr << "fuse read\n";
}



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
  NULL, // link
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

Fuse::Fuse(int _argc, char **_argv) : argc(_argc), argv(_argv) {
  assert(!Fuse_manager);
  Fuse_manager = this;

}

int Fuse::Start() {

  struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
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
