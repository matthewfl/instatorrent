#include <iostream>

#include "fuse.h"


using namespace std;


Fuse f;


int main(int argc, char **argv) {

  if(argc != 4) {
    cerr << "Usage: " << argv[0] << " [fuse dir] [target dir] [watch dir]";
      return 1;
  }

  const char *fuse_argv[2];
  fuse_argv[0] = "-f";
  fuse_argv[1] = argv[1];
  f = Fuse(2, fuse_argv);

  f.Start();


  return 0;
}
