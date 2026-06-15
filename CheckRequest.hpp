#ifndef CHECKREQUEST_HPP
#define CHECKREQUEST_HPP

#include "ConfigParser.hpp"
#include <sys/stat.h>

class Client;

class CheckRequest
{
    private:
        Server      *server;
        Location    *location;
        std::string root;

        Client      *client;

        int     get_server(void);
        int     get_location(void);
        int     check_methods(void);
        int     check_max_body(void);
        int     check_root(void);
        void    cgi_or_static(void);
    public:
        CheckRequest();
        CheckRequest(Client& client);

        void    set_client(Client& clian);

        void    validate();
};

std::string create_301_302_response(std::string code, std::string path);

#endif