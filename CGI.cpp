#include "Client.hpp"
#include "CGI.hpp"

char**  CGI::create_envs()
{
    std::vector<std::string> envs;

    envs.push_back("REQUEST_METHOD=" + client->parsed_request.method);
    envs.push_back("QUERY_STRING=" + client->parsed_request.query_string);
    envs.push_back("SCRIPT_NAME=" + client->parsed_request.path);

    std::ostringstream oss;
    oss << client->parsed_request.body.size();
    envs.push_back("CONTENT_LENGTH=" + oss.str());

    envs.push_back("CONTENT_TYPE=" + client->parsed_request.content_type);
    envs.push_back("HTTP_HOST=" + client->parsed_request.host);

    char **env = new char *[envs.size() + 1];
    for (size_t i = 0; i < envs.size(); i++)
    {
        env[i] = new char[envs[i].size() + 1];
        strcpy(env[i], envs[i].c_str());
    }
    env[envs.size()] = NULL;
    return env;
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

void CGI::run_cgi()
{
    if (is_child_running)
        return ;
    std::cout << "geting server by host\n";
    Server server = client->config->findServerByHost(client->parsed_request.host);
    std::cout << "secssus geting server by host\n";
    std::cout << "geting location by path\n";
    Location *location = server.findLocation(client->parsed_request.path);
    std::cout << "secssus geting location by path\n";
    
    std::string root;
    if (location && !location->getRoot().empty())
        root = location->getRoot();
    else if (!server.getRoot().empty())
        root = server.getRoot();
    else
        root = ".";

    root += client->parsed_request.path;
    std::cout << "root = " << root << '\n';

    if (pipe(pipes) == -1)
        return (std::cout << "error in pipes\n", client->status = CLOSE, (void)0);
    pid = fork();
    if (pid == 0)
    {
        std::cout << "from child\n";
        dup2(pipes[0], STDIN_FILENO);
        dup2(pipes[1], STDOUT_FILENO);
        close(pipes[0]);
        close(pipes[1]);
        char **env = create_envs();
        char **arg = new char*[3];
        arg[0] = (char *)(std::string("/usr/bin/python3").c_str());
        arg[1] = (char *)(root.c_str());
        arg[2] = NULL;
        if (execve(arg[0], arg, env) == -1)
        {
            std::cerr << strerror(errno) << '\n';
            exit(1);
        }
    }else if (pid != -1)
    {
        std::cout << "from parent\n";
        write(pipes[1], client->parsed_request.body.c_str(), client->parsed_request.body.size());
        close(pipes[1]);
        is_child_running = 1;
    }else
        return (client->status = CLOSE, std::cout << "error in fork\n", (void)0);
}
