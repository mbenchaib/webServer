#ifndef CHECKREQUEST_HPP
#define CHECKREQUEST_HPP

#include "ConfigParser.hpp"
#include <sys/stat.h>

class Client;

class CheckRequest
{
    private:
        int     get_server(void); //get server block and regester it
        int     get_location(void); // get location block by the longest prefix if there is no location there well be NULL and u have to fall back to server block
        int     check_methods(void); // cheching method for request in location first and second server if there is no location or method not setup
        int     check_max_body(void); // checking body leght from request by the server or location max_body
        int     check_root(void); //
        void    cgi_or_static(void);
    public:
        Server      *server;
        Location    *location;

        Client      *client;
        bool        is_a_dir;

        std::string root;
        std::string compailer;

        CheckRequest();
        CheckRequest(const CheckRequest& other);
        CheckRequest(Client& client);
        CheckRequest& operator=(const CheckRequest& other);
        ~CheckRequest();

        void    set_client(Client& clian);

        void    validate();
};

std::string create_301_302_response(std::string code, std::string path);

#endif