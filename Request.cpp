#include "Request.hpp"

void    trim_crlf(std::string& s)
{
    if (!s.empty() && s.back() == '\r')
        s.pop_back();
    if (!s.empty() && s.back() == '\n')
        s.pop_back();
}

int check_path(std::string path)
{
    for (int i = 0;i < path.size();i++)
    {
        if (path[i] < 32 || path[i] > 126)
            return 0;
    }
    return 1;
}

void    Request::check_first_line(std::stringstream& first)
{
    first >> method >> path >> version;
    if (method.empty() || path.empty() || version.empty())
    {
        valid = false;
        return;
    }

    std::string extra;
    if (first >> extra)
        { valid = false; std::cout << "extra input in first line\n";return;}
    if (method != "GET" && method != "POST" && method != "DELETE")
        { valid = false; std::cout << "method not valid\n";return;}
    if (version != "HTTP/1.0" && version != "HTTP/1.1")
        { valid = false; std::cout << "HTTP not valid\n";return;}
    if (path.size() == 0)
        { valid = false; std::cout << "path not valid\n";return;}
    trim_crlf(version);
    if (!check_path(path))
        { valid = false; std::cout << "path not valid\n";return;}
    size_t  pos = path.find("?");
    if (pos != std::string::npos)
    {
        quere_string = path.substr(pos + 1);
        path = path.substr(0, pos);
    }
    pos = path.find(".py");
    if (pos != std::string::npos && (pos + 3) == path.size())
        CGI = true;
    pos = path.find(".php");
    if (pos != std::string::npos && (pos + 4) == path.size())
        CGI = true;
}

void Request::parse_request(const std::string& raw)
{
    std::stringstream stream(raw);
    std::string line;

    if (!std::getline(stream, line))
        { valid = false; std::cout << "method not valid\n";return;}

    std::stringstream first(line);
    check_first_line(first);
    if (!valid)
        return;

    while (std::getline(stream, line))
    {
        trim_crlf(line);
        if (line.empty())
            break;

        size_t pos = line.find(":");
        if (pos == std::string::npos)
            continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        size_t start = value.find_first_not_of(" \t");
        if (start != std::string::npos)
            value = value.substr(start);

        if (key == "Host")
            {host = value;continue;}
        if (key == "content_type")
            {content_type = value;continue;}
        trim_crlf(value);
        headers[key] = value;
    }
    if (method == "POST" && headers.find("Content-Length") == headers.end())
        { valid = false; return; }
    std::string Content_Length = headers["Content-Length"];
    if (Content_Length.size())
    {
        for (size_t i = 0; i < Content_Length.size(); i++)
            if (!isdigit(Content_Length[i]))
                { valid = false; return; }
        body_len = strtod(Content_Length.c_str(), NULL);
    }
}

void    Request::print(void)
{
    std::cout << "\tFIRST LINE\t\n";
    std::cout << "method : " << method << '\n';
    std::cout << "path : " << path << '\n';
    std::cout << "quere_string : " << quere_string << '\n';
    std::cout << "version : " << version << '\n';

    std::cout << "host : " << host << '\n';
    std::cout << "body : " << body << '\n';
    std::cout << "content_type : " << content_type << '\n';
    std::cout << "body_len : " << body_len << '\n';
    if (CGI)
        std::cout << "REQUEST IS CGI\n";
    std::cout << "\tHEADERS LINES\t\n";
    std::cout << "KEY\tVALUE\n";
    for (std::map<std::string, std::string>::iterator begin = headers.begin(); begin != headers.end(); begin++)
        std::cout << (*begin).first<< "\t"<<(*begin).second<<'\n';
    if (body_len)
        std::cout << "\tBODY LINE\n" << body<<'\n';
}