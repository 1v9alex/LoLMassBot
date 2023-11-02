// Newbot.cpp : This file contains the 'main' function. Program execution begins
// and ends there.
//
#define _CRT_SECURE_NO_WARNINGS
// Newbot.cpp : This file contains the 'main' function. Program execution begins
// and ends there.
//

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

#include "LCU.h"
#include "Misc.h"
#include "Utils.h"
#include "json/forwards.h"
#include "json/json.h"

#include <windows.h>
#include <Shlwapi.h> // For PathRemoveFileSpecA

static std::string Login(std::string username, std::string password);

static void LoginOnClientOpen(std::string username, std::string password) {
    while (true) {
        LCU::SetRiotClientInfo();
        if (::FindWindowA("RCLIENT", "Riot Client") && LCU::riot.port != 0) {
            // waits to be sure that client is fully loaded
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
static std::string logout() {
    while (true) {
        LCU::SetRiotClientInfo();
        if (::FindWindowA("RCLIENT", "Riot Client") && LCU::riot.port != 0) {
            // waits to be sure that client is fully loaded
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    cpr::Session session;
    session.SetVerifySsl(false);
    session.SetHeader(Utils::StringToHeader(LCU::riot.header));
    session.SetUrl(std::format(
        "https://127.0.0.1:{}/product-integration/v1/signout", LCU::riot.port));
    std::string result = session.Post().text;

    Json::CharReaderBuilder builder;
    const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    JSONCPP_STRING err;
    Json::Value root;
    if (reader->parse(result.c_str(),
        result.c_str() + static_cast<int>(result.length()), &root,
        &err)) {
        if (!root["type"].empty())
            return root["type"].asString();
        else if (!root["message"].empty())
            return root["message"].asString();
    }
    return result;
}
static std::string Login(std::string username, std::string password) {
    // If riot client not open
    if (LCU::riot.port == 0) {
        if (std::filesystem::exists(LCU::leaguePath)) {
            std::cout << "Launching client...\n";
            Misc::LaunchClient("");
            LoginOnClientOpen(username, password);
        }
        else
            return "Invalid client path, change it in Settings tab";
    }

    cpr::Session session;
    session.SetVerifySsl(false);
    session.SetHeader(Utils::StringToHeader(LCU::riot.header));
    session.SetUrl(std::format("https://127.0.0.1:{}/rso-auth/v2/authorizations",
        LCU::riot.port));
    session.SetBody(
        R"({"clientId":"riot-client","trustLevels":["always_trusted"]})");
    session.Post();
    // refresh session

    std::string loginBody = R"({"username":")" + username + R"(","password":")" +
        password + R"(","persistLogin":false})";
    session.SetUrl(std::format(
        "https://127.0.0.1:{}/rso-auth/v1/session/credentials", LCU::riot.port));
    session.SetBody(loginBody);
    std::string result = session.Put().text;

    Json::CharReaderBuilder builder;
    const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    JSONCPP_STRING err;
    Json::Value root;
    if (reader->parse(result.c_str(),
        result.c_str() + static_cast<int>(result.length()), &root,
        &err)) {
        if (!root["type"].empty())
            return root["type"].asString();
        else if (!root["message"].empty())
            return root["message"].asString();
    }
    return result;
}

std::string g_loginFilePath;

#define MY_BUFSIZE 1024  // Buffer size for console window titles.
std::vector<std::pair<std::string, std::string>> get_login_list() {
    HWND hwndFound;  // This is what is returned to the caller.
    char pszNewWindowTitle[MY_BUFSIZE];  // Contains fabricated
    // WindowTitle.
    char pszOldWindowTitle[MY_BUFSIZE];  // Contains original
    // WindowTitle.

// Fetch current window title.

    GetConsoleTitleA(pszOldWindowTitle, MY_BUFSIZE);

    // Format a "unique" NewWindowTitle.

    wsprintfA(pszNewWindowTitle, "%d/%d", GetTickCount(), GetCurrentProcessId());

    // Change current window title.

    SetConsoleTitleA(pszNewWindowTitle);

    // Ensure window title has been updated.

    Sleep(40);

    // Look for NewWindowTitle.

    hwndFound = FindWindowA(NULL, pszNewWindowTitle);

    // Restore original window title.

    SetConsoleTitleA(pszOldWindowTitle);

    // common dialog box structure, setting all fields to 0 is important
    OPENFILENAME ofn = { 0 };
    TCHAR szFile[260] = { 0 };
    // Initialize remaining fields of OPENFILENAME structure
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwndFound;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = L"All\0*.*\0Text\0*.TXT\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    if (GetOpenFileName(&ofn) == TRUE) {
        // Convert the TCHAR (wide character) file path to a narrow string
        char filePathAnsi[MY_BUFSIZE];
        wcstombs(filePathAnsi, szFile, wcslen(szFile) + 1);
        g_loginFilePath = filePathAnsi; // Save the file path in the global variable

        std::ifstream file(szFile);
        std::vector<std::pair<std::string, std::string>> result;
        for (std::string line; std::getline(file, line); ) {
            auto separator = line.find(':');
            if (separator == std::string::npos) {
                throw std::runtime_error("Error in login file format.");
            }
            std::string username = line.substr(0, separator);
            std::string password = line.substr(separator + 1);
            result.emplace_back(username, password);
        }
        file.close();
        return result;
    }
    throw std::runtime_error("File selection was cancelled or an error occurred.");


    return {};
}

std::string g_friendListFilePath;

bool get_friend_list(std::vector<std::string>& players) {
    HWND hwndFound;  // This is what is returned to the caller.
    char pszNewWindowTitle[MY_BUFSIZE];  // Contains fabricated
    // WindowTitle.
    char pszOldWindowTitle[MY_BUFSIZE];  // Contains original
    // WindowTitle.

// Fetch current window title.

    GetConsoleTitleA(pszOldWindowTitle, MY_BUFSIZE);

    // Format a "unique" NewWindowTitle.

    wsprintfA(pszNewWindowTitle, "%d/%d", GetTickCount(), GetCurrentProcessId());

    // Change current window title.

    SetConsoleTitleA(pszNewWindowTitle);

    // Ensure window title has been updated.

    Sleep(40);

    // Look for NewWindowTitle.

    hwndFound = FindWindowA(NULL, pszNewWindowTitle);

    // Restore original window title.

    SetConsoleTitleA(pszOldWindowTitle);

    // common dialog box structure, setting all fields to 0 is important
    OPENFILENAME ofn = { 0 };
    TCHAR szFile[260] = { 0 };
    // Initialize remaining fields of OPENFILENAME structure
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwndFound;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = L"All\0*.*\0Text\0*.TXT\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    if (GetOpenFileName(&ofn) == TRUE) {
        char filePathAnsi[MAX_PATH];
        wcstombs(filePathAnsi, szFile, wcslen(szFile) + 1);
        g_friendListFilePath = filePathAnsi; // Set the global variable
        char c_szText[260];
        wcstombs(c_szText, szFile, wcslen(szFile) + 1);
        std::ifstream file(szFile);

        for (std::string line; std::getline(file, line);) {
            players.push_back(line);
        }
        return !players.empty();
    }
    return false;
}
void run_mass_friend() {
    HWND hwndFound;  // This is what is returned to the caller.
    char pszNewWindowTitle[MY_BUFSIZE];  // Contains fabricated
    // WindowTitle.
    char pszOldWindowTitle[MY_BUFSIZE];  // Contains original
    // WindowTitle.

// Fetch current window title.

    GetConsoleTitleA(pszOldWindowTitle, MY_BUFSIZE);

    // Format a "unique" NewWindowTitle.

    wsprintfA(pszNewWindowTitle, "%d/%d", GetTickCount(), GetCurrentProcessId());

    // Change current window title.

    SetConsoleTitleA(pszNewWindowTitle);

    // Ensure window title has been updated.

    Sleep(40);

    // Look for NewWindowTitle.

    hwndFound = FindWindowA(NULL, pszNewWindowTitle);

    // Restore original window title.

    SetConsoleTitleA(pszOldWindowTitle);

    // common dialog box structure, setting all fields to 0 is important
    OPENFILENAME ofn = { 0 };
    TCHAR szFile[260] = { 0 };
    // Initialize remaining fields of OPENFILENAME structure
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwndFound;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = L"All\0*.*\0Text\0*.TXT\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    if (GetOpenFileName(&ofn) == TRUE) {
        char c_szText[260];
        wcstombs(c_szText, szFile, wcslen(szFile) + 1);
        std::ifstream file(szFile);

        for (std::string line; std::getline(file, line);) {
            std::cout << line << std::endl;
            std::string invite = R"({"name":")" + line + R"("})";
            std::cout << LCU::Request("POST",
                "https://127.0.0.1/lol-chat/v1/friend-requests",
                invite)
                << std::endl;
        }
    }
}

std::string url_encode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
        std::string::value_type c = (*i);

        // Keep alphanumeric and other accepted characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }

        // Any other characters are percent-encoded
        escaped << std::uppercase;
        escaped << '%' << std::setw(2) << int((unsigned char)c);
        escaped << std::nouppercase;
    }

    return escaped.str();
}

void run_mass_message(std::string& message) {
    std::string getFriends =
        LCU::Request("GET", "https://127.0.0.1/lol-chat/v1/friends");
    Json::CharReaderBuilder builder;
    const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    JSONCPP_STRING err;
    Json::Value root;
    if (reader->parse(getFriends.c_str(),
        getFriends.c_str() + static_cast<int>(getFriends.length()),
        &root, &err)) {
        if (root.isArray()) {

            for (Json::Value::ArrayIndex i = 0; i < root.size(); ++i) {
                std::string friendSummId = root[i]["name"].asString();
                std::cout << friendSummId << std::endl;
                std::string encodedMessage = url_encode(message); // URL-encode the message
                std::string inviteBody = "summonerName=" + friendSummId + "&message=" + encodedMessage;

                std::cout << LCU::Request("POST",
                    "https://127.0.0.1/lol-game-client-chat/v1/instant-messages?" + inviteBody)
                    << std::endl;
            }
            /*for (Json::Value::ArrayIndex i = 0; i < root.size(); ++i) {
                std::string friendSummId = root[i]["name"].asString();
                std::cout << friendSummId << std::endl;
                std::string inviteBody =
                    "summonerName=" + friendSummId + "&message=" + message;
                std::cout << LCU::Request("POST",
                    "https://127.0.0.1/lol-game-client-chat/v1/"
                    "instant-messages?" +
                    inviteBody)
                    << std::endl;
            }*/
        }
    }
}

void save_successful_login(const std::pair<std::string, std::string>& account) {
    std::ofstream outfile("successful_logins.txt", std::ios::app); // Open in append mode
    if (outfile.is_open()) {
        outfile << account.first << ":" << account.second << std::endl;
        outfile.close();
    }
    else {
        std::cerr << "Unable to open file for writing successful logins." << std::endl;
    }
}

void remove_account_and_save(const std::pair<std::string, std::string>& account) {
    // Load all accounts from the original file
    std::ifstream inFile(g_loginFilePath);
    std::vector<std::pair<std::string, std::string>> accounts;
    std::string line;
    while (std::getline(inFile, line)) {
        auto separator = line.find(':');
        if (separator != std::string::npos) {
            std::string username = line.substr(0, separator);
            std::string password = line.substr(separator + 1);
            if (username != account.first) { // If it's not the account we want to remove
                accounts.emplace_back(username, password);
            }
        }
    }
    inFile.close();

    // Write the updated list back to the file
    std::ofstream outFile(g_loginFilePath, std::ios::trunc);
    for (const auto& acc : accounts) {
        outFile << acc.first << ":" << acc.second << "\n";
    }
    outFile.close();

    // Append the successfully logged-in account to the 'successful_logins.txt' file in the program directory
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    PathRemoveFileSpecA(exePath); // Remove the executable name, leaving the directory path
    //GetCurrentDirectoryA(MAX_PATH, programDir); // Get the current directory where the program is running

    std::string successFilePath = std::string(exePath) + "\\successful_logins.txt";

    std::ofstream successFile(successFilePath, std::ios::app);
    if (successFile.is_open()) {
        successFile << account.first << ":" << account.second << "\n"; // Write the account data
        successFile.close(); // Close the file
    }
    else {
        std::cerr << "Unable to open file for writing successful logins." << std::endl;
    }
}

void remove_account_and_save_invalid(const std::pair<std::string, std::string>& account) {
    // Load all accounts from the original file
    std::ifstream inFile(g_loginFilePath);
    std::vector<std::pair<std::string, std::string>> accounts;
    std::string line;
    while (std::getline(inFile, line)) {
        auto separator = line.find(':');
        if (separator != std::string::npos) {
            std::string username = line.substr(0, separator);
            std::string password = line.substr(separator + 1);
            if (username != account.first) { // If it's not the account we want to remove
                accounts.emplace_back(username, password);
            }
        }
    }
    inFile.close();

    // Write the updated list back to the file
    std::ofstream outFile(g_loginFilePath, std::ios::trunc);
    for (const auto& acc : accounts) {
        outFile << acc.first << ":" << acc.second << "\n";
    }
    outFile.close();

    // Append the successfully logged-in account to the 'successful_logins.txt' file in the program directory
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    PathRemoveFileSpecA(exePath); // Remove the executable name, leaving the directory path
    //GetCurrentDirectoryA(MAX_PATH, programDir); // Get the current directory where the program is running

    std::string invalidFilePath = std::string(exePath) + "\\invalid_logins.txt";

    std::ofstream invalidFile(invalidFilePath, std::ios::app);
    if (invalidFile.is_open()) {
        invalidFile << account.first << ":" << account.second << "\n"; // Write the account data
        invalidFile.close(); // Close the file
    }
    else {
        std::cerr << "Unable to open file for writing invalid logins." << std::endl;
    }
}

void remove_friend_and_update_list(const std::string& friendName, const std::string& filePath) {
    // Open the existing file to read the friend list
    std::ifstream inFile(filePath);
    std::vector<std::string> friends;
    std::string line;
    while (std::getline(inFile, line)) {
        if (line != friendName) { // If it's not the friend we want to remove
            friends.push_back(line);
        }
    }
    inFile.close();

    // Write the updated list back to the file
    std::ofstream outFile(filePath, std::ios::trunc);
    for (const auto& name : friends) {
        outFile << name << "\n";
    }
    outFile.close();
}


int main() {
    LPWSTR* szArgList;
    int argCount;
    szArgList = CommandLineToArgvW(GetCommandLineW(), &argCount);
    std::wstring programPath = szArgList[0];
    std::wstring programName =
        programPath.substr(programPath.find_last_of(L"/\\") + 1);
    if (argCount > 1) {
        std::string applicationName =
            Utils::WstringToString(szArgList[1]);  // league
        std::string cmdLine;
        for (int i = 2; i < argCount; i++) {
            cmdLine += "\"" + Utils::WstringToString(szArgList[i]) + "\" ";
        }

        cmdLine.replace(cmdLine.find("\"--no-proxy-server\""),
            strlen("\"--no-proxy-server\""), "");

        AllocConsole();
        FILE* f;
        freopen_s(&f, "CONOUT$", "w", stdout);

        STARTUPINFOA startupInfo;
        memset(&startupInfo, 0, sizeof(STARTUPINFOA));
        startupInfo.cb = sizeof(startupInfo);
        PROCESS_INFORMATION processInformation;
        memset(&processInformation, 0, sizeof(PROCESS_INFORMATION));

        if (!CreateProcessA(applicationName.c_str(),
            const_cast<char*>(cmdLine.c_str()), 0, 0, false, 2U, 0,
            0, &startupInfo, &processInformation))
            return 0;

        std::cout << "App: " << applicationName << std::endl;
        std::cout << "PID: " << processInformation.dwProcessId << std::endl;
        std::cout << "Args: " << cmdLine << std::endl;

        if (!DebugActiveProcessStop(processInformation.dwProcessId)) {
            CloseHandle(processInformation.hProcess);
            CloseHandle(processInformation.hThread);
            fclose(f);
            FreeConsole();
            return 0;
        }

        WaitForSingleObject(processInformation.hProcess, INFINITE);

        std::cout << "Exited" << std::endl;

        CloseHandle(processInformation.hProcess);
        CloseHandle(processInformation.hThread);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        fclose(f);
        FreeConsole();
    }
    else {
#ifndef NDEBUG
        AllocConsole();
        FILE* f;
        freopen_s(&f, "CONOUT$", "w", stdout);
#endif

        std::cout << "Proxy? (Y\N): ";
        char option;
        std::cin >> option;
        if (std::tolower(option) == 'y') {
            std::cout << "Proxy server:\n";
            std::string server;
            std::cin >> server;
            cpr::Proxies proxies = cpr::Proxies{ {"http", server.c_str()} };
            LCU::session.SetProxies(proxies);

            std::string user;
            std::string pass;
            std::cout << "Username:\n";
            std::cin >> user;
            std::cout << "Password\n";
            std::cin >> pass;
            cpr::ProxyAuthentication auth{
                {"http", cpr::EncodedAuthentication{user, pass}}};
            LCU::session.SetProxyAuth(auth);
        }

        std::cout << "Use login file? (Y\N): ";
        char option2;
        std::cin >> option2;
        if (std::tolower(option2) == 'y') {
            auto list = get_login_list();

            std::vector<std::string> friend_targets;
            std::cout << "Friend with all accounts? (Y\N): ";
            char friend_option;
            std::cin >> friend_option;
            bool should_friend_all = std::tolower(friend_option) == 'y';
            if (should_friend_all) {
                // Populate friend_targets once if needed.
                get_friend_list(friend_targets);
            }
            std::cout << "Message with all accounts? (Y\N): ";
            char option3;
            std::cin >> option3;
            std::string message;
            bool should_message_all = std::tolower(option3) == 'y';

            if (should_message_all) {
                std::cout << "Message:\n";
                std::cin.ignore();
                std::getline(std::cin, message);
            }
            std::cout << "League Path\n";
            std::cin.ignore();
            getline(std::cin, LCU::leaguePath);
            std::cout << "Debug: League Path set to '" << LCU::leaguePath << "'\n";


            //std::vector<std::pair<std::string, std::string>> list1 = get_login_list();
            std::vector<std::pair<std::string, std::string>> remaining_logins;

            std::string loginFilePath = g_loginFilePath;


            for (auto& i : list) {
                LCU::GetLeagueProcesses();
                if (LCU::SetLeagueClientInfo()) {
                    // league client is running
                    std::cout << "client running\n";
                    std::cout << "attempting close and logout" << std::endl;
                    system("taskkill /F /T /IM LeagueClient.exe");
                    logout();
                    system("taskkill /F /T /IM RiotClientServices.exe");
                }
                LCU::riot.port = 0;

                // riot client with login screen is up
                auto login_result = Login(i.first, i.second);
                std::cout << login_result << std::endl;


                if (login_result == "authenticated") {
                    std::cout << "Login successful. Checking client readiness..." << std::endl;

                    //save_successful_login(i);
                    std::cout << "Login successful for account: " << i.first << std::endl;

                    int retries = 0;
                    const int maxRetries = 30;
                    bool isClientReady = false;

                    LCU::GetLeagueProcesses();
                    while (!isClientReady && retries < maxRetries) {
                        LCU::GetLeagueProcesses();
                        isClientReady = LCU::SetLeagueClientInfo();
                        if (!isClientReady) {
                            // If the client is not ready, wait a bit before trying again
                            std::this_thread::sleep_for(std::chrono::seconds(1));
                            std::cout << "Waiting for the client to be ready..." << std::endl;
                            retries++;
                        }
                    }

                    if (!isClientReady) {
                        std::cerr << "Client is not ready after waiting. Exiting." << std::endl;
                    }


                    std::cout << "Client is ready. Waiting a bit before doing any actions..." << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(15));

                    remove_account_and_save(i);


                    if (should_message_all) {
                        std::cout << "Messaging all friends..." << std::endl;
                        run_mass_message(message);
                    }

                    std::set<std::string> processedFriends;


                    if (!friend_targets.empty()) {
                        std::cout << "Adding friends..." << std::endl;
                        for (auto it = friend_targets.begin(); it != friend_targets.end(); ) {
                            const std::string& friendName = *it;
                            std::string invite = R"({"name":")" + friendName + R"("})";
                            std::string response = LCU::Request("POST", "https://127.0.0.1/lol-chat/v1/friend-requests", invite);
                            std::cout << friendName << std::endl;
                            std::cout << "Response: " << response << std::endl;

                            Json::CharReaderBuilder builder;
                            JSONCPP_STRING errs;
                            Json::Value jsonResponse;
                            std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
                            if (reader->parse(response.c_str(), response.c_str() + response.size(), &jsonResponse, &errs)) {
                                // The parsing was successful, now check for the error condition
                                if (jsonResponse.isMember("errorCode") && jsonResponse["errorCode"].asString() == "RPC_ERROR" &&
                                    jsonResponse.isMember("httpStatus") && jsonResponse["httpStatus"].asInt() == 500) {
                                    std::cerr << "Error response for POST /chat/v4/friendrequests: " << jsonResponse["message"].asString() << std::endl;
                                    // Log out and break out of the friend_targets loop to proceed to the next account
                                    logout();
                                    break;
                                }
                                else
                                {
                                    processedFriends.insert(friendName);
                                }
                            }
                            it = friend_targets.erase(it); // This removes the current friend and advances the iterator

                            // Now write the updated friend_targets list back to the file
                            std::ofstream outFile(g_friendListFilePath, std::ios::trunc);
                            for (const std::string& remainingFriend : friend_targets) {
                                outFile << remainingFriend << "\n";
                            }
                            outFile.close();
                        }
                    }
                }
                else if (login_result == "needs_credentials" || "needs_multifactor_verification")
                {
                    remove_account_and_save_invalid(i);
                }

            }
        }
        else {
            LCU::GetLeagueProcesses();
            if (LCU::SetLeagueClientInfo()) {
                // league client is running

                std::cout << "client running\n";
            }
            else {
                // riot client with login screen is up
                LCU::SetRiotClientInfo();
                std::cout << "League Path\n";
                std::cin.ignore();
                getline(std::cin, LCU::leaguePath);
                std::cout << "Login\nUsername:\n";
                std::string username;
                std::string password;
                std::cin.ignore();
                std::cin >> username;
                std::cout << "Password:\n";
                std::cin >> password;
                std::cout << Login(username, password) << std::endl;
            }
            while (true) {
                std::cout
                    << "Options:\n1: Mass Friend Request\n2: Mass Message\n3: Exit\n";
                int option = 3;
                std::cin >> option;
                std::string message;
                switch (option) {
                case 1:
                    run_mass_friend();
                    break;
                case 2:
                    std::cout << "Message:\n";
                    std::cin.ignore();
                    std::getline(std::cin, message);
                    run_mass_message(message);
                    break;
                case 3:
                    return 0;
                }
            }
        }
    }
    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started:
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add
//   Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project
//   and select the .sln file