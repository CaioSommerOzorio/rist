#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

#include "../external/inicpp.h"

namespace fs = std::filesystem;
using namespace std;

fs::path removePathPrefix(const fs::path &full_path, const fs::path &prefix) {
  auto p_it = full_path.begin();
  auto pre_it = prefix.begin();

  while (p_it != full_path.end() && pre_it != prefix.end() &&
         *p_it == *pre_it) {
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

vector<string> splitString(string s, const string &delimiter) {
  if (s.empty() || delimiter.empty()) {
    return {};
  }

  vector<string> tokens;
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
  const char *homeDir = getenv("HOME");
  fs::path dotConfig = fs::path(homeDir) / ".config";
  fs::path ristConfigPath = fs::path(homeDir) / fs::path(".config/rist");
  fs::path ricePath = dotConfig / fs::path("rist") / fs::path(riceName);
  fs::path ristIniPath = ristConfigPath / "rist.ini";

  ini::IniFile ristIni;
  ristIni.load(ristIniPath);

  for (const auto sectionPair : ristIni) {
    const string &configRice = sectionPair.first;
    if (riceName == configRice) {
      cout << "ERROR: rice with name " << riceName << " already exists."
           << endl;
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
  const char *homeDir = getenv("HOME");
  fs::path dotConfig = fs::path(homeDir) / ".config";
  fs::path ristConfigPath = dotConfig / "rist";
  fs::path relConfigPath = removePathPrefix(configFilePath, dotConfig);

  fs::path ristIniPath = ristConfigPath / "rist.ini";
  ini::IniFile ristIni;
  ristIni.load(ristIniPath);

  string loadedRice = ristIni["meta"]["loaded"].as<string>();

  if (loadedRice == "none") {
    cout << "Create a rice and load it first" << endl;
    return 1;
  }

  fs::path ricePath = ristConfigPath / loadedRice;
  fs::path configFilePathInRist = ricePath / relConfigPath;

  if (!fs::exists(configFilePath)) {
    cerr << "Config file does not exist: " << configFilePath << endl;
    return 1;
  }

  vector<string> configs =
      splitString(ristIni[loadedRice]["configs"].as<string>(), " ");

  string relativePath = relConfigPath.string();

  if (find(configs.begin(), configs.end(), relativePath) != configs.end()) {
    cerr << "Config already added: " << relativePath << endl;
    return 1;
  }

  fs::path realSource = fs::canonical(configFilePath);
  fs::create_directories(configFilePathInRist.parent_path());

  fs::copy_file(realSource, configFilePathInRist);

  if (fs::exists(configFilePath) || fs::is_symlink(configFilePath)) {
    fs::remove(configFilePath);
  }

  fs::create_symlink(fs::absolute(configFilePathInRist), configFilePath);

  if (!configs.empty()) {
    ristIni[loadedRice]["configs"] =
        ristIni[loadedRice]["configs"].as<string>() + " ";
  }

  ristIni[loadedRice]["configs"] =
      ristIni[loadedRice]["configs"].as<string>() + relativePath;

  ristIni.save(ristIniPath);

  return 0;
}

int loadRice(string riceName) {
  const char *homeDir = getenv("HOME");
  fs::path dotConfig = fs::path(homeDir) / ".config";
  fs::path ristConfigPath = fs::path(homeDir) / fs::path(".config/rist");
  fs::path ricePath = dotConfig / fs::path("rist") / fs::path(riceName);

  fs::path ristIniPath = ristConfigPath / "rist.ini";
  ini::IniFile ristIni;
  ristIni.load(ristIniPath);

  bool found = false;
  for (const auto &sectionPair : ristIni) {
    if (sectionPair.first == riceName) {
      found = true;
    }
  }
  if (!found) {
    cout << "no rice with name " << riceName << " was found" << endl;
    return 1;
  }

  vector<string> configPaths =
      splitString(ristIni[riceName]["configs"].as<string>(), " ");
  for (string p : configPaths) {
    fs::path configFilePathInRist = ricePath / p;
    fs::path configFilePath = dotConfig / p;

    if (!fs::exists(configFilePathInRist)) {
      cout << "Missing file in rice: " << configFilePathInRist << endl;
      return 1;
    }
    if (fs::is_symlink(configFilePath)) {
      fs::remove(configFilePath);
    } else if (fs::exists(configFilePath)) {
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
  const char *homeDir = getenv("HOME");
  fs::path dotConfig = fs::path(homeDir) / ".config";
  fs::path ristConfigPath = fs::path(homeDir) / fs::path(".config/rist");
  fs::path ricePath = dotConfig / fs::path("rist") / fs::path(riceName);

  fs::path ristIniPath = ristConfigPath / "rist.ini";
  ini::IniFile ristIni;
  ristIni.load(ristIniPath);

  bool found = false;
  for (const auto &sectionPair : ristIni) {
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

  fs::path riceDir = ristConfigPath / riceName;
  fs::remove_all(riceDir);

  ristIni.erase(riceName);
  ristIni.save(ristIniPath);

  return 0;
}

int removeConfig(fs::path configFilePath) {
  const char *homeDir = getenv("HOME");
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

  vector<string> configs =
      splitString(ristIni[loadedRice]["configs"].as<string>(), " ");

  if (configs.empty()) {
    cout << "there are no config files to remove" << endl;
    return 1;
  }
  for (int i = 0; i < configs.size(); i++) {
    if (configs.at(i) == relConfigPath.string()) {
      if (!fs::is_symlink(configFilePath)) {
        cout << "Path is not a symlink." << endl;
        return 1;
      }
      fs::remove(configFilePath);
      configs.erase(configs.begin() + i);

      string updatedConfigs;
      for (const auto &config : configs) {
        if (!updatedConfigs.empty()) {
          updatedConfigs += " ";
        }
        updatedConfigs += config;
      }
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
  cout << "rist create riceName - creates a new rice environment and selects it"
       << endl;
  cout << "rist load riceName - loads target rice" << endl;
  cout << "rist destroy riceName - removes target rice" << endl;
  cout << "rist add directory - adds a directory to selected rice" << endl;
  cout << "rist rm directory - removes a directory from selected rice" << endl;
  cout << "rist name - lists the name of the current loaded rice" << endl;
  cout << "rist rices - lists the available rices" << endl;
  cout << "rist configs - lists the currently loaded configs" << endl;
}

int main(int argc, char *argv[]) {
  const char *homeDir = getenv("HOME");
  if (!homeDir) {
    cerr << "HOME environment variable not set" << endl;
    return 1;
  }
  fs::path configFilePath = fs::path(homeDir) / ".config" / "rist";
  fs::path ristDataPath = fs::path(configFilePath) / "rist.ini";

  if (!fs::exists(configFilePath)) {
    cout << "Creating configuration path at " << configFilePath << endl;
    fs::create_directory(configFilePath);
  }

  if (argc == 1) {
    help();
    return 0;
  }

  if (argc < 3) {
    if (!strcmp(argv[1], "name")) {
      ini::IniFile ristIni;
      ristIni.load(ristDataPath);
      cout << "Loaded rice: " << ristIni["meta"]["loaded"].as<string>() << endl;
      return 0;
    } else if (!strcmp(argv[1], "rices")) {
      ini::IniFile ristIni;
      ristIni.load(ristDataPath);
      for (const auto &sectionPair : ristIni) {
        string riceName = sectionPair.first;
        if (riceName != "meta")
          cout << riceName << endl;
      }
      return 0;
    } else if (!strcmp(argv[1], "configs")) {
      ini::IniFile ristIni;
      ristIni.load(ristDataPath);
      vector<string> configs =
          splitString(ristIni[ristIni["meta"]["loaded"].as<string>()]["configs"]
                          .as<string>(),
                      " ");
      if (configs.empty()) {
        cout << "No configs loaded" << endl;
        return 0;
      }
      for (int i = 0; i < configs.size(); i++) {
        cout << configs.at(i) << endl;
      }

      // maybe im a stupid larp but this is evil
      // and it wont work
      // can't wait to come back to this in like
      // a day and it just be the most obvious thing ever
      // for (const auto &config : configs) {
      //  cout << config << endl;
      //  return 0;
      //}
      return 0;
    }
    cout << "Command not recognised. Run rist help to print a help message of "
            "available commands."
         << endl;
    return 1;
  }

  if (!fs::exists(ristDataPath)) {
    cout << "Creating rist.ini at " << ristDataPath << endl;
    ini::IniFile ristIni;
    ristIni["meta"]["loaded"] = "none";
    ristIni.save(ristDataPath);
  }

  if (!strcmp(argv[1], "create")) {
    if (argc != 3) {
      cout << "Incorrect usage: please supply a rice name" << endl;
      return 1;
    } else if (!strcmp(argv[2], "meta")) {
      cout << "Incorrect usage: rice name cannot be meta" << endl;
      return 1;
    }

    return createRice(argv[2]);
  }

  if (!strcmp(argv[1], "destroy")) {
    if (argc != 3) {
      cout << "Incorrect usage: please supply a rice name to destroy" << endl;
      return 1;
    } else if (!strcmp(argv[2], "meta")) {
      cout << "Incorrect usage: cannot detroy meta" << endl;
      return 1;
    }

    return destroyRice(argv[2]);
  }

  if (!strcmp(argv[1], "add")) {
    if (argc != 3) {
      cout << "Incorrect usage: please supply a config file" << endl;
      return 1;
    }

    return addConfig(fs::absolute(fs::path(argv[2])));
  }

  if (!strcmp(argv[1], "rm")) {
    if (argc != 3) {
      cerr << "Incorrect usage: please supply a directory to remove" << endl;
      return 1;
    }

    return removeConfig(fs::absolute(fs::path(argv[2])));
  }

  if (!strcmp(argv[1], "load")) {
    if (argc != 3) {
      cout << "Incorrect usage: please supply a rice to load" << endl;
      return 1;
    }
    return loadRice(argv[2]);
  }

  help();
  return 0;
}
