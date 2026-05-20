/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webServer.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mben-cha <mben-cha@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/26 23:06:52 by mben-cha          #+#    #+#             */
/*   Updated: 2026/05/20 18:15:06 by mben-cha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <string>
#include <vector>
#include "ConfigParser.hpp"
#include <sys/event.h>
#include <map>

#define BACKLOG 128
#define MAX_EVENTS 64

class Client;

class WebServer
{
private:
    int                         kq;
    Config                      config;
    std::vector<int>            server_sd;
    struct kevent               events[MAX_EVENTS];
    std::map<int, Client>       clients;
    
    void setupSocket();
    void acceptClient(int, struct kevent);
    void registerSocketEvent(struct kevent&);
    //void HandleClient()
    
    
public:
    WebServer(std::string config_file);
    ~WebServer();
    void run();
};
