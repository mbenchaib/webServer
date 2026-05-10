/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mben-cha <mben-cha@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/30 12:08:04 by mben-cha          #+#    #+#             */
/*   Updated: 2026/05/10 16:56:07 by mben-cha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"
#include "Exceptions.hpp"
#include <cctype>
#include <cstddef>
#include <cstring>
#include <vector>
#include <string>
#include <map>
#include <cstdlib>
#include "ConfigRules.hpp"
#include <iostream>

                            // =======================
                            //         Helper
                            // =======================
                            
static bool hasDuplicate(std::vector<Location>::const_iterator iter, std::vector<Location>::const_iterator& end)
{
    std::vector<Location>::const_iterator it =  iter;
    
    while (++iter != end)
    {
        if (it->path == iter->path)
            return (true);
    }
    return (false);
}

static bool hasDuplicate(std::vector<Directive>::const_iterator iter, std::vector<Directive>::const_iterator& end, bool allow)
{
    std::vector<Directive>::const_iterator it =  iter;
    
    while (++iter != end)
    {
        if (it->name == iter->name && !allow)
            return (true);
    }
    return (false);
}

                            /* =========================================
                                        Tokenization Stage
                            ========================================= */

std::vector<std::string> ConfigParser::tokenize(std::ifstream& input)
{
    std::string                 line;
    std::string                 content;
    std::vector<std::string>    tokens;
    size_t                      i = 0;
    size_t                      j = 0;
    
    while (std::getline(input, line))
        content = content + line;
    
    if (content.empty())
        throw EmptyConfigException();
    
    while (i < content.size())
    {
        while(content[i] == ' ' || content[i] == '\t')
            i++;
        if (i == content.size())
            break ;
        if (content[i] == '\0')
            throw ConfigSyntaxError("Error: null byte detected");
        if (content[i] != '{' && content[i] != '}' && content[i] != ';')
        {
            j = i;
            while (j < content.size() && content[j] != ' ' && content[j] != '{' && content[j] != '}' && content[j] != ';')
                j++;
            tokens.push_back(content.substr(i, j - i));
            i = j;
        }
        else
        {
            tokens.push_back(std::string(1, content[i]));
            i++;
        }
    }
    return (tokens);
}

                            /* =========================================
                                        Parsing Helpers
                            ========================================= */
                            
static void expect(const char *expect, std::vector<std::string>::const_iterator& index, std::vector<std::string>::const_iterator end)
{
    if (index == end || *index != expect)
        throw ConfigSyntaxError("Error: configuration syntax error");
    index++;
}

static void parseDirective(std::vector<std::string>::const_iterator& index, std::vector<std::string>::const_iterator end, std::vector<Directive>& directives)
{
    if (index != end && (*index == "{" || *index == "}" || *index == ";"))
        throw ConfigSyntaxError("Error: configuration syntax error");
    
    Directive dir;
    dir.name = *index;
    directives.push_back(dir);
    
    index++;
    while (index != end && *index != ";")
    {
        if (index != end && (*index == "{" || *index == "}" || *index == ";"))
            throw ConfigSyntaxError("Error: configuration syntax error");
        directives.back().values.push_back(*index);
        index++;
    }
    expect(";", index, end);
}
static void parseLocation(std::vector<std::string>::const_iterator& index, std::vector<std::string>::const_iterator end, Config& conf)
{
    expect("location", index, end);

    if (index == end)
        throw ConfigSyntaxError("Error: configuration syntax error");

    Location loc;
    loc.path = *index;
    conf.servers.back().locations.push_back(loc);
    
    if (index == end || (*index)[0] != '/')
        throw ConfigSyntaxError("Error: configuration syntax error.");
    index++;

    expect("{", index, end);

    while (index != end && *index != "}")
        parseDirective(index, end, conf.servers.back().locations.back().directives);

    expect("}", index, end);
}


static void parseServer(std::vector<std::string>::const_iterator& index, std::vector<std::string>::const_iterator& end, Config& conf)
{
    expect("server", index, end);

    Server se;
    conf.servers.push_back(se);

    expect("{", index, end);

    while (index != end && *index != "}")
    {
        if (*index == "location")
            parseLocation(index, end, conf);
        else
            parseDirective(index, end, conf.servers.back().directives);
    }

    expect("}", index, end);
}


                            /* =========================================
                                        Parsing Stage
                            ========================================= */

Config ConfigParser::parse(const std::vector<std::string>& tokens)
{
    Config  conf;

    std::vector<std::string>::const_iterator index = tokens.begin();
    std::vector<std::string>::const_iterator end = tokens.end();
    while (index != end)
        parseServer(index, end, conf);
    return (conf);
}

                            /* =========================================
                                        Validation Helpers
                            ========================================= */

static void validateDirectives(const std::vector<Directive>& directives, std::map<std::string, DirectiveRule>& DirRules)
{
    if (directives.empty())
        return ;

    std::vector<Directive>::const_iterator iter = directives.begin();
    std::vector<Directive>::const_iterator end = directives.end();

    std::map<std::string, DirectiveRule>::const_iterator iter_map;

    while (iter != end)
    {
        iter_map = DirRules.lower_bound(iter->name);
        if (!(iter_map != DirRules.end() && iter->values.size() >= iter_map->second.minValues
                                     && iter->values.size() <= iter_map->second.maxValues))
            throw ConfigValidationError("Error: Unknown directive or invalid number of directive values");

        if (hasDuplicate(iter, end, iter_map->second.allowDuplicate))
            throw ConfigValidationError("Error: duplicate directive");

        if (!validateValueType(iter_map->second.type, iter->values))
            throw ConfigValidationError("Error: Invalid directive value");
        iter++;     
    }
}

static void validateLocations(const std::vector<Location>& locations, std::map<std::string, DirectiveRule>& locationRules)
{
    if (locations.empty())
        return ;

    std::vector<Location>::const_iterator iter = locations.begin();
    std::vector<Location>::const_iterator end = locations.end();

    while (iter != end)
    {
        if (iter->path[0] != '/')
            throw ConfigValidationError("Error: Invalid directive value");

        if (hasDuplicate(iter, end))
            throw ConfigValidationError("Error: duplicate location");

        for (size_t i = 1; i < iter->path.size(); i++)
        {
            if (!isalnum(static_cast<unsigned char>(iter->path[i])) && iter->path[i] != '.' && iter->path[i] != '-' && iter->path[i] != '_' && iter->path[i] != '/')
                throw ConfigValidationError("Error: Invalid directive value");
        }

        validateDirectives(iter->directives, locationRules);
        iter++;
    }
}

                            /* =========================================
                                    Validation Stage (Semantic)
                            ========================================= */

void ConfigParser::validate(const Config& config)
{
    if (config.servers.empty())
        throw ConfigValidationError("Error: No server configuration found");

    std::vector<Server>::const_iterator iter = config.servers.begin();
    std::vector<Server>::const_iterator end = config.servers.end();

    std::map<std::string, DirectiveRule>    serverRules;
    std::map<std::string, DirectiveRule>    locationRules;

    initRules(serverRules, locationRules);
    
    while (iter != end)
    {
        if ((*iter).directives.empty() && (*iter).locations.empty())
            throw ConfigValidationError("Error: server block must contain at least one directive or location");

        validateDirectives((*iter).directives, serverRules);
        validateLocations((*iter).locations, locationRules);
        iter++;
    }
}

/*
** Main entry point for configuration parsing.
** Builds and returns a validated Config object from file.
*/

Config ConfigParser::parseFile(const std::string& filename)
{
    std::ifstream file(filename.c_str());
    if (!file.is_open())
        throw FileOpenException();

    std::vector<std::string> tokens = tokenize(file);
    
    Config conf = parse(tokens);
    
    validate(conf);

    return (conf);
}
