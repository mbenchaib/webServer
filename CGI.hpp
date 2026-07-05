#ifndef CGI_HPP
#define CGI_HPP

#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
#include <fcntl.h>
#include <sstream>
#include <vector>
#include <string>

class Client;

void clear_memory(char **arr);

typedef enum child_status
{
    NOT_RUNNING,
    RUNNING,
    FINISHED
} child_status;

class CGI
{
    public:
        int             file_in;
        int             file_out;
        int             pid;

        std::string     dir;
        std::string     script;

        char            **env;
        char            **arg;

        int             writing;
        int             reading;
        int             child_finished;

        size_t          data_send;

        bool            pipe_closed;
        std::string     cgi_buffer;
        child_status    status;
        Client* client;

        std::string     in_path;
        std::string     out_path;

        CGI();
        CGI(const CGI& other);
        CGI& operator=(const CGI& other);
        ~CGI();

        void setClient(Client *c);

        int     run_cgi_process(void);
        void    check_cgi(struct pollfd& p);
        
        void    create_envs(void);
        void    create_args(void);

        int     file_init(void);
        int     check_child(void);
        int     write_to_child(void);
        int     reading_from_child(void);
        int     checking_permission(void);

        void    starting_cgi(std::vector<struct pollfd>& pfds, struct pollfd& p);
        void    building_response(void);
};

#endif