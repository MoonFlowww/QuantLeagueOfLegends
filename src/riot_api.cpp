#include "riot_api.h"
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <array>
#include <string>
#include <unistd.h>
#include <iostream>

namespace riot {

bool api_key_valid(const std::string &apiKey) {
    if (apiKey.size() != 42) return false;
    if (apiKey.rfind("RGAPI-", 0) != 0) return false;
    for (size_t i = 6; i < apiKey.size(); ++i) {
        char c = apiKey[i];
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-') return false;
    }
    return true;
}

static std::string exec(const std::string &cmd) {
    std::array<char, 4096> buffer;
    std::string result;
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe) return result;
    while (fgets(buffer.data(), buffer.size(), pipe)) {
        result += buffer.data();
    }
    pclose(pipe);
    return result;
}

// Fetch a URL using curl, capturing the HTTP status code along with the body.
// On error, status will be set to 0 and an empty string is returned.
static std::string fetch(const std::string &url,
                         const std::string &apiKey,
                         long &status) {
    std::string cmd = "curl -s -w '%{http_code}' -H 'X-Riot-Token: " + apiKey +
                      "' '" + url + "'";
    std::string out = exec(cmd);
    if (out.size() < 3) {
        status = 0;
        return "";
    }
    std::string codeStr = out.substr(out.size() - 3);
    status = std::strtol(codeStr.c_str(), nullptr, 10);
    return out.substr(0, out.size() - 3);
}

static std::string jq_extract(const std::string &json, const std::string &filter) {
    char tmpName[] = "/tmp/jsonXXXXXX";
    int fd = mkstemp(tmpName);
    if (fd == -1) return "";
    close(fd);
    std::ofstream ofs(tmpName);
    ofs << json;
    ofs.close();
    std::string cmd = std::string("jq -r '") + filter + "' " + tmpName;
    std::string out = exec(cmd);
    std::remove(tmpName);
    return out;
}

std::string get_puuid(const std::string &gameName,
                      const std::string &tagLine,
                      const std::string &apiKey,
                      const std::string &routing) {
    if (!api_key_valid(apiKey)) return "";
    std::string url = "https://" + routing +
                       ".api.riotgames.com/riot/account/v1/accounts/by-riot-id/" +
                       gameName + "/" + tagLine;
    long status = 0;
    std::string json = fetch(url, apiKey, status);
    if (status != 200) {
        std::cerr << "get_puuid failed with HTTP status " << status << std::endl;
        return "";
    }
    return jq_extract(json, ".puuid");
}

std::vector<std::string> get_match_ids(const std::string &puuid,
                                       int count,
                                       const std::string &apiKey,
                                       const std::string &routing) {
    if (!api_key_valid(apiKey)) return {};
    if (count <= 0) return {};
    std::string url =
        "https://" + routing +
        ".api.riotgames.com/lol/match/v5/matches/by-puuid/" + puuid +
        "/ids?count=" + std::to_string(count);
    long status = 0;
    std::string json = fetch(url, apiKey, status);
    if (status != 200) {
        std::cerr << "get_match_ids failed with HTTP status " << status << std::endl;
        return {};
    }
    std::string ids = jq_extract(json, ".[]");
    std::vector<std::string> result;
    std::istringstream iss(ids);
    std::string tmp;
    while (std::getline(iss, tmp)) {
        if (!tmp.empty()) result.push_back(tmp);
    }
    return result;
}

std::string get_match(const std::string &matchId,
                      const std::string &apiKey,
                      const std::string &routing) {
    if (!api_key_valid(apiKey)) return "";
    std::string url = "https://" + routing +
                       ".api.riotgames.com/lol/match/v5/matches/" + matchId;
    long status = 0;
    std::string json = fetch(url, apiKey, status);
    if (status != 200) {
        std::cerr << "get_match failed with HTTP status " << status << std::endl;
        return "";
    }
    return json;
}

} // namespace riot

