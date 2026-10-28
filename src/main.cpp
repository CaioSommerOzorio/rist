#include <filesystem>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <string>

#include "../external/inicpp.h"

namespace fs = std::filesystem;
using namespace std;

string strip(string s, char c) {
  string token = "";
  for (auto sub: s) {
    if (sub != c) {
      token += sub;
    }
  }
  return token;
}

vector<string> parseList(string s, const string& delimiter) {
  vector<string>tokens;
  size_t pos = 0;
  string token;
  while ((pos = s.find(delimiter)) != string::npos) {
    token = s.substr(0, pos);
    tokens.push_back(token);
    s.erase(0, pos + delimiter.length());
  }
  tokens.push_back(s);

  return tokens;
}

void initFile(fs::path path) {
  ini::IniFile riceData;
  riceData["meta"]["loaded"] = "none";
  riceData.save(path);
}

void help() {
  cout << "This is a ricing tool assistant" << endl;
  cout << "rist create riceName - creates a new rice environment and selects it" << endl;
  cout << "rist load riceName - loads target rice" << endl;
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
    initFile(riceDataPath);
  }

  ini::IniFile riceData;
  riceData.load(riceDataPath);

  if (!strcmp(argv[1], "create")) {
    if (argc != 3) {
      cout << "Incorrect usage" << endl;
      return 1;
    };
    // check for duplicates
    for (const auto sectionPair: riceData) {
      const string &riceName = sectionPair.first;
      if (argv[2] == riceName) {
        cout << "ERROR: rice with name " << riceName << " already exists." << endl;
        return 1;
      }
    }
    fs::path riceDir = configPath / argv[2];
    error_code ec;
    if (!fs::create_directory(riceDir, ec)) {
      cerr << "Failed to create rice directory: " << ec.message() << '\n';
      return 1;
    }
    riceData[argv[2]]["path"] = string(riceDir);
    riceData["meta"]["loaded"] = argv[2];
    riceData.save(riceDataPath);
  }

  if (!strcmp(argv[1], "add")) {
    if (argc != 3) {
      cout << "Incorrect usage." << endl;
      return 1;
    }

    fs::path configFile = fs::path(argv[2]);
    string loadedRice = riceData["meta"]["loaded"].as<string>();

    if (loadedRice == "none") {
      cout << "ERROR: create a rice and load it first." << endl;
      return 1;
    }

    if (!riceData[loadedRice]["configs"].as<string>().empty()) {
      riceData[loadedRice]["configs"] = riceData[loadedRice]["configs"].as<string>() + ("," + string(configFile));
    }
    else {
      riceData[loadedRice]["configs"] = riceData[loadedRice]["configs"].as<string>() + string(configFile);
    }

    riceData.save(riceDataPath);

    fs::path originalPath = fs::absolute(argv[2]);
    fs::path configRoot = fs::path(getenv("HOME")) / ".config";
    fs::path relativePath = fs::relative(originalPath, configRoot);
    
    fs::path riceDir = riceData[loadedRice]["path"].as<string>();
    fs::path riceFile = riceDir / relativePath;
    
    fs::create_directories(riceFile.parent_path());
    fs::rename(originalPath, riceFile);  // Preserve the existing config
    fs::create_symlink(riceFile, originalPath);
  }

  if (!strcmp(argv[1], "rm")) {
    if (argc != 3) {
      cout << "Incorrect usage." << endl;
      return 1;
    }
    fs::path configFile = fs::path(argv[2]);
    string loadedRice = riceData["meta"]["loaded"].as<string>();
    if (loadedRice != "none") {
      vector<string> configs = parseList(riceData[loadedRice]["configs"].as<string>(), ",");

      vector<string> remaining;
      for (const auto& config : configs) {
        if (config != argv[2]) {
          remaining.push_back(config);
        }
      }
      
      string configsParsed;
      for (const auto& config : remaining) {
        if (!configsParsed.empty()) {
          configsParsed += ",";
        }
        configsParsed += config;
      }
      
      riceData[loadedRice]["configs"] = configsParsed;
      riceData.save(riceDataPath);
    }
  }

  if (!strcmp(argv[1], "load")) {
    if (argc != 3) {
      cout << "Incorrect usage." << endl;
      return 1;
    }
  }
  return 0;
}
