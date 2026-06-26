/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webServer.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sael-kha <sael-kha@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/27 16:34:38 by mben-cha          #+#    #+#             */
/*   Updated: 2026/06/26 13:30:29 by sael-kha         ###   ########.fr       */
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
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
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

        //--------
        int opt = 1;
        if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
            throw SocketSetupError(strerror(errno));

        if (bind(sd, res->ai_addr, res->ai_addrlen) == -1)
            throw SocketSetupError(strerror(errno));

        if (listen(sd, BACKLOG) == -1)
            throw SocketSetupError(strerror(errno));
        
        server_sd.push_back(sd);
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
        throw SocketSetupError(strerror(errno));
    
    struct pollfd   pfd;
    
    pfd.fd = fd_client;
    pfd.events = (POLLIN | POLLOUT);
    pfds.push_back(pfd);

    clients.insert(std::make_pair(pfd.fd, Client(pfd.fd, config)));
    clients[pfd.fd].cgi.setClient(&clients[pfd.fd]);
    clients[pfd.fd].checker.set_client(clients[pfd.fd]);
}

void    WebServer::HandleClient(struct pollfd& fd)
{
    Client& clian = clients[fd.fd];
    // hna can9ra men client request ou nparsih
    if (clian.status == READ && fd.revents & POLLIN)
    {
        std::cout << "server read now\n";
        clian.reading_request();
    }
    // hnakanvalidi wach request huwahadak awla
    if (clian.status == VALIDATION)
    {
        std::cout << "validating client request\n";
        clian.checker.validate();
    }
    // hnaya rashid ybuidy static response dyalo
    if (clian.status == STATIC && fd.revents & POLLOUT)
    {
        std::cout << "builting STATIC response now\n";
        clian.status = WRITE;
        // clian.sending_response();
    }
    // hnaya cankhadem cgi ou canbuidy response
    if (clian.status == CGI_RUNNING && fd.revents & POLLOUT)
    {
        // std::cout << "builting CGI response now\n";
        clian.cgi.starting_cgi();
    }
    // hnaya cancoun salit men building response ou cansardo n client
    if (clian.status == WRITE && fd.revents & POLLOUT)
    {
        clian.parsed_request.print();
        clian.sending_response();
    }
    // hna mli kansali client canmsho
    if (clian.status == CLOSE)
    {
        std::cout << "server close client\n";

        close(fd.fd);

        clients.erase(fd.fd);

        for (std::vector<pollfd>::iterator it = pfds.begin(); it != pfds.end(); ++it)
        {
            if (it->fd == fd.fd)
            {
                pfds.erase(it);
                break;
            }
        }
        return;
    }
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
            
            
            bool isListening = std::find(server_sd.begin(), server_sd.end(), pfds[i].fd) != server_sd.end();
            
            if (pfds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
            {
                std::cout << "client with "<<pfds[i].fd << " gone\n";
                std::cout << strerror(errno) << '\n';
                int fd = pfds[i].fd;
                
                close(fd);
                pfds.erase(pfds.begin() + i);
                if (!isListening)
                    clients.erase(fd);
                else
                {
                    std::vector<int>::iterator it = std::find(server_sd.begin(), server_sd.end(), fd); 
                    server_sd.erase(it);

                    if (server_sd.empty())
                        throw NoListenSocketException("All listening sockets have failed; server shutting down");
                }
                i--;
                continue ;
            }
            if (isListening && (pfds[i].revents & POLLIN))
                acceptClient(pfds[i].fd);
            else if (!isListening)
                HandleClient(pfds[i]);
        }
    }
}