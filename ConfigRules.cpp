/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigRules.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sael-kha <sael-kha@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/29 22:06:50 by mben-cha          #+#    #+#             */
/*   Updated: 2026/06/29 10:55:12 by sael-kha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigRules.hpp"
#include <ctype.h>
#include <cstddef>
#include <vector>
#include <map>
#include <cctype>
#include <cerrno>
#include <string>
#include <climits>


                            /* =========================================
                                            Helper Functions
                            ========================================= */

static bool isValidNumber(const std::string number, int min, int max)
{
    if (number.empty())
        return (false);
    
    char *end;
    errno = 0;
    long nb = strtol(number.c_str(), &end, 10);
    if (errno == ERANGE || *end != '\0' || nb < min || nb > max)
        return (false);

    return (true);
}

static bool isValidIPport(const std::vector<std::string> values)
{
    std::string value = values.back();
    
    for (size_t i = 0; i < value.size(); i++)
    {
        if (!isdigit(value[i]) && value[i] != '.' && value[i] != ':')
            return (false);
    }

    size_t pos = value.find(':');
    if (pos != std::string::npos)
    {
        size_t posD1 = value.find('.');
        if (posD1 == std::string::npos)
            return (false);
        
        size_t posD2 = value.find('.', posD1 + 1);
        if (posD2 == std::string::npos)
            return (false);

        size_t posD3 = value.find('.', posD2 + 1);
        if (posD3 == std::string::npos)
            return (false);

        std::string nb_s1 = value.substr(0, posD1);
        if (!isValidNumber(nb_s1, 0, 255))
            return (false);

        std::string nb_s2 = value.substr(posD1 + 1, posD2 - posD1 - 1);
        if (!isValidNumber(nb_s2, 0, 255))
            return (false);

        std::string nb_s3 = value.substr(posD2 + 1, posD3 - posD2 - 1);
        if (!isValidNumber(nb_s3, 0, 255))
            return (false);
        
        std::string nb_s4 = value.substr(posD3 + 1, pos - posD3 - 1);
        if (!isValidNumber(nb_s4, 0, 255))
            return (false);

        std::string port = value.substr(pos + 1);
        if (!isValidNumber(port, 1, 65535))
            return (false);
    }
    else
    {
        if (!isValidNumber(value, 1, 65535))
            return (false);
    }
    return (true);
}

static bool isValidHost(const std::vector<std::string> values)
{
    std::string value = values.back();

    for (size_t i = 0; i < value.size(); i++)
    {
        if (!isalnum(static_cast<unsigned char>(value[i])) && value[i] != '.' && value[i] != '-')
            return false;
    }
    return (true);
}


static bool isValidPath(const std::vector<std::string> values)
{
    std::string value = values.back();

    if (value[0] != '/' && value[0] != '.')
        return (false);

    for (size_t i = 1; i < value.size(); i++)
    {
        if (!isalnum(static_cast<unsigned char>(value[i])) && value[i] != '.' && value[i] != '-' && value[i] != '_' && value[i] != '/')
            return false;
    }
    
    return (true);
}

static bool isValidFilename(const std::vector<std::string> values)
{
    std::vector<std::string>::const_iterator iter = values.begin();
    std::vector<std::string>::const_iterator end = values.end();

    while (iter != end)
    {
        for (size_t i = 0; i < iter->size(); i++)
        {
            if (!isalnum(static_cast<unsigned char>((*iter)[i])) && (*iter)[i] != '.' && (*iter)[i] != '-' && (*iter)[i] != '_')
                return false;
        }
        iter++;
    }
    return (true);
}

static bool isValidSize(const std::vector<std::string> values)
{
    std::string value = values.back();
    
    size_t i;
    for (i = 0; i < value.size() && !std::isalpha(value[i]); i++);
    
    if (i == value.size())
    {
        std::string nb = value.substr(0);
        if (!isValidNumber(nb, 0, INT_MAX))
            return (false);
    }
    else
    {
        std::string nb = value.substr(0, i);
        if (!isValidNumber(nb, 0, INT_MAX))
            return (false);

        std::string unit = value.substr(i);
        if (unit.empty() || (unit != "M" && unit != "K" && unit != "G"))
            return (false);
    }
    return (true);
}


static bool isValidErrorPage(const std::vector<std::string> values)
{
    size_t i;
    for (i = 0; i < values.size() - 1; i++)
    {
        if (!isValidNumber(values[i], 100, 599))
            return (false);
    }
    
    if (!isValidPath(values))
        return (false);

    return (true);
}

static bool isValidMethod(const std::vector<std::string> values)
{
    for (size_t i = 0; i < values.size(); i++)
    {
        if (values[i] != "GET" && values[i] != "POST" && values[i] != "DELETE")
            return (false);
    }
    return (true);
}

static bool isValidOnOf(const std::vector<std::string> values)
{
    std::string value  = values.back();

    if (value != "on" && value != "off")
        return (false);
    return (true);
}

static bool isValidCgi(const std::vector<std::string> values)
{
    if (values[0][0] != '.')
        return (false);

    if (!isValidPath(values))
        return (false);
    
    return (true);
}

static bool isValidReturn(const std::vector<std::string> values)
{
    if (values.size() == 1)
    {
        if (isValidNumber(values[0], 100, 599))
            return (true);
    }
    else
    {
        if (!isValidNumber(values[0], 100, 599))
            return (false);

        if (isValidPath(values))
            return (true);
        else if (values[1].find("http://") == 0 || values[1].find("https://") == 0)
        {
            for (size_t i = 0; i < values[1].size(); i++)
            {
                if (!isalnum(static_cast<unsigned char>(values[1][i]))
                    && values[1][i] != '.'
                    && values[1][i] != '/'
                    && values[1][i] != ':'
                    && values[1][i] != '?'
                    && values[1][i] != '&'
                    && values[1][i] != '='
                    && values[1][i] != '-'
                    && values[1][i] != '_')
                {
                    return false;
                }
            }
            return (true);
        }
    }
    return (false);
}

                            /* ===== Validation Dispatcher ===== */
                    
bool validateValueType(ValueType type, const std::vector<std::string> values)
{
    switch(type)
    {
        case TYPE_METHOD:
            return (isValidMethod(values));
        case TYPE_PATH:
            return (isValidPath(values));
        case TYPE_FILENAME:
            return (isValidFilename(values));
        case TYPE_HOST:
            return (isValidHost(values));
        case TYPE_IP_PORT:
            return (isValidIPport(values));
        case TYPE_ON_OFF:
            return (isValidOnOf(values));
        case TYPE_SIZE:
            return (isValidSize(values));
        case TYPE_ERROR_PAGE:
            return (isValidErrorPage(values));
        case TYPE_CGI:
            return (isValidCgi(values));
        case TYPE_RETURN:
            return (isValidReturn(values));
        default:
            return (false);
    }
}

                            /* ===== Directive Rules ===== */
                            
void initRules(std::map<std::string, DirectiveRule>& serverRules, std::map<std::string, DirectiveRule>& locationRules)
{
    serverRules["listen"]        =   DirectiveRule(1, 1, TYPE_IP_PORT, false, true);
    serverRules["host"]          =   DirectiveRule(1, 1, TYPE_HOST, false, false);
    serverRules["root"]          =   DirectiveRule(1, 1, TYPE_PATH, false, false);
    serverRules["index"]         =   DirectiveRule(1, UNSPECIFIED_MAX_VALUES, TYPE_FILENAME, false, false);
    serverRules["max_body"]      =   DirectiveRule(1, 1, TYPE_SIZE, false, false);
    serverRules["error_page"]    =   DirectiveRule(2, UNSPECIFIED_MAX_VALUES, TYPE_ERROR_PAGE, true, false);
    
    locationRules["methods"]     =   DirectiveRule(1, 3, TYPE_METHOD, false, false);
    locationRules["root"]        =   DirectiveRule(1, 1, TYPE_PATH, false, false);
    locationRules["index"]       =   DirectiveRule(1, UNSPECIFIED_MAX_VALUES, TYPE_FILENAME, false, false);
    locationRules["autoindex"]   =   DirectiveRule(1, 1, TYPE_ON_OFF, false, false);
    locationRules["max_body"]    =   DirectiveRule(1, 1, TYPE_SIZE, false, false);
    locationRules["cgi"]         =   DirectiveRule(2, 2, TYPE_CGI, true, false);
    locationRules["upload_path"] =   DirectiveRule(1, 1, TYPE_PATH, false, false); 
    locationRules["return"]      =   DirectiveRule(1, 2, TYPE_RETURN, false, false);
}