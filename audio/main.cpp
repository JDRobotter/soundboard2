#include <iostream>

#include "maddecoder.hpp"

int main(int argc, char **argv) {
  
  std::cout<<"Testing mad decoder wrapper\n";

  MADDecoder mad;

  mad.open(argv[1]);
  mad.start();



  mad.join();
  std::cerr<<"We're out\n";
  return 0;
}
