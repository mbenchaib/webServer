#ifndef CGI_HPP
#define CGI_HPP

#include <vector>

class Client;

void    clear_memory(char **arr);

class CGI
{
    public:
        int                         pipes[2];
        int                         pid;

        char                        **env;
        char                        **arg;

        int                         is_child_running;
        int                         is_child_finished;
        Client*                     client;

        CGI() : client(NULL), pid(-1), is_child_running(0), env(NULL), arg(NULL)
        {
            pipes[0] = -1;
            pipes[1] = -1;
        }
        ~CGI()
        {
            if (pipes[0] != -1) close(pipes[0]);
            if (pipes[1] != -1) close(pipes[1]);
            clear_memory(env);
            clear_memory(arg);
        }
        void setClient(Client *c)
        {
            client = c;
        }

        int     run_cgi(void);
        void    check_cgi(void);
        
        void    create_envs(void);
        void    create_args(void);

        int     pipe_init(void);
        int     checking_permission(void);
        
        void    starting_cgi(void);
};

#endif