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

fs::path removePathPrefix(const fs::path& full_path, const fs::path& prefix) {
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

vector<string> splitString(string s, const string& delimiter) {
  if (s.empty() || delimiter.empty()) {
    return {};
  }

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

int createRice(string riceName) {
  const char* homeDir = getenv("HOME");
  fs::path dotConfig = fs::path(homeDir) / ".config";
  fs::path ristConfigPath = fs::path(homeDir) / fs::path(".config/rist");
  fs::path ricePath = dotConfig / fs::path("rist") / fs::path(riceName);
  fs::path ristIniPath = ristConfigPath / "rist.ini";

  ini::IniFile ristIni;
  ristIni.load(ristIniPath);

  for (const auto sectionPair: ristIni) {
    const string &configRice = sectionPair.first;
    if (riceName == configRice) {
      cout << "ERROR: rice with name " << riceName << " already exists." << endl;
      return 1;
    }
  }

  fs::create_directory(ricePath);

  ristIni["meta"]["loaded"] = riceName;
  ristIni[riceName]["path"] = string(ricePath);

  ristIni.save(ristIniPath);
  return 0;
}

int addConfig(fs::path configFilePath) {
  const char* homeDir = getenv("HOME");
  fs::path dotConfig = fs::path(homeDir) / ".config";
  fs::path ristConfigPath = fs::path(homeDir) / fs::path(".config/rist");
  fs::path relConfigPath = removePathPrefix(configFilePath, dotConfig);

  fs::path ristIniPath = ristConfigPath / "rist.ini";
  ini::IniFile ristIni;
  ristIni.load(ristIniPath);

  string loadedRice = ristIni["meta"]["loaded"].as<string>();
  fs::path ricePath = dotConfig / fs::path("rist") / fs::path(loadedRice);

  if (loadedRice == "none") {
    cout << "Create a rice and load it first";
    return 1;
  }

  if (!ristIni[loadedRice]["configs"].as<string>().empty()) {
    ristIni[loadedRice]["configs"] =  ristIni[loadedRice]["configs"].as<string>() + " ";
  }
  ristIni[loadedRice]["configs"] = ristIni[loadedRice]["configs"].as<string>() + string(relConfigPath);

  fs::path configFilePathInRist = ricePath / relConfigPath;
  fs::create_directories(configFilePathInRist.parent_path());

  fs::rename(configFilePath, configFilePathInRist);
  fs::create_symlink(configFilePathInRist, configFilePath);

  ristIni.save(ristIniPath);

  return 0;
}

int loadRice(string riceName) {
  const char* homeDir = getenv("HOME");
  fs::path dotConfig = fs::path(homeDir) / ".config";
  fs::path ristConfigPath = fs::path(homeDir) / fs::path(".config/rist");
  fs::path ricePath = dotConfig / fs::path("rist") / fs::path(riceName);

  fs::path ristIniPath = ristConfigPath / "rist.ini";
  ini::IniFile ristIni;
  ristIni.load(ristIniPath);

  bool found = false;
  for (const auto &sectionPair: ristIni) {
    if (sectionPair.first == riceName) {
      found = true;
    }
  }
  if (!found) {
    cout << "no rice with name " << riceName << " was found" << endl;
    return 1;
  }

  vector<string> configPaths = splitString(ristIni[riceName]["configs"].as<string>(), " ");
  for (string p: configPaths) {
    fs::path configFilePathInRist = ricePath / p;
    fs::path configFilePath = dotConfig / p;

    if (!fs::exists(configFilePathInRist)) {
      cout << "Missing file in rice: " << configFilePathInRist << endl;
      return 1;
    }
    if (fs::is_symlink(configFilePath)) {
      fs::remove(configFilePath);
    }
    else if (fs::exists(configFilePath)) {
      cout << "Refusing to replace non-symlink: " << configFilePath << endl;
      return 1;
    }

    fs::create_symlink(configFilePathInRist, configFilePath);
  }

  ristIni["meta"]["loaded"] = riceName;
  ristIni.save(ristIniPath);

  return 0;
}

int destroyRice(string riceName) {
  const char* homeDir = getenv("HOME");
  fs::path dotConfig = fs::path(homeDir) / ".config";
  fs::path ristConfigPath = fs::path(homeDir) / fs::path(".config/rist");
  fs::path ricePath = dotConfig / fs::path("rist") / fs::path(riceName);

  fs::path ristIniPath = ristConfigPath / "rist.ini";
  ini::IniFile ristIni;
  ristIni.load(ristIniPath);

  bool found = false;
  for (const auto &sectionPair: ristIni) {
    if (sectionPair.first == riceName) {
      found = true;
    }
  }
  if (!found) {
    cout << "no rice with name " << riceName << " was found" << endl;
    return 1;
  }

  string loadedRice = ristIni["meta"]["loaded"].as<string>();
  if (loadedRice == riceName) {
    cout << "Cannot destroy curreently loaded rice." << endl;
    return 1;
  }

  fs::path riceDir = ristIni[riceName]["path"].as<string>();
  fs::remove_all(riceDir);

  ristIni.erase(riceName);
  ristIni.save(ristIniPath);

  return 0;
}

int removeConfig(fs::path configFilePath) {
  const char* homeDir = getenv("HOME");
  fs::path dotConfig = fs::path(homeDir) / ".config";
  fs::path ristConfigPath = fs::path(homeDir) / fs::path(".config/rist");
  fs::path relConfigPath = removePathPrefix(configFilePath, dotConfig);

  fs::path ristIniPath = ristConfigPath / "rist.ini";
  ini::IniFile ristIni;
  ristIni.load(ristIniPath);

  string loadedRice = ristIni["meta"]["loaded"].as<string>();
  fs::path ricePath = dotConfig / fs::path("rist") / fs::path(loadedRice);

  if (loadedRice == "none") {
    cout << "Create a rice and load it first";
    return 1;
  }

  vector<string> configs = splitString(ristIni[loadedRice]["configs"].as<string>(), " ");

  if (configs.empty()) {
    cout << "there are no config files to remove" << endl;
  }
  for (int i = 0; i < configs.size(); i++) {
    if (configs.at(i) == relConfigPath) {
      configs.erase(configs.begin() + i);

      string updateConfigs;
      for (const auto& config: configs) {
        if (!updatedConfigs.empty()) {
          updatedConfigs += " ";
        }
        updatedConfigs += config;
      }

      if (!fs::is_symlink(configFilePath)) {
        cout << "Path is not a symlink." << endl;
        return 1;
      }
      fs::remove(configFilePath);
      ristIni[loadedRice]["configs"] = updatedConfigs;
      ristIni.save(ristIniPath);
      return 0;
    }
  }

  cout << "unable to find config " << configFilePath << endl;
  return 1;
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
  fs::path configFilePath = fs::path(homeDir) / ".config" / "rist";
  fs::path riceDataPath = fs::path(configFilePath) / "rist.ini";

  if (!fs::exists(configFilePath)) {
    cout << "Creating configuration path at " << configFilePath << endl;
    fs::create_directory(configFilePath);
  }

  if (argc < 3) {
    help();
    return 1;
  }

  if (!fs::exists(riceDataPath)) {
    cout << "Creating rist.ini at " << riceDataPath << endl;
    ini::IniFile riceData;
    riceData["meta"]["loaded"] = "none";
    riceData.save(riceDataPath);
  }

  ini::IniFile riceData;
  riceData.load(riceDataPath);

  if (!strcmp(argv[1], "create")) {
    if (argc != 3) {
      cout << "Incorrect usage" << endl;
      return 1;
    };
 
    return createRice(argv[2]);
  }

  if (!strcmp(argv[1], "destroy")) {
    if (argc != 3) {
      cout << "Incorrect usage." << endl;
      return 1;
    }

    return destroyRice(argv[2]);
  }

  if (!strcmp(argv[1], "add")) {
    if (argc != 3) {
      cout << "Incorrect usage." << endl;
      return 1;
    }

    return addConfig(fs::path(argv[2]));
  }

  if (!strcmp(argv[1], "rm")) {
    if (argc != 3) {
      cerr << "Incorrect usage. Use: rist rm directory\n";
      return 1;
    }

    return removeConfig(argv[2]);
  }

  if (!strcmp(argv[1], "load")) {
    if (argc != 3) {
      cout << "Incorrect usage." << endl;
      return 1;
    }
    return loadRice(argv[2]);
  }

  help();
  return 0;
}
