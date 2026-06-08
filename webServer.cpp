/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webServer.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mben-cha <mben-cha@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/27 16:34:38 by mben-cha          #+#    #+#             */
/*   Updated: 2026/06/08 16:42:21 by mben-cha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "webServer.hpp"
#include "Exceptions.hpp"
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string.h>
#include <string>
#include <sys/fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>


                            // =======================
                            //         Helpers
                            // =======================

static void cleanupSockets(std::vector<int>& server_sd)
{
    for (size_t i = 0; i < server_sd.size(); i++)
        close(server_sd[i]);
}

static bool isListenAddressUsed(const std::string& ip_port, const std::vector<std::string>& listenAddresses)
{
    for (size_t i = 0; i < listenAddresses.size() - 1; i++)
    {
        if (ip_port == listenAddresses[i])
            return (true);
    }
    return (false);
}

// ===== Constructor / Destructor =====

WebServer::WebServer(std::string config_file)
{
    ConfigParser cp;
    
    config = cp.parseFile(config_file);
}

WebServer::~WebServer()
{
    cleanupSockets(server_sd);
}

// ===== Set up server sockets: resolve address, create, bind, and listen =====

void WebServer::setupSocket()
{
    int                         status;
    int                         sd;
    struct addrinfo             hints, *res;
    std::vector<std::string>    listenAddresses;
    std::string                 ip;
    std::string                 port;
    
    for (size_t i = 0; i < config.servers.size(); i++)
    {
        std::string ip_port = config.servers[i].getDirective("listen")->getValues()[0];
        size_t pos = ip_port.find(':');
        if (pos == std::string::npos)
        {
            ip = "0.0.0.0";
            port = ip_port;
        }
        else
        {
            ip = ip_port.substr(0, pos);
            port = ip_port.substr(pos + 1);
        }
        listenAddresses.push_back(ip_port);

        if (isListenAddressUsed(ip_port, listenAddresses))
            continue ;
        
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        
        if ((status = getaddrinfo(ip.c_str(), port.c_str(), &hints, &res)) == -1)
            throw SocketSetupError(gai_strerror(status));

        if ((sd = socket(res->ai_family, res->ai_socktype, 0)) == -1)
            throw SocketSetupError(strerror(errno));

        int yes = 1;
        if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
            throw SocketSetupError(strerror(errno));

        if (fcntl(sd, F_SETFL, O_NONBLOCK) == -1)
            throw SocketSetupError(strerror(errno));

        server_sd.push_back(sd);

        if (bind(sd, res->ai_addr, res->ai_addrlen) == -1)
            throw SocketSetupError(strerror(errno));

        if (listen(sd, BACKLOG) == -1)
            throw SocketSetupError(strerror(errno));
    }
}

// ===== Add listening sockets to the poll file descriptor list =====

void WebServer::addListenFds()
{
    for (size_t i = 0; i < server_sd.size(); i++)
    {
        struct pollfd   pfd;
        
        pfd.fd = server_sd[i];
        pfd.events = POLLIN;
        pfds.push_back(pfd);
    }
}

// ===== Accept a new client and add it to the poll list =====

void WebServer::acceptClient(int serv_sd)
{
    int fd_client;
    if ((fd_client = accept(serv_sd, NULL, NULL)) == -1)
    {
        std::cerr << "accept failed on socket "
                  << serv_sd
                  << ": "
                  << strerror(errno)
                  << "\n";
        return ;
    }
    
    if (fcntl(fd_client, F_SETFL, O_NONBLOCK) == -1)
    {
        close(fd_client);
        return;
    }
    
    struct pollfd   pfd;
    
    pfd.fd = fd_client;
    pfd.events = POLLIN;
    pfds.push_back(pfd);
    
    //clients[fd_client] = Client();
}

// ===== Initialize server and process socket events using poll() =====

void WebServer::run()
{
    setupSocket();
    
    if (server_sd.empty())
        throw NoListenSocketException("Server startup failed: no listening sockets available");
    
    addListenFds();
    
    while (true)
    {
        int n = poll(pfds.data(), pfds.size(), -1);
        if (n == -1)
        {
            if (errno == EINTR)
                continue;

            throw std::runtime_error(std::string("poll failed in event loop: ") + strerror(errno));
        }

        for (size_t i = 0; i < pfds.size(); i++)
        {
            if (pfds[i].revents == 0)
                continue;
            
            if (pfds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
            {
                close(pfds[i].fd);
                pfds.erase(pfds.begin() + i);
                i--;
                continue;
            }
            
            bool isListening = std::find(server_sd.begin(), server_sd.end(), pfds[i].fd) != server_sd.end();

            if (isListening && (pfds[i].revents & POLLIN))
                acceptClient(pfds[i].fd);
            else if (!isListening)
                //HandleClient()
        }
    }
}