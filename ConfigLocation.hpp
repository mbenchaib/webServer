#ifndef CONFIGLOCATION_HPP
#define CONFIGLOCATION_HPP

#include <string>
#include <vector>
#include <map>

struct ConfigLocation
{
    std::string              path;
    std::string              root;
    std::string              index;
    std::string              upload_dir;
    bool                     directory_listing;
    int                      redirect_code;
    std::string              redirect_url;
    std::vector<std::string> methods;
    std::map<std::string, std::string> cgi; // ext -> binary path

    ConfigLocation() :
        directory_listing(false),
        redirect_code(0)
    {}

    bool methodAllowed(const std::string &m) const
    {
        for (size_t i = 0; i < methods.size(); i++)
            if (methods[i] == m) return true;
        return false;
    }
};

#endif