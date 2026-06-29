/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigRules.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mben-cha <mben-cha@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/29 22:06:41 by mben-cha          #+#    #+#             */
/*   Updated: 2026/06/29 21:44:29 by mben-cha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIGRULES_HPP
#define CONFIGRULES_HPP

#include <map>
#include <string>
#include <vector>
#define UNSPECIFIED_MAX_VALUES 10

enum ValueType
{
    TYPE_UNKNOWN,
    TYPE_METHOD,
    TYPE_PATH,
    TYPE_FILENAME,
    TYPE_HOST,
    TYPE_IP_PORT,
    TYPE_ON_OFF,
    TYPE_SIZE,
    TYPE_ERROR_PAGE,
    TYPE_CGI,
    TYPE_RETURN
};

struct DirectiveRule
{
    int         minValues;
    int         maxValues;
    ValueType   type;
    bool        allowDuplicate;
    bool        required;

    DirectiveRule() : minValues(0), maxValues(0), type(TYPE_UNKNOWN), allowDuplicate(false), required(false) {}
    DirectiveRule(int x, int y, ValueType ty, bool ad, bool re) : minValues(x), maxValues(y), type(ty), allowDuplicate(ad), required(re) {}
};

void initRules(std::map<std::string, DirectiveRule>& serverRules, std::map<std::string, DirectiveRule>& locationRules);
bool validateValueType(ValueType type, const std::vector<std::string> values);

#endif