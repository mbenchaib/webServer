/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webServer.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mben-cha <mben-cha@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/27 16:34:38 by mben-cha          #+#    #+#             */
/*   Updated: 2026/05/10 17:03:26 by mben-cha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "webServer.hpp"
#include <fstream>
#include "Exceptions.hpp"
#include "ConfigParser.hpp"

webServer::webServer(std::string config_file)
{
    std::ifstream file(config_file.c_str());
    if (!file.is_open())
    {
        throw 
    }
}

webServer::run()
{
    //setupSocket()
    while (true)
    {
        poll
        
        //acceptClient()
        //readRequest(int fd)
        //HttpParser here just check the syntax and structure of request message
        //HttpValidator take message from HttpParsor and validate it
        //HttpHandler take message from HttpValidator and handle it
        //sendResponse
    }
}