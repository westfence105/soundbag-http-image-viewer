#include <iostream>
#include <string>
#include <stdexcept>

#include "ImageServer.hpp"

void usage(const std::string& appname);

int main(int argc, char** argv) {
  int port = 8080;

  if (argc > 1) {
    try {
      port = std::stoi(argv[1]);
    }
    catch (const std::invalid_argument& e) {
      usage(argv[0]);
      return -1;
    }
  }

  ImageServer server("", port);
  server.start();

  char buf;
  std::cout << "Server running on port " << port << "...\nPress 'x' to stop." << std::endl;
  do {
    std::cin >> buf;
  } while (buf != 'x');
  server.stop();

  return 0;
}

void usage(const std::string& appname) {
  std::cout << "Usage: " << appname << " [port]" << std::endl;
}
