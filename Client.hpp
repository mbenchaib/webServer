#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <vector>
#include <poll.h>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "Request.hpp"
#include "ConfigParser.hpp"
#include <fstream>
#include <sstream>
#include "CheckRequest.hpp"
#include "CGI.hpp"

typedef enum hala
{
    READ,
    VALIDATION,
    CGI_RUNNING,
    STATIC,
    WRITE,
    CLOSE
}   hala;

// class dyal client->
// client 3andi ki9da ou kisared 

class Client
{
    public:
        Config*         config;         // config dyal server
        CGI             cgi;            // cgi class fih nrany cgi

        int             fd;             // socket_fd dyal client from accept
        hala            status;         // status wach client ki9ra daba awla kisared respone awla CLOSE "sf sala"
        
        std::string     raw_buffer;     // hna fin kanjma3 request from fd
        std::string     response;       // hada fih wahed static response just for test
        
        int             read_body;      // bhadi kanhseb chehal 9rit f body from client fd
        Request         parsed_request; // hada request ba3d ma tparsa
        unsigned long   bytes_send_to_client;   // bhadi cantba3 chechal sardt n client men response

        CheckRequest    checker;

        //difault constractor just for test 
        Client(void);
        Client(int fd, Config& config);
        // hna can9ra request ou kanparsih
        void    reading_request(void);
        // had can sared response n client
        void    sending_response(void);

        int     check_recv_error(int bytes);
        void    generate_error_response(int code);
};

std::string get_http_msg(int code);

#endif
