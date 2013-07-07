#ifndef _tor_fuse
#define _tor_fuse

#define FUSE_USE_VERSION 26

#include <fuse_lowlevel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include <map>
#include <string>



class Fuse {
private:
  std::map<std::string, fuse_ino_t> name_map;
  std::map<fuse_ino_t, std::string> ino_map;

  int argc;
  char **argv;
public:
  Fuse(int _argc, char **_argv);
  int Start();
};



#endif // _tor_fuse
