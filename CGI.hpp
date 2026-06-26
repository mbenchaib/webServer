#ifndef CGI_HPP
#define CGI_HPP

#include <vector>

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

        CGI() : client(NULL), pid(-1), status(NOT_RUNNING), env(NULL),
            arg(NULL), pipe_closed(false), writing(0), reading(0), data_send(0), child_finished(0)
        {
            pipe_in[0] = -1;
            pipe_in[1] = -1;
            pipe_out[0] = -1;
            pipe_out[1] = -1;
        }
        ~CGI()
        {
            std::cout << "cleaning cgi\n";
            if (pipe_in[0] != -1) close(pipe_in[0]);
            if (pipe_in[1] != -1) close(pipe_in[1]);
            if (pipe_out[0] != -1) close(pipe_out[0]);
            if (pipe_out[1] != -1) close(pipe_out[1]);
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
        int     check_child(void);
        int     write_to_child(void);
        int     reading_from_child(void);
        int     checking_permission(void);

        void    starting_cgi(void);

        void    building_response(void);
};

#endif