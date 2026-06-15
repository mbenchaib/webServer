#ifndef CGI_HPP
#define CGI_HPP

#include <vector>

class Client;      // forward declaration

class CGI
{
    public:
        int                         pipes[2];       //  pipes fd fhal minishell
        int                         pid;            // pid dyal child

        std::vector<std::string>    envs;           // hado envs dyal child
        std::vector<std::string>    args;           // hado ars dyal child

        int                         is_child_running;

        Client*                     client;

        CGI() : client(NULL), pid(-1), is_child_running(0)
        {
            pipes[0] = -1;
            pipes[1] = -1;
        }
        void setClient(Client *c)
        {
            client = c;
        }

        void run_cgi();
        void check_cgi();
        char **create_envs();
};

#endif