/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mben-cha <mben-cha@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/29 20:40:02 by mben-cha          #+#    #+#             */
/*   Updated: 2026/06/06 17:13:16 by mben-cha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <cstddef>
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
};

struct Server
{
    std::vector<Directive> directives;
    std::vector<Location> locations;

    std::vector<Location> getLocations()
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