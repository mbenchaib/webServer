#include "Client.hpp"

std::string get_http_msg(int code)
{
    switch (code)
    {
        case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 304: return "Not Modified";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 408: return "Request Timeout";
        case 409: return "Conflict";
        case 411: return "Length Required";
        case 413: return "Payload Too Large";
        case 414: return "URI Too Long";
        case 415: return "Unsupported Media Type";
        case 431: return "Request Header Fields Too Large";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Timeout";
        case 505: return "HTTP Version Not Supported";
        default:  return "Unknown Status";
    }
}


void Client::generate_error_response(int code)
{
    std::string message = get_http_msg(code);
    std::string body;
    bool custom_page_loaded = false;

    if (config != NULL)
    {
        Server server = config->findServerByHost(parsed_request.host);
        std::ostringstream code_stream;
        code_stream << code;
        std::string error_page_path = server.getErrorPage(code_stream.str());

        if (!error_page_path.empty())
        {
            std::ifstream file(error_page_path.c_str(), std::ios::in | std::ios::binary);
            if (file.is_open())
            {
                std::ostringstream oss;
                oss << file.rdbuf();
                body = oss.str();
                custom_page_loaded = true;
                file.close();
            }
            else
            {
                std::cerr << "Error: Configured error page not found or unreadable: " 
                          << error_page_path << std::endl;
            }
        }
    }

    if (!custom_page_loaded)
    {
        std::ostringstream html;
        html << "<html>\r\n<head><title>" << code << " " << message << "</title></head>\r\n"
             << "<body>\r\n<center><h1>" << code << " " << message << "</h1></center>\r\n"
             << "<hr><center>webserv</center>\r\n</body>\r\n</html>";
        body = html.str();
    }

    std::ostringstream response_stream;
    response_stream << "HTTP/1.0 " << code << " " << message << "\r\n"
                    << "Content-Type: text/html\r\n"
                    << "Content-Length: " << body.size() << "\r\n"
                    << "Connection: close\r\n\r\n"
                    << body;
    
    response = response_stream.str();
}


Client::Client(void) : config(NULL), fd(-1), status(READ), read_body(0), bytes_send_to_client(0)
{
    cgi.setClient(this);
    checker.set_client(*this);
}

Client::Client(const Client& other) : config(other.config), cgi(other.cgi),
    time(other.time), fd(other.fd), status(other.status), raw_buffer(other.raw_buffer),
    response(other.response), read_body(other.read_body), parsed_request(other.parsed_request),
    bytes_send_to_client(other.bytes_send_to_client), checker(other.checker)
{
    cgi.setClient(this);
    checker.set_client(*this);
}

Client::Client(int fd, Config& config) : config(&config), fd(fd), status(READ), read_body(0), bytes_send_to_client(0)
{
    cgi.setClient(this);
    checker.set_client(*this);
}

Client& Client::operator=(const Client& other)
{
    if (this == &other)
        return *this;

    config = other.config;
    cgi = other.cgi;
    time = other.time;
    fd = other.fd;
    status = other.status;
    raw_buffer = other.raw_buffer;
    response = other.response;
    read_body = other.read_body;
    parsed_request = other.parsed_request;
    bytes_send_to_client = other.bytes_send_to_client;
    checker = other.checker;

    cgi.setClient(this);
    checker.set_client(*this);
    return *this;
}

Client::~Client() {}
        


int Client::check_recv_error(int bytes)
{
    if (bytes == 0)
    {
        status = CLOSE;
        return 0; 
    }
    else if (bytes == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return -1;

        std::cerr << "recv error: " << strerror(errno) << std::endl;
        status = CLOSE;
        return 0; 
    }
    return 1; 
}
// hadi member function biha client ki9ra men fd

void Client::reading_request(void)
{
    if (status != READ) return;

    char buffer[4096];
    while (true)
    {
        ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);

        if (bytes < 1) { break;}

        raw_buffer.append(buffer, bytes);
    }

    // Parse headers (outside the loop)
    if (!read_body)
    {
        size_t pos = raw_buffer.find("\r\n\r\n");
        if (pos != std::string::npos)
        {
            std::string header_block = raw_buffer.substr(0, pos);
            raw_buffer = raw_buffer.substr(pos + 4);
            parsed_request.parse_request(header_block);
            if (!parsed_request.valid)
            {
                generate_error_response(parsed_request.error_code);
                status = WRITE;
                return;
            }

            if (parsed_request.method != "POST")
            {
                parsed_request.body.clear();
                parsed_request.body_len = 0;
                status = VALIDATION;
                return;
            }
            read_body = 1;
        }
        else if (raw_buffer.size() > 8192)
        {
            // Headers too large
            generate_error_response(431);
            status = WRITE;
            return;
        }
    }

    // Parse body (outside the loop)
    if (read_body)
    {
        if (parsed_request.is_chunked)
        {
            parsed_request.parse_chunked_body(raw_buffer);
            if (parsed_request.chunk_state == CHUNK_DONE)
                status = VALIDATION;
            else if (parsed_request.chunk_state == CHUNK_ERROR)
                { generate_error_response(400); status = WRITE; }
            parsed_request.body_len = parsed_request.body.size();
        }
        else
        {
            parsed_request.body.append(raw_buffer);
            raw_buffer.clear();

            if (parsed_request.body.size() >= parsed_request.body_len)
            {
                parsed_request.body = parsed_request.body.substr(0, parsed_request.body_len);
                status = VALIDATION;
            }
        }
    }
}

void Client::sending_response(void)
{
    if (status != WRITE) return;

    size_t bytes_to_send = response.size() - bytes_send_to_client;
    
    ssize_t bytes_sent = send(fd, response.c_str() + bytes_send_to_client, bytes_to_send, 0);
    if (bytes_sent <= 0)
    {
        status = CLOSE;
        return;
    }

    bytes_send_to_client += bytes_sent;
    if (bytes_send_to_client >= response.size())
        status = CLOSE;
}