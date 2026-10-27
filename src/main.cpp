#include <filesystem>
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>

#include "../external/inicpp.h"

namespace fs = std::filesystem;
using namespace std;

void help() {
  cout << "This is a ricing tool assistant" << endl;
  cout << "rist create riceName - creates a new rice environment and selects it" << endl;
  cout << "rist load riceName - loads target rice" << endl;
  cout << "rist save riceName - saves target rice based on what's loaded" << endl;
  cout << "rist destroy riceName - removes target rice" << endl;
  cout << "rist add directory - adds a directory to selected rice" << endl;
  cout << "rist rm directory - removes a directory from selected rice" << endl;
}

int main(int argc, char* argv[]) {
  const char* homeDir = getenv("HOME");
  fs::path configPath = fs::path(homeDir) / ".config" / "rist";
  fs::path riceDataPath = fs::path(configPath) / "rist.ini";

  if (!fs::exists(configPath)) {
    cout << "Creating configuration path at " << configPath << endl;
    fs::create_directory(configPath);
  }

  cout << "Displaying arguments: " << endl;
  for (int i = 0; i < argc; i++) {
    cout << argv[i] << endl;
  }
  if (argc < 3) {
    help();
    return 0;
  }

  if (!fs::exists(riceDataPath)) {
    cout << "Creating rist.ini at " << riceDataPath << endl;
    ofstream file(riceDataPath);
    file.close();
  }

  ini::IniFile riceData;
  riceData.load(riceDataPath);

  std::cout << riceData.size() << endl;

  if (!strcmp(argv[1], "create")) {
    ofstream file(riceDataPath);
    if (file.is_open()) {
      file << "test";
      file.close();
    }
    else {
      cerr << "Failed to create file." << endl;
    }
  }
  return 0;
}
