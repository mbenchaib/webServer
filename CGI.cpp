#include "Client.hpp"
#include "CGI.hpp"

void    clear_memory(char **arr)
{
    if (!arr)
        return ;
    
    int i = 0;
    while(arr[i])
    {
        delete[] arr[i];
        i++;
    }
}


void CGI::create_envs()
{
    std::vector<std::string> envs;

    envs.push_back("GATEWAY_INTERFACE=CGI/1.1");
    envs.push_back("SERVER_PROTOCOL=HTTP/1.1");
    envs.push_back("SERVER_SOFTWARE=webserv/1.0");
    envs.push_back("SERVER_NAME=" + client->parsed_request.host);
    envs.push_back("REQUEST_METHOD=" + client->parsed_request.method);
    envs.push_back("QUERY_STRING=" + client->parsed_request.query_string);
    envs.push_back("SCRIPT_NAME=" + client->parsed_request.path);
    envs.push_back("SCRIPT_FILENAME=" + client->checker.root);
    envs.push_back("PATH_INFO=" + client->parsed_request.path);
    envs.push_back("PATH_TRANSLATED=" + client->checker.root);

    if (client->parsed_request.method == "POST")
    {
        std::ostringstream len_stream;
        len_stream << client->parsed_request.body.size();
        envs.push_back("CONTENT_LENGTH=" + len_stream.str());
        envs.push_back("CONTENT_TYPE=" + client->parsed_request.content_type);
    }

    for (std::map<std::string, std::string>::iterator it = client->parsed_request.headers.begin(); 
         it != client->parsed_request.headers.end(); ++it)
    {
        std::string header_name = "HTTP_";
        for (size_t i = 0; i < it->first.length(); ++i)
        {
            if (it->first[i] == '-')
                header_name += '_';
            else
                header_name += toupper(it->first[i]);
        }
        envs.push_back(header_name + "=" + it->second);
    }

    env = new char*[envs.size() + 1];
    for (size_t i = 0; i < envs.size(); ++i)
    {
        env[i] = new char[envs[i].length() + 1];
        std::strcpy(env[i], envs[i].c_str());
    }
    env[envs.size()] = NULL;
}

void CGI::check_cgi()
{
    if (is_child_running == 0)
        return ;
    int wstatus;
    pid_t ret = waitpid(pid, &wstatus, WNOHANG);

    if (ret == 0)
        return;

    if (ret == pid)
    {
        client->response.clear();

        char buffer[1000];
        int bytes = read(pipes[0], buffer, sizeof(buffer));

        while (bytes > 0)
        {
            client->response.append(buffer, bytes);
            bytes = read(pipes[0], buffer, sizeof(buffer));
        }

        if (bytes == -1)
            client->status = CLOSE;
        else
            client->status = WRITE;
        close(pipes[0]);
    }
}

int CGI::run_cgi()
{
    pid = fork();
    if (pid == 0)
    {
        std::cout << "from child\n";
        dup2(pipes[0], STDIN_FILENO);
        dup2(pipes[1], STDOUT_FILENO);
        close(pipes[0]);
        close(pipes[1]);
        if (execve(arg[0], arg, env) == -1)
        {
            std::cerr << strerror(errno) << '\n';
            exit(1);
        }
    }
    if (pid != -1)
    {
        std::cout << "from parent\n";
        write(pipes[1], client->parsed_request.body.c_str(), client->parsed_request.body.size());
        close(pipes[1]);
        is_child_running = 1;
    }else
        return (client->generate_error_response(500), client->status = WRITE, -1);
    return 1;
}

int CGI::checking_permission()
{
    if (access(client->checker.compailer.c_str(), X_OK) != 0)
        return (client->generate_error_response(403), client->status = WRITE, -1);
    if (access(client->checker.root.c_str(), R_OK) != 0)
        return (client->generate_error_response(403), client->status = WRITE, -1);
    return 1;
}

int set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
    {
        perror("fcntl F_GETFL");
        return -1;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        perror("fcntl F_SETFL");
        return -1;
    }
    return 0;
}

int CGI::pipe_init()
{
    if (pipe(pipes) == -1)
        return (client->generate_error_response(500), client->status = WRITE, -1);
    if (set_nonblocking(pipes[0]) == -1)
        return (client->generate_error_response(500), client->status = WRITE, -1);
    if (set_nonblocking(pipes[1]) == -1)
        return (client->generate_error_response(500), client->status = WRITE, -1);
    return 1;
}

void    CGI::create_args()
{
    arg = new char*[3];
    
    arg[0] = new char[client->checker.compailer.length() + 1];
    std::strcpy(arg[0], client->checker.compailer.c_str());

    arg[1] = new char[client->checker.root.length() + 1];
    std::strcpy(arg[1], client->checker.root.c_str());

    arg[2] = NULL;
}

void    CGI::starting_cgi(void)
{
    if (is_child_running == 0)
    {
        if(checking_permission() == -1)
            return ;
        if(pipe_init() == -1)
            return ;
        create_envs();
        if (run_cgi() == -1)
            return ;
    }
    if(is_child_running)
    {
        
    }
}