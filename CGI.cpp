#include "Client.hpp"
#include "CGI.hpp"
#include <cstring>
#include <cerrno>
#include <sstream>

static std::string extract_header_value(const std::string& headers, const std::string& name)
{
    std::istringstream stream(headers);
    std::string line;

    while (std::getline(stream, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        if (line.empty())
            continue;
        if (line.compare(0, name.size(), name) == 0 && line.size() > name.size() && line[name.size()] == ':')
        {
            size_t value_pos = name.size() + 1;
            while (value_pos < line.size() && (line[value_pos] == ' ' || line[value_pos] == '\t'))
                value_pos++;
            return line.substr(value_pos);
        }
    }
    return std::string();
}

static char **duplicate_memory(char **arr)
{
    if (!arr)
        return NULL;

    size_t count = 0;
    while (arr[count])
        count++;

    char **copy = new char*[count + 1];
    for (size_t i = 0; i < count; ++i)
    {
        copy[i] = new char[std::strlen(arr[i]) + 1];
        std::strcpy(copy[i], arr[i]);
    }
    copy[count] = NULL;
    return copy;
}

CGI::CGI() : pid(-1), env(NULL), arg(NULL), writing(0), reading(0),
    child_finished(0), data_send(0), pipe_closed(false), cgi_buffer(),
    status(NOT_RUNNING), client(NULL)
{
    pipe_in[0] = -1;
    pipe_in[1] = -1;
    pipe_out[0] = -1;
    pipe_out[1] = -1;
}

CGI::CGI(const CGI& other) : pid(-1), env(NULL), arg(NULL), writing(other.writing),
    reading(other.reading), child_finished(other.child_finished), data_send(other.data_send),
    pipe_closed(other.pipe_closed), cgi_buffer(other.cgi_buffer), status(other.status),
    client(other.client)
{
    pipe_in[0] = -1;
    pipe_in[1] = -1;
    pipe_out[0] = -1;
    pipe_out[1] = -1;
    env = duplicate_memory(other.env);
    arg = duplicate_memory(other.arg);
}

CGI& CGI::operator=(const CGI& other)
{
    if (this == &other)
        return *this;

    if (pipe_in[0] != -1) close(pipe_in[0]);
    if (pipe_in[1] != -1) close(pipe_in[1]);
    if (pipe_out[0] != -1) close(pipe_out[0]);
    if (pipe_out[1] != -1) close(pipe_out[1]);
    clear_memory(env);
    clear_memory(arg);

    pipe_in[0] = -1;
    pipe_in[1] = -1;
    pipe_out[0] = -1;
    pipe_out[1] = -1;
    pid = -1;

    env = duplicate_memory(other.env);
    arg = duplicate_memory(other.arg);
    writing = other.writing;
    reading = other.reading;
    child_finished = other.child_finished;
    data_send = other.data_send;
    pipe_closed = other.pipe_closed;
    cgi_buffer = other.cgi_buffer;
    status = other.status;
    client = other.client;
    return *this;
}

CGI::~CGI()
{
    if (pipe_in[0] != -1) close(pipe_in[0]);
    if (pipe_in[1] != -1) close(pipe_in[1]);
    if (pipe_out[0] != -1) close(pipe_out[0]);
    if (pipe_out[1] != -1) close(pipe_out[1]);
    clear_memory(env);
    clear_memory(arg);

    if (pid > 0)
    {
        int wait_status = 0;
        int result = waitpid(pid, &wait_status, WNOHANG);
        if (result == 0)
        {
            kill(pid, SIGKILL);
            waitpid(pid, &wait_status, 0);
        }
    }
}

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
    std::string raw = cgi_buffer;
    std::string headers;
    std::string body;
    int status_code = 200;
    std::string content_type = "text/html";

    size_t header_end = raw.find("\r\n\r\n");
    size_t delimiter_len = 4;
    if (header_end == std::string::npos)
    {
        header_end = raw.find("\n\n");
        delimiter_len = 2;
    }

    if (header_end != std::string::npos)
    {
        headers = raw.substr(0, header_end);
        body = raw.substr(header_end + delimiter_len);
    }
    else
    {
        body = raw;
    }

    if (headers.compare(0, 5, "HTTP/") == 0)
    {
        std::istringstream status_stream(headers);
        std::string http_version;
        std::string status_text;

        status_stream >> http_version >> status_code;
    }

    std::string header_content_type = extract_header_value(headers, "Content-Type");
    if (!header_content_type.empty())
        content_type = header_content_type;

    std::ostringstream response_stream;
    response_stream << "HTTP/1.0 " << status_code << " " << get_http_msg(status_code) << "\r\n"
                    << "Content-Type: " << content_type << "\r\n"
                    << "Content-Length: " << body.size() << "\r\n"
                    << "Connection: close\r\n"
                    << "\r\n"
                    << body;

    client->response = response_stream.str();
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
        return (std::cout << strerror(errno) << '\n', client->generate_error_response(500), client->status = WRITE, -1);
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
        return (std::cout << strerror(errno) << '\n', client->generate_error_response(500), client->status = WRITE, -1);
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
    envs.push_back("SERVER_PROTOCOL=HTTP/1.0");
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
    struct stat file_info;

    if (stat(client->checker.root.c_str(), &file_info) == -1)
        return (client->generate_error_response(404), client->status = WRITE, -1);

    if (!S_ISREG(file_info.st_mode))
        return (client->generate_error_response(403), client->status = WRITE, -1);

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
        signal(SIGPIPE, SIG_IGN);
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
