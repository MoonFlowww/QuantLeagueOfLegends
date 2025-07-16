#include "db.h"
#include <cstdio>
#include <sstream>

PGconn *connect_db(const std::string &conninfo) {
    PGconn *conn = PQconnectdb(conninfo.c_str());
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        return nullptr;
    }
    return conn;
}

void save_match(PGconn *conn, const MatchSummary &match) {
    const char *create_sql =
        "CREATE TABLE IF NOT EXISTS matches ("\
        "match_id TEXT PRIMARY KEY,"\
        "timestamp BIGINT,"\
        "summoner TEXT,"\
        "champion INT,"\
        "lane TEXT,"\
        "kills INT,"\
        "deaths INT,"\
        "assists INT,"\
        "gold INT,"\
        "win BOOL"\
        ")";
    PGresult *res = PQexec(conn, create_sql);
    PQclear(res);

    for (const auto &p : match.participants) {
        std::stringstream ss;
        ss << "INSERT INTO matches (match_id,timestamp,summoner,champion,lane,kills,deaths,assists,gold,win) VALUES (";
        ss << '\'' << match.matchId << '\'' << ',';
        ss << match.gameTimestamp << ',';
        char *esc = PQescapeLiteral(conn, p.summonerName.c_str(), p.summonerName.size());
        ss << '\'' << esc << '\'' << ',';
        PQfreemem(esc);
        ss << p.championId << ',';
        ss << '\'' << p.lane << '\'' << ',';
        ss << p.kills << ',' << p.deaths << ',' << p.assists << ',' << p.goldEarned << ',' << (p.win ? "true" : "false") << ") ON CONFLICT (match_id) DO NOTHING";
        std::string sql = ss.str();
        res = PQexec(conn, sql.c_str());
        PQclear(res);
    }
    PQexec(conn, "COMMIT");
}

