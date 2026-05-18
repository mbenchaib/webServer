#include "server.hpp"

int bring_client(std::vector<client> &clients, int find)
{
    for (size_t i = 0; i < clients.size(); i++)
    {
        if (clients[i].fd == find)
            return i;
    }
    return -1;
}

server::server()
{
    poll_fds.reserve(128);
    clients.reserve(128);
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    check_status(server_fd, 0, "socket");
    opt = 1;
    check_status(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)), 0, "setsockopt");
    add.sin_family = AF_INET;
    add.sin_port = htons(8080);
    add.sin_addr.s_addr = INADDR_ANY;
    check_status(bind(server_fd, reinterpret_cast<struct sockaddr *>(&add), sizeof(add)), 0, "bind");
    struct pollfd server_poll;
    server_poll.fd = server_fd;
    server_poll.events = POLLIN;
    poll_fds.push_back(server_poll);
    std::cout << "server init\n";
}
void    server::start_server(void)
{
    listen(server_fd, 10);
    while (1)
    {
        check_status(poll(&poll_fds[0], poll_fds.size(), -1), 0, "poll");
        for (size_t i = 0; i < poll_fds.size(); i++)
        {
            if (poll_fds[i].fd == server_fd && poll_fds[i].revents & POLLIN)
            {
                std::cout << "we have a new client\n";
                int new_client = accept(server_fd, NULL, NULL);
                check_status(new_client, 0, "accept");

                struct pollfd client_poll;
                client_poll.fd = new_client;
                client_poll.events = POLLIN;
                
                poll_fds.push_back(client_poll);
                clients.__emplace_back(new_client);
                break ;
            }else
            {
                int     client_index = bring_client(clients, poll_fds[i].fd);
                if (client_index == -1)
                    continue;
                client  &clian = clients[client_index];

                if (clian.state == client::CLOSE || poll_fds[i].revents & (POLLHUP | POLLERR))
                {
                    std::cout << "remove client " << clian.fd<< " cus he gone from list\n";
                    close(poll_fds[i].fd);
                    clients.erase(clients.begin() + (client_index));
                    poll_fds.erase(poll_fds.begin() + i);
                    i--;
                    continue ;
                }

                if (poll_fds[i].revents & POLLIN)
                    clian.read_request(poll_fds[i]);

                else if (poll_fds[i].revents & POLLOUT)
                    clian.send_response(poll_fds[i]);
            }
        }
        
    }
}

server::~server()
{
    close(server_fd);
    std::cout << "server close\n";
}
