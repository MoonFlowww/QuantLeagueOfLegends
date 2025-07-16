#include "riot_api.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <array>
#include <string>
#include <unistd.h>

namespace riot {

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
    std::string url = "https://" + routing + ".api.riotgames.com/riot/account/v1/accounts/by-riot-id/" +
                       gameName + "/" + tagLine;
    std::string cmd = "curl -s -H 'X-Riot-Token: " + apiKey + "' '" + url + "'";
    std::string json = exec(cmd);
    return jq_extract(json, ".puuid");
}

std::vector<std::string> get_match_ids(const std::string &puuid,
                                       int count,
                                       const std::string &apiKey,
                                       const std::string &routing) {
    std::string url = "https://" + routing + ".api.riotgames.com/lol/match/v5/matches/by-puuid/" +
                       puuid + "/ids?count=" + std::to_string(count);
    std::string cmd = "curl -s -H 'X-Riot-Token: " + apiKey + "' '" + url + "'";
    std::string json = exec(cmd);
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
    std::string url = "https://" + routing + ".api.riotgames.com/lol/match/v5/matches/" + matchId;
    std::string cmd = "curl -s -H 'X-Riot-Token: " + apiKey + "' '" + url + "'";
    return exec(cmd);
}

} // namespace riot

