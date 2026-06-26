#include "Client.hpp"
#include "CGI.hpp"
#include <cstring>
#include <cerrno>

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
    delete[] arr;
}

void CGI::building_response(void)
{
    // 1. Find the split between headers and body
    size_t header_end = cgi_buffer.find("\r\n\r\n");
    if (header_end == std::string::npos) 
    {
        // Script didn't output valid headers
        client->generate_error_response(502); 
        client->status = WRITE;
        return;
    }

    // 2. Extract CGI headers and body
    std::string cgi_headers = cgi_buffer.substr(0, header_end + 2); // Keep one \r\n
    std::string cgi_body = cgi_buffer.substr(header_end + 4);       // Skip the \r\n\r\n

    // 3. Determine the status code
    std::string status_line = "";

    // 2. DEFENSIVE CHECK: Did the script output its own HTTP line?
    if (cgi_headers.compare(0, 5, "HTTP/") == 0)
    {
        // Extract the script's HTTP line and remove it from cgi_headers
        size_t line_end = cgi_headers.find('\n');
        if (line_end != std::string::npos)
        {
            status_line = cgi_headers.substr(0, line_end + 1); // Includes the \r\n
            cgi_headers.erase(0, line_end + 1);
        }
    }
    else
    {
        // 3. Normal CGI parsing: Look for "Status:"
        status_line = "HTTP/1.1 "; 
        size_t pos = cgi_headers.find("Status: ");
        if (pos == std::string::npos)
            pos = cgi_headers.find("status: ");

        if (pos != std::string::npos)
        {
            size_t line_end = cgi_headers.find('\r', pos);
            status_line += cgi_headers.substr(pos + 8, line_end - pos - 8) + "\r\n";
            cgi_headers.erase(pos, line_end - pos + 2); // Strip the "Status:" line
        }
        else
        {
            status_line += "200 OK\r\n"; 
        }
    }

    // 4. Build the final HTTP response (Order is now guaranteed to be correct!)
    std::string final_response = status_line;
    
    // Add essential server headers
    std::ostringstream len_stream;
    len_stream << cgi_body.size();
    final_response += "Content-Length: " + len_stream.str() + "\r\n";
    final_response += "Server: webserv/1.0\r\n";

    // Append the CGI's headers, the final blank line, and the body
    final_response += cgi_headers;
    final_response += "\r\n"; 
    final_response += cgi_body;

    // 5. Hand it off to the client to send
    client->response = final_response;
    client->status = WRITE;
}

int CGI::write_to_child()
{
    if (pipe_in[1] == -1)
        return (std::cerr << "Invalid pipe state: write end is closed\n", client->generate_error_response(500), client->status = WRITE, -1);
    ssize_t bytes = 0;

    while((bytes = write(pipe_in[1], client->parsed_request.body.c_str() + data_send, client->parsed_request.body.size() - data_send)) > 0)
        data_send += bytes;

    if (bytes == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
        return (std::cout << strerror(errno) << '\n', client->generate_error_response(500), -1);
    if (data_send == client->parsed_request.body.size())
    {
        writing = 1;
        close(pipe_in[1]);
        pipe_in[1] = -1;
    }
    return 0;
}

int CGI::check_child()
{
    int status;
    pid_t ret = waitpid(pid, &status, WNOHANG);

    if (ret == -1)
    {
        std::cerr << "waitpit: " << strerror(errno) << '\n';
        client->generate_error_response(500);
        client->status = WRITE;
        return -1;
    }

    if (ret == pid)
    {
        child_finished = 1;

        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        {
            client->generate_error_response(500);
            client->status = WRITE;
            return -1;
        }
    }
    return 0;
}

int CGI::reading_from_child()
{
    char buffer[1024];
    ssize_t bytes = 0;
    while ((bytes = read(pipe_out[0], buffer, sizeof(buffer))) > 0)
        cgi_buffer.append(buffer, bytes);
    if (bytes == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
        return (std::cout << strerror(errno) << '\n', client->generate_error_response(500), -1);
    if (bytes == 0)
    {
        reading = 1;
        pipe_closed = true;
    }
    return 0;
}

void CGI::check_cgi()
{
    if (!writing)
        if (write_to_child())
            return ;
    if (!child_finished)
        if (check_child())
            return ;
    if (!reading)
        if (reading_from_child())
            return ;
    if (child_finished && reading)
        status = FINISHED;
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
        strcpy(env[i], envs[i].c_str());
    }
    env[envs.size()] = NULL;
}


int CGI::run_cgi()
{
    pid = fork();

    if (pid == -1)
        return (std::cerr << "fork: " + std::string(strerror(errno)) << '\n', client->generate_error_response(500), client->status = WRITE, -1);
    
    if (pid == 0)
    {
        std::cout << "from child\n";
        dup2(pipe_in[0], STDIN_FILENO);
        dup2(pipe_out[1], STDOUT_FILENO);
        close(pipe_in[0]);
        close(pipe_in[1]);
        close(pipe_out[0]);
        close(pipe_out[1]);
        if (arg == NULL || env == NULL)
        {
            if (arg == NULL)
                std::cerr << "Invalid arguments for execve\n";
            if (env == NULL)
                std::cerr << "Invalid environment for execve\n";
            if (arg == NULL && env == NULL)
                std::cerr << "Invalid arguments and environment for execve\n";
            exit(1);
        }
        if (execve(arg[0], arg, env) == -1)
        {
            std::cerr << strerror(errno) << '\n';
            _exit(1);
        }
    }else
    {
        std::cout << "from parent\n";
        close(pipe_in[0]);
        close(pipe_out[1]);
        pipe_in[0] = -1;
        pipe_out[1] = -1;
        write_to_child();
        status = RUNNING;
    }
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
    if (pipe(pipe_in) == -1)
        return (std::cerr << "pipe: " + std::string(strerror(errno)) << '\n', client->generate_error_response(500), client->status = WRITE, -1);
    if (pipe(pipe_out) == -1)
        return (std::cerr << "pipe: " + std::string(strerror(errno)) << '\n', client->generate_error_response(500), client->status = WRITE, -1);
    if (set_nonblocking(pipe_in[1]) == -1)
        return (std::cerr << "pipe: " + std::string(strerror(errno)) << '\n', client->generate_error_response(500), client->status = WRITE, -1);
    if (set_nonblocking(pipe_out[0]) == -1)
        return (std::cerr << "pipe: " + std::string(strerror(errno)) << '\n', client->generate_error_response(500), client->status = WRITE, -1);
    return 1;
}

void    CGI::create_args()
{
    arg = new char*[3];
    
    arg[0] = new char[client->checker.compailer.length() + 1];
    strcpy(arg[0], client->checker.compailer.c_str());

    arg[1] = new char[client->checker.root.length() + 1];
    strcpy(arg[1], client->checker.root.c_str());

    arg[2] = NULL;
}

void    CGI::starting_cgi(void)
{
    if (status == NOT_RUNNING)
    {
        if(checking_permission() == -1)
            return ;
        if(pipe_init() == -1)
            return ;
        create_envs();
        create_args();
        if (run_cgi() == -1)
            return ;
    }
    if(status == RUNNING || pipe_closed == false)
        check_cgi();
    if (status == FINISHED)
        building_response();
}
