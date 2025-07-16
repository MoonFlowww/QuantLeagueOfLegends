#ifndef SUMMARY_H
#define SUMMARY_H

#include "db.h"
#include <map>
#include <string>
#include <vector>

struct OverallSummary {
    int games = 0;
    int wins = 0;
    double avgKills = 0;
    double avgDeaths = 0;
    double avgAssists = 0;
    std::map<int, int> champGames;
    std::map<int, int> champWins;
};

OverallSummary compute_summary(const std::vector<MatchSummary> &matches,
                               const std::string &summoner);
void print_summary(const OverallSummary &sum);

#endif
