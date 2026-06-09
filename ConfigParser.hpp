/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mben-cha <mben-cha@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/29 20:40:02 by mben-cha          #+#    #+#             */
/*   Updated: 2026/06/08 23:22:04 by mben-cha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <vector>

struct Directive
{
    std::string name;
    std::vector<std::string> values;

    const std::vector<std::string>& getValues()
    {
        return (values);
    }
};

struct Location
{
    std::string path;
    std::vector<Directive> directives;

    Directive* getDirective(const std::string& name)
    {
        for (size_t i = 0; i < directives.size(); i++)
        {
            if (directives[i].name == name)
                return (&(directives[i]));
        }
        return (NULL);
    }

    std::string getCgiHandler(const std::string& extension)
    {
        for (size_t i = 0; i < directives.size(); i++)
        {
            if (directives[i].name == "cgi")
            {
                for (size_t j = 0; j < directives[i].values.size(); j++)
                {
                    if (extension == directives[i].values[j])
                        return (directives[i].values.back());
                }
            }
        }
        return ("");
    }

    std::string getRoot()
    {
        Directive* dir;
        if ((dir = getDirective("root")))
            return (dir->getValues()[0]);
        return ("");
    }
};

struct Server
{
    std::vector<Directive> directives;
    std::vector<Location> locations;

    const std::vector<Location>& getLocations()
    {
        return (locations);
    }

    Directive* getDirective(const std::string& name)
    {
        for (size_t i = 0; i < directives.size(); i++)
        {
            if (directives[i].name == name)
                return (&(directives[i]));
        }
        return (NULL);
    }

    std::string getErrorPage(const std::string& code)
    {
        for (size_t i = 0; i < directives.size(); i++)
        {
            if (directives[i].name == "error_page")
            {
                for (size_t j = 0; j < directives[i].values.size(); j++)
                {
                    if (code == directives[i].values[j])
                        return (directives[i].values.back());
                }
            }
        }
        return ("");
    }

    std::string getRoot()
    {
        Directive* dir;
        if ((dir = getDirective("root")))
            return (dir->getValues()[0]);
        return ("");
    }
    
    Location* findLocation(const std::string& uri)
    {
        std::vector<int> l;
        
        for (size_t i = 0; i < locations.size(); i++)
        {
            std::size_t pos;
            if (!(pos = uri.find(locations[i].path)))
                l.push_back(locations[i].path.size());
        }
        if (!l.size())
        {
            std::vector<int>::iterator it = std::max_element(l.begin(), l.end());
            return (&locations[*it]);
        }
        return (NULL);
    }
};

struct Config
{
    std::vector<Server> servers;

    Server findServerByHost(const std::string& host)
    {
        for (size_t i = 0; i < servers.size(); i++)
        {
            Directive* dir = servers[i].getDirective("host");
            if (!dir)
                continue;
            
            if (dir->getValues()[0] == host)
                return (servers[i]);
        }
        return (servers[0]);
    }
};

class ConfigParser
{
private:
    std::vector<std::string> tokenize(std::ifstream& input);
    Config parse(const std::vector<std::string>& tokens);
    void validate(const Config& config);

public:
    Config parseFile(const std::string& filename);
};