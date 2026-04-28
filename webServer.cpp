/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webServer.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mben-cha <mben-cha@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/27 16:34:38 by mben-cha          #+#    #+#             */
/*   Updated: 2026/04/28 17:23:12 by mben-cha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "webServer.hpp"

webServer::webServer(std::string config_file)
{
    //Handle config file
    //Check if exist if valid...
    //Should be in specific format i think
}

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