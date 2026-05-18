#ifndef    SERVER_HPP
#define     SERVER_HPP

#include "client.hpp"

int bring_client(std::vector<client> &clients, int find);

class server
{
    private:
        int                         server_fd;
        std::vector<struct pollfd>  poll_fds;
        std::vector<client>         clients;
        int                         opt;
        struct sockaddr_in          add;
    public:
        server();
        void    start_server(void);
        ~server();
};
#endif