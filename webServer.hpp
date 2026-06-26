/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webServer.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sael-kha <sael-kha@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/26 23:06:52 by mben-cha          #+#    #+#             */
/*   Updated: 2026/06/16 16:14:15 by sael-kha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <map>
#include <string>
#include <vector>
#include <poll.h>
#include "Client.hpp"
#include "ConfigParser.hpp"

#define BACKLOG 128
#define MAX_EVENTS 64

class Client;

class WebServer
{
private:
    int                         kq;
    Config                      config;
    std::vector<int>            server_sd;
    std::vector<struct pollfd>  pfds;
    std::map<int, Client>       clients;
    
    void setupSocket();
    void acceptClient(int);
    void addListenFds();
    void HandleClient(struct pollfd& fd);
    
    
public:
    WebServer(std::string config_file);
    ~WebServer();
    void run();
};
