#ifndef     CLIENT_HPP
#define     CLIENT_HPP

#include <vector>
#include <poll.h>
#include <fcntl.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

void    check_status(int status, int throw_or_not, std::string msg);

class client
{
    public:
        typedef enum status
        {
            READ,
            WRITE,
            CLOSE,
        }   status;
        
        int             fd;
        status          state;
        int             file_fd;

        std::string     request;
        std::string     response;
        HttpRequest     parcing_req;

        int             is_header_full;
        int             is_body_full;
        long            content_length;
        std::string     body;
        
        int             bytes_send_client;

        void    built_request(void);
        void    read_body(struct pollfd &fds);
        void    read_request(struct pollfd &fds);
        void    send_response(struct pollfd &fds);
        client(int  client_fd) : body(""), content_length(0), is_body_full(0), is_header_full(0), fd(client_fd), state(READ), bytes_send_client(0), parcing_req("") {};
        ~client(void);
};

#endif