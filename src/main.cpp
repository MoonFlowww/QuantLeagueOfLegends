#include "riot_api.h"
#include "db.h"
#include "summary.h"
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <array>
#include <cstdio>
#include <unistd.h>

static std::string run_cmd(const std::string &cmd) {
    std::array<char, 4096> buffer;
    std::string out;
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe) return out;
    while (fgets(buffer.data(), buffer.size(), pipe)) out += buffer.data();
    pclose(pipe);
    return out;
}

static MatchSummary parse_match_json(const std::string &json, const std::string &matchId) {
    MatchSummary m;
    m.matchId = matchId;
    // extract timestamp
    char tmpName[] = "/tmp/matchXXXXXX";
    int fd = mkstemp(tmpName);
    if (fd == -1) return m;
    close(fd);
    std::ofstream ofs(tmpName);
    ofs << json;
    ofs.close();
    std::string ts = run_cmd(std::string("jq -r '.info.gameStartTimestamp' ") + tmpName);
    m.gameTimestamp = std::stoll(ts);
    std::string cmd = std::string("jq -r '.info.participants[] | [.summonerName,.teamId,.championId,.teamPosition,.kills,.deaths,.assists,.goldEarned,.win] | @tsv' ") + tmpName;
    std::string out = run_cmd(cmd);
    std::remove(tmpName);
    std::istringstream iss(out);
    std::string line;
    while (std::getline(iss, line)) {
        std::istringstream lineStream(line);
        ParticipantStats p;
        lineStream >> p.summonerName >> p.teamId >> p.championId >> p.lane >> p.kills >> p.deaths >> p.assists >> p.goldEarned >> p.win;
        m.participants.push_back(p);
    }
    return m;
}

int main() {
    const std::string apiKey = "RGAPI-6fa36fe7-477f-4b49-96d3-6bf75f9a277e"; // 24hrs
    const std::string region = "euw1";
    const std::string routing = "europe";
    const std::string summoner = "MoonFloww";

    if (!riot::api_key_valid(apiKey)) {
        std::cerr << "Invalid Riot API key." << std::endl;
        return 1;
    }

    riot::set_verbose(true);
    std::cout << "Fetching PUUID...\n";
    std::string puuid = riot::get_puuid(summoner, apiKey, region);
    if (puuid.empty()) {
        std::cerr << "Failed to get PUUID\n";
        return 1;
    }

    std::cout << "Last how many games? (enter number, or 0 for specific game): ";
    int n; std::cin >> n; std::cin.ignore();
    std::vector<std::string> matchIds;
    if (n > 0) {
        matchIds = riot::get_match_ids(puuid, n, apiKey, routing);
    } else {
        std::string gid; std::cout << "Game ID: "; std::cin >> gid; std::cin.ignore();
        matchIds.push_back(gid);
    }

    if (matchIds.empty()) {
        std::cerr << "No match IDs retrieved. Check your network connection or API key." << std::endl;
        return 1;
    }

    std::vector<MatchSummary> matches;
    for (const auto &id : matchIds) {
        std::cout << "Downloading match " << id << "\n";
        std::string json = riot::get_match(id, apiKey, routing);
        if (json.empty()) {
            std::cerr << "Failed to download match " << id << "\n";
            continue;
        }
        matches.push_back(parse_match_json(json, id));
    }

    OverallSummary sum = compute_summary(matches, summoner);
    print_summary(sum);

    std::cout << "Save to DB? (y/n): ";
    char ans; std::cin >> ans;
    if (ans == 'y' || ans == 'Y') {
        std::string conninfo = "host=localhost port=5432 dbname=leagueoflegends user=player password=xyz";
        PGconn *conn = connect_db(conninfo);
        if (!conn) return 1;
        for (const auto &m : matches) {
            save_match(conn, m);
        }
        PQfinish(conn);
    }
    return 0;
}

