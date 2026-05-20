/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webServer.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mben-cha <mben-cha@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/27 16:34:38 by mben-cha          #+#    #+#             */
/*   Updated: 2026/05/20 18:31:57 by mben-cha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "webServer.hpp"
#include "Exceptions.hpp"
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/event.h>
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

    if ((kq = kqueue()) == -1)
        throw SocketSetupError(strerror(errno));
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
    
    for (size_t i = 0; i < config.servers.size(); i++)
    {
        std::string ip_port = config.servers[i].getDirective("listen")->getValues()[0];
        size_t pos = ip_port.find(':');
        std::string ip = ip_port.substr(0, pos);
        std::string port = ip_port.substr(pos + 1);
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

        server_sd.push_back(sd);

        if (bind(sd, res->ai_addr, res->ai_addrlen) == -1)
            throw SocketSetupError(strerror(errno));

        if (listen(sd, BACKLOG) == -1)
            throw SocketSetupError(strerror(errno));
    }
}

// ===== Register listening sockets with kqueue for read events =====

void WebServer::registerSocketEvent(struct kevent& ev)
{
    for (size_t i = 0; i < server_sd.size(); i++)
    {
        EV_SET(&ev, server_sd[i], EVFILT_READ, EV_ADD, 0, 0, NULL);
        if (kevent(kq, &ev, 1, NULL, 0, NULL) == -1)
        {
            std::cerr << "Failed to register socket "
                      << server_sd[i]
                      << ": "
                      << strerror(errno)
                      << "\n";
            close(server_sd[i]);
            server_sd.erase(server_sd.begin() + i);
            i--;
        }
    }
}

// ===== Accept a new client and register it for event monitoring =====

void acceptClient(int serv_sd, struct kevent& ev)
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
    
    EV_SET(&ev, fd_client, EVFILT_READ, EV_ADD, 0, 0, NULL);
    if (kevent(fd_client, &ev, 1, NULL, 0, NULL) == -1)
    {
        std::cerr << "Failed to register socket "
                    << fd_client
                    << ": "
                    << strerror(errno)
                    << "\n";
        close(fd_client);
        return ;
    }
    //clients[fd_client] = Client();    
}

// ===== Initialize the server and process socket events in the main kqueue loop =====

void WebServer::run()
{
    struct kevent ev;

    setupSocket();
    
    registerSocketEvent(ev);

    if (server_sd.empty())
        throw NoListenSocketException("Server startup failed: no listening sockets available");
    
    while (true)
    {
        int n = kevent(kq, NULL, 0, events, MAX_EVENTS, NULL);
        if (n == -1)
        {
            if (errno == EINTR)
                continue ;

            throw std::runtime_error(std::string("kevent failed in event loop: ") + strerror(errno));
        }

        for (int i = 0; i < n; i++)
        {
            if (events[i].flags & EV_ERROR)
            {
                std::cerr << "kevent event error on fd "
                          << events[i].ident
                          << ": "
                          << strerror(static_cast<int>(events[i].data))
                          << "\n";
                close(events[i].ident);
                clients.erase(events[i].ident);
                continue;
            }
            
            std::vector<int>::iterator it = std::find(server_sd.begin(), server_sd.end(), events[i].ident);
            if (it != server_sd.end())
                acceptClient(events[i].ident, ev);
            else
                //HandleClient()
        }
    }
}