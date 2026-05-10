/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mben-cha <mben-cha@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/29 20:40:02 by mben-cha          #+#    #+#             */
/*   Updated: 2026/05/10 16:51:38 by mben-cha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <fstream>
#include <vector>

struct Directive
{
    std::string name;
    std::vector<std::string> values;

    std::vector<std::string> getValues()
    {
        return (values);
    }
};

struct Location
{
    std::string path;
    std::vector<Directive> directives;

    Directive* getDirectives(const std::string& name)
    {
        for (size_t i = 0; i < directives.size(); i++)
        {
            if (directives[i].name == name)
                return (&(directives[i]));
        }
        return (NULL);
    }
};

struct Server
{
    std::vector<Directive> directives;
    std::vector<Location> locations;

    std::vector<Location> getLocations()
    {
        return (locations);
    }

    Directive* getDirectives(const std::string& name)
    {
        for (size_t i = 0; i < directives.size(); i++)
        {
            if (directives[i].name == name)
                return (&(directives[i]));
        }
        return (NULL);
    }
};

struct Config
{
    std::vector<Server> servers;
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