/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webServer.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sael-kha <sael-kha@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/27 16:34:38 by mben-cha          #+#    #+#             */
/*   Updated: 2026/06/29 10:37:14 by sael-kha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "webServer.hpp"
#include "Exceptions.hpp"
#include "Response.hpp"
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
#include <iostream>
#include <algorithm>

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
    while (true)
    {
        int fd_client = accept(serv_sd, NULL, NULL);

        if (fd_client == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;

            std::cerr << "accept failed on socket "
                      << serv_sd
                      << ": "
                      << strerror(errno)
                      << '\n';
            break;
        }

        if (fcntl(fd_client, F_SETFL, O_NONBLOCK) == -1)
        {
            close(fd_client);
            continue;
        }

        struct pollfd pfd;

        pfd.fd = fd_client;
        pfd.events = (POLLIN | POLLOUT);
        pfd.revents = 0;

        pfds.push_back(pfd);

        clients[fd_client] = Client(fd_client, config);
        
        clients[fd_client].cgi.setClient(&clients[fd_client]);
        clients[fd_client].checker.set_client(clients[fd_client]);
        clients[fd_client].time = time(NULL);
    }
}

int timeout_check(Client& client)
{
    if ((time(NULL) - client.time) > 200)
    {
        if (client.cgi.pid != -1)
            client.generate_error_response(504);
        else
            client.generate_error_response(408);
        client.status = WRITE;
        return 1;
    }
    return 0;
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
        Response(clian).build();
    }
    // hnaya cankhadem cgi ou canbuidy response
    if (clian.status == CGI_RUNNING && fd.revents & POLLOUT)
    {
        // std::cout << "builting CGI response now\n";
        clian.cgi.starting_cgi();
    }
    // hnaya cancoun salit men building response ou cansardo n client
    if ((clian.status == WRITE || timeout_check(clian)) && fd.revents & POLLOUT)
    {
        // clian.parsed_request.print();
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