#ifndef CONFIGSERVER_HPP
#define CONFIGSERVER_HPP

#include "ConfigLocation.hpp"
#include <string>
#include <vector>
#include <map>

struct ConfigServer
{
    std::string                     host;
    int                             port;
    long                            max_body_size;
    std::map<int, std::string>      error_pages;  // code -> path
    std::vector<ConfigLocation>     locations;

    ConfigServer() :
        host("0.0.0.0"),
        port(8080),
        max_body_size(1000000)
    {}

    // find the best matching location for a given path
    ConfigLocation *matchLocation(const std::string &path)
    {
        ConfigLocation *best = NULL;
        size_t best_len = 0;

        for (size_t i = 0; i < locations.size(); i++)
        {
            std::string &loc = locations[i].path;
            if (path.substr(0, loc.size()) == loc)
            {
                if (loc.size() > best_len)
                {
                    best_len = loc.size();
                    best = &locations[i];
                }
            }
        }
        return best;
    }
};

#endif