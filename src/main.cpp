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

fs::path remove_prefix(const fs::path& full_path, const fs::path& prefix) {
  auto p_it = full_path.begin();
  auto pre_it = prefix.begin();

  while (p_it != full_path.end() && pre_it != prefix.end() && *p_it == *pre_it) {
    ++p_it;
    ++pre_it;
  }

  if (pre_it == prefix.end()) {
    fs::path result;
    for (; p_it != full_path.end(); ++p_it) {
      result /= *p_it;
    }
    return result;
  }

  return full_path;
}

vector<string> parseString(string s, const string& delimiter) {
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

  if (!strcmp(argv[1], "destroy")) {
    if (argc != 3) {
      cout << "Incorrect usage." << endl;
      return 1;
    }

    bool found = false;
    for (const auto &sectionPair: riceData) {
      if (sectionPair.first == argv[2]) {
        found = true;
      }
    }
    if (!found) {
      cout << argv[2] << " not an existing rice." << endl;
      return 1;
    }
    fs::remove_all(riceData[argv[2]]["path"]);
  }

  if (!strcmp(argv[1], "add")) {
    if (argc != 3) {
      cout << "Incorrect usage." << endl;
      return 1;
    }

    fs::path configFile = fs::path(argv[2]);
    fs::path configRoot = fs::path(getenv("HOME")) / ".config";
    fs::path relConfigFile = remove_prefix(configFile, configRoot);

    cout << "Relative config file: " << relConfigFile << endl;

    string loadedRice = riceData["meta"]["loaded"].as<string>();

    if (loadedRice == "none") {
      cout << "ERROR: create a rice and load it first." << endl;
      return 1;
    }

    if (!riceData[loadedRice]["configs"].as<string>().empty()) {
      riceData[loadedRice]["configs"] = riceData[loadedRice]["configs"].as<string>() + ("," + string(relConfigFile));
    }
    else {
      riceData[loadedRice]["configs"] = riceData[loadedRice]["configs"].as<string>() + string(relConfigFile);
    }

    riceData.save(riceDataPath);

    fs::path originalPath = fs::absolute(argv[2]);
    fs::path relativePath = fs::relative(originalPath, configRoot);

    fs::path riceDir = riceData[loadedRice]["path"].as<string>();
    fs::path riceFile = riceDir / relativePath;

    fs::create_directories(riceFile.parent_path());
    fs::rename(originalPath, riceFile);
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
      vector<string> configs = parseString(riceData[loadedRice]["configs"].as<string>(), ",");

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
      fs::remove(originalPath);
      fs::rename(riceFile, originalPath);
    }
  }

  if (!strcmp(argv[1], "load")) {
    if (argc != 3) {
      cout << "Incorrect usage." << endl;
      return 1;
    }
    bool found = false;
    for (const auto &sectionPair: riceData) {
      if (sectionPair.first == argv[2]) {
        found = true;
      }
    }
    if (!found) {
      cout << argv[2] << " not an existing rice." << endl;
      return 1;
    }

    vector<string> paths = parseString(riceData[argv[2]]["configs"].as<string>(), ",");
    fs::path loadRicePath = configPath / fs::path(argv[2]);
    fs::path configDir = fs::path(homeDir) / fs::path(".config");

    for (string p: paths) {
      fs::path target = loadRicePath / fs::path(p);
      fs::path link = configDir / fs::path(p);
      fs::create_symlink(target, link);
    }
  }
  return 0;
}
