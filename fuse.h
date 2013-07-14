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

#include <sys/types.h>
#include <dirent.h>
#include <time.h>


#include <map>
#include <vector>
#include <string>

#include <mutex>



class Fuse {
public:
  class fileInfo {
  public:
    std::string name;
    fuse_ino_t inode;
    std::map<std::string, fileInfo*> files;
    char *dirEntryData = NULL;
    size_t dirEntryData_size;
    std::mutex lock;
    bool access = true;
    fileInfo *parent = NULL;
    enum type_t {
      FILE,
      DIRECTORY
    } type;
    DIR* openDir();
    std::string getPath();
  };
public:

  Fuse(char *_fuse_dir, char *_target_dir);
  Fuse() {}
  int Start();

  // use by static methods for working with fuse
  std::vector<fileInfo> listFiles(int inode);
  DIR* openTargetDir();
  time_t time();

  fileInfo* newInode(fileInfo *parent);
  fileInfo* lookupInode(fuse_ino_t ino);



private:
  //std::map<std::string, fuse_ino_t> name_map;
  // TODO: use a thread safe map
  std::map<fuse_ino_t, fileInfo*> inoMap;
  //  std::map<inode_t, fileInfo*> realFsToIno;

  char *fuse_dir;
  char *target_dir;

  fuse_ino_t inode_count = 10;
};



#endif // _tor_fuse
