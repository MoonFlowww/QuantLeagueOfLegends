#include "summary.h"
#include <iostream>

OverallSummary compute_summary(const std::vector<MatchSummary> &matches,
                               const std::string &summoner) {
    OverallSummary res;
    for (const auto &m : matches) {
        for (const auto &p : m.participants) {
            if (p.summonerName != summoner) continue;
            res.games++;
            if (p.win) res.wins++;
            res.avgKills += p.kills;
            res.avgDeaths += p.deaths;
            res.avgAssists += p.assists;
            res.champGames[p.championId]++;
            if (p.win) res.champWins[p.championId]++;
        }
    }
    if (res.games) {
        res.avgKills /= res.games;
        res.avgDeaths /= res.games;
        res.avgAssists /= res.games;
    }
    return res;
}

void print_summary(const OverallSummary &sum) {
    std::cout << "Games: " << sum.games << "\n";
    if (!sum.games) return;
    std::cout << "Win rate: " << (100.0 * sum.wins / sum.games) << "%\n";
    std::cout << "Avg K/D/A: " << sum.avgKills << '/' << sum.avgDeaths << '/' << sum.avgAssists << "\n";
    for (const auto &kv : sum.champGames) {
        int champ = kv.first;
        int games = kv.second;
        int wins = sum.champWins.count(champ) ? sum.champWins.at(champ) : 0;
        double wr = 100.0 * wins / games;
        std::cout << "Champion " << champ << ": " << games << " games, " << wr << "% win rate\n";
    }
}

