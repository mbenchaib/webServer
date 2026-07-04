#include "Client.hpp"
#include "CGI.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <cstring>
#include <iostream>
#include <sstream>

void CGI::setClient(Client *c)
{
    client = c;
}

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

CGI::CGI() : file_in(-1), file_out(-1), pid(-1), env(NULL), arg(NULL), 
             writing(0), reading(0), child_finished(0), data_send(0), 
             pipe_closed(false), cgi_buffer(), status(NOT_RUNNING), client(NULL),
             in_path(""), out_path("")
{
}

CGI::CGI(const CGI& other) : file_in(-1), file_out(-1), pid(-1), env(NULL), arg(NULL),
                             writing(other.writing), reading(other.reading), 
                             child_finished(other.child_finished), data_send(other.data_send),
                             pipe_closed(other.pipe_closed), cgi_buffer(other.cgi_buffer), 
                             status(other.status), client(other.client),
                             in_path(other.in_path), out_path(other.out_path)
{
    env = duplicate_memory(other.env);
    arg = duplicate_memory(other.arg);
}

CGI& CGI::operator=(const CGI& other)
{
    if (this == &other)
        return *this;

    clear_memory(env);
    clear_memory(arg);

    if (file_in >= 0) close(file_in);
    if (file_out >= 0) close(file_out);

    if (!in_path.empty()) unlink(in_path.c_str());
    if (!out_path.empty()) unlink(out_path.c_str());

    pid = -1;
    file_in = -1;
    file_out = -1;

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
    in_path = other.in_path;
    out_path = other.out_path;
    return *this;
}

CGI::~CGI()
{
    clear_memory(env);
    clear_memory(arg);

    if (file_in >= 0) close(file_in);
    if (file_out >= 0) close(file_out);

    if (!in_path.empty()) unlink(in_path.c_str());
    if (!out_path.empty()) unlink(out_path.c_str());

    if (pid > 0)
    {
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
    }
}

void clear_memory(char **arr)
{
    if (!arr)
        return;
    
    int i = 0;
    while (arr[i])
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

int CGI::file_init()
{
    std::ostringstream ss_in, ss_out;
    ss_in << "/tmp/webserv_cgi_in_" << getpid() << "_" << reinterpret_cast<intptr_t>(this);
    ss_out << "/tmp/webserv_cgi_out_" << getpid() << "_" << reinterpret_cast<intptr_t>(this);
    
    in_path = ss_in.str();
    out_path = ss_out.str();

    file_in = open(in_path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (file_in == -1)
    {
        std::cerr << "Failed to create input file: " << in_path << '\n';
        client->generate_error_response(500);
        client->status = WRITE;
        return -1;
    }

    file_out = open(out_path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (file_out == -1)
    {
        std::cerr << "Failed to create output file: " << out_path << '\n';
        close(file_in);
        file_in = -1;
        unlink(in_path.c_str());
        client->generate_error_response(500);
        client->status = WRITE;
        return -1;
    }
    return 1;
}

int CGI::write_to_child()
{
    if (writing)
        return 0;

    size_t total_size = client->parsed_request.body.size();
    size_t remaining = total_size - data_send;
    
    if (remaining == 0)
    {
        writing = 1;
        lseek(file_in, 0, SEEK_SET);
        return 0;
    }

    size_t chunk_size = (remaining > 4096) ? 4096 : remaining;
    ssize_t bytes = write(file_in, client->parsed_request.body.c_str() + data_send, chunk_size);

    if (bytes > 0)
    {
        data_send += bytes;
        if (data_send == total_size)
        {
            writing = 1;
            lseek(file_in, 0, SEEK_SET);
        }
    }
    else if (bytes == -1)
    {
        std::cerr << "write to temp file failed: " << strerror(errno) << '\n';
        client->generate_error_response(500);
        client->status = WRITE;
        return -1;
    }
    return 0;
}

int CGI::check_child()
{
    int status_loc;
    pid_t ret = waitpid(pid, &status_loc, WNOHANG);

    if (ret == -1)
    {
        std::cerr << "waitpid: " << strerror(errno) << '\n';
        client->generate_error_response(500);
        client->status = WRITE;
        return -1;
    }

    if (ret == pid)
    {
        child_finished = 1;
        pid = -1;

        if (!WIFEXITED(status_loc) || WEXITSTATUS(status_loc) != 0)
        {
            client->generate_error_response(500);
            client->status = WRITE;
            return -1;
        }
        lseek(file_out, 0, SEEK_SET);
    }
    return 0;
}

int CGI::reading_from_child()
{
    char buffer[4096];
    ssize_t bytes = read(file_out, buffer, sizeof(buffer));

    if (bytes > 0)
    {
        cgi_buffer.append(buffer, bytes);
    }
    else if (bytes == 0)
    {
        reading = 1;
        close(file_out);
        file_out = -1;
    }
    else
    {
        std::cerr << "read from output file failed: " << strerror(errno) << '\n';
        client->generate_error_response(500);
        client->status = WRITE;
        return -1;
    }
    return 0;
}

void CGI::check_cgi(struct pollfd& p)
{
    (void)p;

    if (!writing)
    {
        if (write_to_child() == -1)
            return;
        if (writing)
        {
            if (run_cgi_process() == -1)
                return;
        }
        return;
    }

    if (!child_finished)
    {
        if (check_child() == -1)
            return;
    }

    if (child_finished && !reading)
    {
        if (reading_from_child() == -1)
            return;
    }

    if (writing && child_finished && reading)
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

int CGI::run_cgi_process()
{
    pid = fork();
    if (pid == -1)
    {
        std::cerr << "fork system call failed\n";
        client->generate_error_response(500);
        client->status = WRITE;
        return -1;
    }

    if (pid == 0)
    {
        if (chdir(dir.c_str()) == -1)
        {
            std::cerr << "chdir failed: " << strerror(errno) << '\n';
            _exit(1);
        }
        dup2(file_in, STDIN_FILENO);
        dup2(file_out, STDOUT_FILENO);

        close(file_in);
        close(file_out);

        if (arg == NULL || env == NULL)
        {
            _exit(1);
        }
        execve(arg[0], arg, env);
        _exit(1);
    }
    else
    {
        close(file_in);
        file_in = -1;
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

void CGI::create_args()
{
    dir = client->checker.root.substr(0, client->checker.root.find_last_of('/'));
    script = client->checker.root.substr(client->checker.root.find_last_of('/') + 1);
    arg = new char*[3];
    
    arg[0] = new char[client->checker.compailer.length() + 1];
    strcpy(arg[0], client->checker.compailer.c_str());

    arg[1] = new char[script.length() + 1];
    strcpy(arg[1], script.c_str());

    arg[2] = NULL;
}

void CGI::starting_cgi(std::vector<struct pollfd>& pfds, struct pollfd& p)
{
    (void)pfds;
    if (status == NOT_RUNNING)
    {
        if (checking_permission() == -1)
            return;
        if (file_init() == -1)
            return;
        create_envs();
        create_args();
        status = RUNNING;
    }
    if (status == RUNNING)
        check_cgi(p);
    if (status == FINISHED)
    {
        building_response();
    }
}