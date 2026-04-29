/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webServer.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: roubelka <roubelka@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/27 16:34:38 by mben-cha          #+#    #+#             */
/*   Updated: 2026/04/29 17:54:37 by roubelka         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "webServer.hpp"
#include <fstream>

webServer::webServer(std::string config_file)
{
    std::ifstream file(config_file.c_str());
    if (!file.is_open())
    {
        throw 
    }
}
///rachid
webServer::run()
{
    //setupSocket()
    while (true)
    {
        //acceptClient()
        //readRequest(int fd)
        //HttpParser here just check the syntax and structure of request message
        //HttpValidator take message from HttpParsor and validate it
        //HttpHandler take message from HttpValidator and handle it
        //sendResponse
    }
}