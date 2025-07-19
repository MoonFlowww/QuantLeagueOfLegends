#ifndef RIOT_API_H
#define RIOT_API_H

#include <string>
#include <vector>

namespace riot {
bool api_key_valid(const std::string &apiKey);
void set_verbose(bool v);
std::string get_puuid(const std::string &summonerName,
                      const std::string &apiKey,
                      const std::string &region);
std::vector<std::string> get_match_ids(const std::string &puuid,
                                       int count,
                                       const std::string &apiKey,
                                       const std::string &routing);
std::string get_match(const std::string &matchId,
                      const std::string &apiKey,
                      const std::string &routing);
}

#endif
