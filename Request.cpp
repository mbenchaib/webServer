#include "Request.hpp"
#include <algorithm>
#include <climits>

Request::Request(void) : error_code(0), method(), path(), query_string(), version(),
    host(), body(), content_type(), body_len(0), valid(true), is_chunked(false),
    chunk_state(CHUNK_SIZE), current_chunk_size(0), headers() {}

Request::Request(const Request& other) : error_code(other.error_code),
    method(other.method), path(other.path), query_string(other.query_string),
    version(other.version), host(other.host), body(other.body),
    content_type(other.content_type), body_len(other.body_len), valid(other.valid),
    is_chunked(other.is_chunked), chunk_state(other.chunk_state),
    current_chunk_size(other.current_chunk_size), headers(other.headers) {}

Request& Request::operator=(const Request& other)
{
    if (this == &other)
        return *this;
    error_code = other.error_code;
    method = other.method;
    path = other.path;
    query_string = other.query_string;
    version = other.version;
    host = other.host;
    body = other.body;
    content_type = other.content_type;
    body_len = other.body_len;
    valid = other.valid;
    is_chunked = other.is_chunked;
    chunk_state = other.chunk_state;
    current_chunk_size = other.current_chunk_size;
    headers = other.headers;
    return *this;
}

Request::~Request() {}

void trim_crlf(std::string& s)
{
    if (!s.empty() && s[s.length() - 1] == '\n')
        s.erase(s.length() - 1);
    if (!s.empty() && s[s.length() - 1] == '\r')
        s.erase(s.length() - 1);
}


void Request::check_first_line(std::stringstream& first)
{
    first >> method >> path >> version;
    if (method.empty() || path.empty() || version.empty())
    {
        valid = false;
        error_code = 400;
        return;
    }

    if (method != "GET" && method != "POST" && method != "DELETE")
        return (valid = false, error_code = 405, (void)0);
    
    trim_crlf(version);
    if (version != "HTTP/1.0" && version != "HTTP/1.1")
        return (valid = false, error_code = 505, (void)0);

    size_t hash_pos = path.find("#");
    if (hash_pos != std::string::npos)
        path = path.substr(0, hash_pos);

    size_t pos = path.find("?");
    if (pos != std::string::npos)
    {
        query_string = path.substr(pos + 1);
        path = path.substr(0, pos);
    }
}

void    Request::check_body_len(void)
{
    bool has_cl = headers.find("content-length") != headers.end();
    bool has_te = headers.find("transfer-encoding") != headers.end();

    if (has_cl && has_te)
    {
        error_code = 400;
        valid = false;
        return;
    }
    if (headers.find("transfer-encoding") != headers.end() && headers["transfer-encoding"] == "chunked")
        is_chunked = true;

    else if (headers.find("content-length") != headers.end())
    {
        std::string cl = headers["content-length"];
        for (size_t i = 0; i < cl.size(); i++)
            if (!isdigit(cl[i])) { error_code = 400, valid = false; return; }
        
        if (cl.size() > 10) { error_code = 413, valid = false; return; }
        
        body_len = strtoul(cl.c_str(), NULL, 10);
        
        // if (body_len > INT_MAX)  { error_code = 413, valid = false; return; }
    }
    else if (method == "POST")
    {
        error_code = 411;
        valid = false;
        return;
    }
}

void    Request::parse_request(const std::string& raw)
{
    std::stringstream stream(raw);
    std::string line;

    if (!std::getline(stream, line))
        return (valid = false, std::cout << "error in getline\n", error_code = 500, (void)0);

    std::stringstream first(line);
    check_first_line(first);
    if (!valid) return;

    while (std::getline(stream, line))
    {
        trim_crlf(line);
        if (line.empty()) break;

        size_t pos = line.find(":");
        if (pos == std::string::npos) continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        size_t start = value.find_first_not_of(" \t");
        if (start != std::string::npos) value = value.substr(start);
        trim_crlf(value);

        for (size_t i = 0; i < key.size(); ++i) key[i] = std::tolower(key[i]);

        if (key == "host" && host.empty()) { host = value; continue; }
        if (key == "content-type" && content_type.empty()) { content_type = value; continue; }
        
        headers[key] = value;
    }
    check_body_len();
}

void Request::parse_chunked_body(std::string& raw_buffer)
{
    while (!raw_buffer.empty() && chunk_state != CHUNK_DONE && chunk_state != CHUNK_ERROR)
    {
        if (chunk_state == CHUNK_SIZE)
        {
            size_t pos = raw_buffer.find("\r\n");
            if (pos == std::string::npos) return; 

            std::string hex_size = raw_buffer.substr(0, pos);
            char *end;
            current_chunk_size = strtol(hex_size.c_str(), &end, 16);
            
            raw_buffer.erase(0, pos + 2); 
            
            if (current_chunk_size == 0)
            {
                chunk_state = CHUNK_DONE;
                return;
            }
            chunk_state = CHUNK_DATA;
        }
        else if (chunk_state == CHUNK_DATA)
        {
            if (raw_buffer.size() >= current_chunk_size)
            {
                body.append(raw_buffer.substr(0, current_chunk_size));
                raw_buffer.erase(0, current_chunk_size);
                chunk_state = CHUNK_CRLF;
            }
            else return; 
        }
        else if (chunk_state == CHUNK_CRLF)
        {
            if (raw_buffer.size() >= 2)
            {
                if (raw_buffer.substr(0, 2) == "\r\n")
                {
                    raw_buffer.erase(0, 2);
                    chunk_state = CHUNK_SIZE;
                }
                else chunk_state = CHUNK_ERROR; 
            }
            else return; 
        }
    }
}

void Request::print(void)
{
    std::cout << "\tFIRST LINE\t\n";
    std::cout << "method : " <<"\""<< method <<"\""<< '\n';
    std::cout << "path : " <<"\""<< path <<"\""<< '\n';
    std::cout << "version : " <<"\""<< version <<"\""<< '\n';
    std::cout << "host : " <<"\""<< host <<"\""<< '\n';
    std::cout << "body_len : " <<"\""<< body_len <<"\""<< '\n';
}