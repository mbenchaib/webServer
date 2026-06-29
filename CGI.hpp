#ifndef CGI_HPP
#define CGI_HPP

#include <vector>
#include <signal.h>

class Client;

void    clear_memory(char **arr);

typedef enum child_status
{
    NOT_RUNNING,
    RUNNING,
    FINISHED
}   child_status;

class CGI
{
    public:
        int             pipe_in[2];
        int             pipe_out[2];
        int             pid;

        char            **env;
        char            **arg;

        int             writing;
        int             reading;
        int             child_finished;

        size_t          data_send;

        bool            pipe_closed;
        std::string     cgi_buffer;
        child_status    status;
        Client*         client;

        CGI();
        CGI(const CGI& other);
        CGI& operator=(const CGI& other);
        ~CGI();

        void setClient(Client *c)
        {
            client = c;
        }

        int     run_cgi(void);
        void    check_cgi(void);
        
        void    create_envs(void);
        void    create_args(void);

        int     pipe_init(void);
        int     check_child(void);
        int     write_to_child(void);
        int     reading_from_child(void);
        int     checking_permission(void);

        void    starting_cgi(void);

        void    building_response(void);
};

#endif