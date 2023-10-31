#pragma once

#include <tlhelp32.h>

#include <filesystem>
#include <fstream>
#include <thread>

#include "json/json.h"

#include "LCU.h"

#ifdef _MSC_VER
#include <Shlwapi.h>
#define istrstr StrStrIA
#pragma comment(lib, "Shlwapi.lib")
#else
#define istrstr strcasestr
#endif
#include <format>

class Misc {
 public:
  static inline std::string programVersion = "1.5.3";
  static inline std::string latestVersion = "";

  static bool LaunchClient(const std::string args) {
    const std::string path =
        std::format("{}LeagueClient.exe", LCU::leaguePath).c_str();
    ShellExecuteA(NULL, "open", path.c_str(), args.c_str(), NULL,
                  SW_SHOWNORMAL);
    return false;
  }

  static void LaunchLegacyClient() {
    if (!std::filesystem::exists(
            std::format("{}LoL Companion", LCU::leaguePath))) {
      std::filesystem::create_directory(
          std::format("{}LoL Companion", LCU::leaguePath));
    }
    if (!std::filesystem::exists(
            std::format("{}LoL Companion/system.yaml", LCU::leaguePath))) {
      std::ifstream infile(std::format("{}system.yaml", LCU::leaguePath));
      std::ofstream outfile(
          std::format("{}LoL Companion/system.yaml", LCU::leaguePath));
      std::string content = "";

      std::string temp;
      while (std::getline(infile, temp)) content += temp + "\n";

      infile.close();
      size_t pos = content.find("riotclient:");
      content = content.substr(0, pos + 11);

      outfile << content;
      outfile.close();
    }

    if (::FindWindowA("RCLIENT", "League of Legends")) {
      LCU::Request("POST", "https://127.0.0.1/process-control/v1/process/quit");

      // wait for client to close (maybe theres a better method of doing that)
      std::this_thread::sleep_for(std::chrono::milliseconds(4500));
    }

    ShellExecuteA(
        NULL, "open",
        std::format("{}LeagueClient.exe", LCU::leaguePath).c_str(),
        std::format("--system-yaml-override=\"{}LoL Companion/system.yaml\"",
                    LCU::leaguePath)
            .c_str(),
        NULL, SW_SHOWNORMAL);
  }

  //static void CheckVersion() {
  //  std::string getLatest =
  //      cpr::Get(
  //          cpr::Url{
  //              "https://api.github.com/repos/KebsCS/KBotExt/releases/latest"})
  //          .text;
  //
  //  Json::CharReaderBuilder builder;
  //  const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
  //  JSONCPP_STRING err;
  //  Json::Value root;
  //  if (reader->parse(getLatest.c_str(),
  //                    getLatest.c_str() + static_cast<int>(getLatest.length()),
  //                    &root, &err)) {
  //    std::string latestTag = root["tag_name"].asString();
  //    Misc::latestVersion = latestTag;
  //
  //    std::vector<std::string> latestNameSplit =
  //        Utils::StringSplit(latestTag, ".");
  //    std::vector<std::string> programVersionSplit =
  //        Utils::StringSplit(Misc::programVersion, ".");
  //
  //    for (size_t i = 0; i < 2; i++) {
  //      if (latestNameSplit[i] != programVersionSplit[i]) {
  //        if (MessageBoxA(0, "Open download website?",
  //                        "New major version available",
  //                        MB_YESNO | MB_SETFOREGROUND) == IDYES) {
  //          ShellExecuteW(0, 0,
  //                        L"https://github.com/KebsCS/KBotExt/releases/latest",
  //                        0, 0, SW_SHOW);
  //        }
  //      }
  //    }
  //    if (latestTag != Misc::programVersion &&
  //        std::find(LCU::ignoredVersions.begin(), LCU::ignoredVersions.end(),
  //                  latestTag) == LCU::ignoredVersions.end()) {
  //      const auto status = MessageBoxA(
  //          0, "Open download website?\nCancel to ignore this version forever",
  //          "New minor update available", MB_YESNOCANCEL | MB_SETFOREGROUND);
  //      if (status == IDYES) {
  //        ShellExecuteW(0, 0,
  //                      L"https://github.com/KebsCS/KBotExt/releases/latest", 0,
  //                      0, SW_SHOW);
  //      } else if (status == IDCANCEL) {
  //        LCU::ignoredVersions.emplace_back(latestTag);
  //      }
  //    }
  //  }
  //}

  static std::string GetCurrentPatch() {
    std::string result =
        cpr::Get(
            cpr::Url{"http://ddragon.leagueoflegends.com/api/versions.json"})
            .text;
    Json::CharReaderBuilder builder;
    const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    JSONCPP_STRING err;
    Json::Value root;
    if (reader->parse(result.c_str(),
                      result.c_str() + static_cast<int>(result.length()), &root,
                      &err)) {
      return root[0].asString();
    }
    return "0.0.0";
  }

  static void GetAllChampionSkins() {
    std::string getSkins =
        cpr::Get(cpr::Url{"https://raw.communitydragon.org/latest/plugins/"
                          "rcp-be-lol-game-data/global/default/v1/skins.json"})
            .text;
    Json::CharReaderBuilder builder;
    const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    JSONCPP_STRING err;
    Json::Value root;
    if (!reader->parse(getSkins.c_str(),
                       getSkins.c_str() + static_cast<int>(getSkins.length()),
                       &root, &err))
      return;

    std::map<std::string, Champ> champs;
    for (const std::string& id : root.getMemberNames()) {
      const Json::Value currentSkin = root[id];

      std::string loadScreenPath = currentSkin["loadScreenPath"].asString();
      size_t nameStart = loadScreenPath.find("ASSETS/Characters/") +
                         strlen("ASSETS/Characters/");
      std::string champName = loadScreenPath.substr(
          nameStart, loadScreenPath.find("/", nameStart) - nameStart);

      std::string name = currentSkin["name"].asString();

      std::pair<std::string, std::string> skin;
      if (currentSkin["isBase"].asBool() == true) {
        champs[champName].name = champName;

        std::string splashPath = currentSkin["splashPath"].asString();
        size_t keyStart = splashPath.find("champion-splashes/") +
                          strlen("champion-splashes/");
        std::string champKey = splashPath.substr(
            keyStart, splashPath.find("/", keyStart) - keyStart);

        champs[champName].key = std::stoi(champKey);
        skin.first = id;
        skin.second = "default";
        champs[champName].skins.insert(champs[champName].skins.begin(), skin);
      } else {
        if (currentSkin["questSkinInfo"])  // K/DA ALL OUT Seraphine
        {
          const Json::Value skinTiers = currentSkin["questSkinInfo"]["tiers"];
          for (Json::Value::ArrayIndex i = 0; i < skinTiers.size(); i++) {
            skin.first = skinTiers[i]["id"].asString();
            skin.second = skinTiers[i]["name"].asString();
            champs[champName].skins.emplace_back(skin);
          }
        } else {
          skin.first = id;
          skin.second = name;
          champs[champName].skins.emplace_back(skin);
        }
      }
    }

    std::vector<Champ> temp;
    for (const auto& c : champs) {
      temp.emplace_back(c.second);
    }
    champSkins = temp;
  }

  static void TaskKillLeague() {
    std::vector<std::wstring> leagueProcs = {
        L"RiotClientCrashHandler.exe", L"RiotClientServices.exe",
        L"RiotClientUx.exe",           L"RiotClientUxRender.exe",

        L"LeagueCrashHandler.exe",     L"LeagueClient.exe",
        L"LeagueClientUx.exe",         L"LeagueClientUxRender.exe"};

    for (const auto& proc : leagueProcs) {
      Misc::TerminateProcessByName(proc);
    }
  }

  static std::string ChampIdToName(int id) {
    if (!id) {
      return "None";
    } else if (champSkins.empty()) {
      return "No data";  // "Champion data is still being fetched";
    }
    {
      for (const auto& c : champSkins) {
        if (c.key == id) return c.name;
      }
    }
    return "";
  }

  // Terminate all league related processes,
  // remove read only and hidden property from files
  // and delete them
  static std::string ClearLogs() {
    std::string result = "";

    TaskKillLeague();

    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::error_code errorCode;

    auto leaguePath = std::filesystem::path(LCU::leaguePath);

    auto riotClientPath = std::filesystem::path(LCU::leaguePath.substr(
                              0, LCU::leaguePath.find_last_of(
                                     "/\\", LCU::leaguePath.size() - 2))) /
                          "Riot Client";

    char* pLocal;
    size_t localLen;
    _dupenv_s(&pLocal, &localLen, "LOCALAPPDATA");
    auto localAppData = std::filesystem::path(pLocal);

    std::vector<std::filesystem::path> leagueFiles = {
        leaguePath / "Logs",
        leaguePath / "Config",
        leaguePath / "debug.log",
        riotClientPath / "UX" / "natives_blob.bin",
        riotClientPath / "UX" / "snapshot_blob.bin",
        riotClientPath / "UX" / "v8_context_snapshot.bin",
        riotClientPath / "UX" / "icudtl.dat",
        localAppData / "Riot Games"};

    for (const auto& file : leagueFiles) {
      if (std::filesystem::exists(file)) {
        SetFileAttributesA(file.string().c_str(),
                           GetFileAttributesA(file.string().c_str()) &
                               ~FILE_ATTRIBUTE_READONLY &
                               ~FILE_ATTRIBUTE_HIDDEN);
        std::filesystem::remove_all(file, errorCode);
        result += file.string() + " - " + errorCode.message() + "\n";
      }
    }

    int counter = 0;
    for (const auto& file : std::filesystem::directory_iterator(
             std::filesystem::temp_directory_path())) {
      std::filesystem::remove_all(file, errorCode);
      counter++;
    }
    result +=
        "Deleted " + std::to_string(counter) + " files in temp directory\n";
    return result;
  }

  // returns true on success
  static bool TerminateProcessByName(std::wstring processName) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    bool result = false;
    if (snapshot != INVALID_HANDLE_VALUE) {
      PROCESSENTRY32W entry;
      entry.dwSize = sizeof(PROCESSENTRY32W);
      if (Process32FirstW(snapshot, &entry)) {
        do {
          if (std::wstring(entry.szExeFile) == processName) {
            HANDLE process =
                OpenProcess(PROCESS_TERMINATE, false, entry.th32ProcessID);
            bool terminate = TerminateProcess(process, 0);
            CloseHandle(process);
            result = terminate;
          }
        } while (Process32NextW(snapshot, &entry));
      }
    }
    CloseHandle(snapshot);
    return result;
  }
};

int FuzzySearch(const char* needle, void* data) {
  auto& items = *(std::vector<std::string>*)data;
  for (int i = 0; i < (int)items.size(); i++) {
    auto haystack = items[i].c_str();
    // empty
    if (!needle[0]) {
      if (!haystack[0]) return i;
      continue;
    }
    // exact match
    if (strstr(haystack, needle)) return i;
    // fuzzy match
    if (istrstr(haystack, needle)) return i;
  }
  return -1;
}

static bool itemsGetter(void* data, int n, const char** out_str) {
  auto& items = *(std::vector<std::string>*)data;
  if (n >= 0 && n < (int)items.size()) {
    *out_str = items[n].c_str();
    return true;
  }
  return false;
}