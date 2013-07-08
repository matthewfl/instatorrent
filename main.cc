#include <iostream>

#include "fuse.h"


using namespace std;

int start_fuse() {

}

int main(int argc, char **argv) {

  Fuse f(argc, argv);
  f.Start();
  cout << "hello world";
  return 0;
}
