#ifndef RIOT_API_H
#define RIOT_API_H

#include <string>
#include <vector>

namespace riot {
bool api_key_valid(const std::string &apiKey);
std::string get_puuid(const std::string &gameName,
                      const std::string &tagLine,
                      const std::string &apiKey,
                      const std::string &routing);
std::vector<std::string> get_match_ids(const std::string &puuid,
                                       int count,
                                       const std::string &apiKey,
                                       const std::string &routing);
std::string get_match(const std::string &matchId,
                      const std::string &apiKey,
                      const std::string &routing);
}

#endif
