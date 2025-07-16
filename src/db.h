#ifndef DB_H
#define DB_H

#include <libpq-fe.h>
#include <string>
#include <vector>

struct ParticipantStats {
    std::string summonerName;
    int teamId;
    int championId;
    std::string lane;
    int kills;
    int deaths;
    int assists;
    int goldEarned;
    bool win;
};

struct MatchSummary {
    std::string matchId;
    long long gameTimestamp;
    std::vector<ParticipantStats> participants;
};

PGconn *connect_db(const std::string &conninfo);
void save_match(PGconn *conn, const MatchSummary &match);

#endif
