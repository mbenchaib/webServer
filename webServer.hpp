/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webServer.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mben-cha <mben-cha@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/26 23:06:52 by mben-cha          #+#    #+#             */
/*   Updated: 2026/05/04 20:09:11 by mben-cha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

class WebServer
{
private:
    // configuration
    Config      config;                 // loaded from file
    int         server_fd;              // the listening socket
    // socket operations
    void setupSocket();                  // create, bind, listen
    void acceptClient();                 // accept incoming connection
    std::string readRequest(int fd);           // read raw HTTP request
    void        sendResponse(int fd, HttpResponse); // send back
    
public:
    WebServer(std::string config_file);  // load configuration
    void run();                          // start the server loop
};
