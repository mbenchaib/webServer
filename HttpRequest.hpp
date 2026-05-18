#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <map>
#include <sstream>
#include <iostream>

class HttpRequest
{
private:
    std::string _method;
    std::string _path;
    std::string _version;
    std::map<std::string, std::string> _headers;
    std::string _body;
    bool _valid;
    bool _is_cgi;

public:
    HttpRequest(const std::string& raw) : _valid(true), _is_cgi(false) {
        parse(raw);
    }

    void parse(const std::string& raw) {
        std::istringstream stream(raw);
        std::string line;

        // ---- FIRST LINE ----
        if (!std::getline(stream, line)) {
            _valid = false;
            return;
        }

        std::istringstream first(line);
        first >> _method >> _path >> _version;

        if (_method != "GET" && _method != "POST" && _method != "DELETE")
        {
            _valid = false;
            return;
        }

        // ---- HEADERS ----
        while (std::getline(stream, line) && line != "\r") {
            size_t pos = line.find(":");
            if (pos == std::string::npos)
                continue;

            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            // remove leading space
            if (!value.empty() && value[0] == ' ')
                value.erase(0, 1);

            // remove trailing \r
            if (!value.empty() && value[value.size() - 1] == '\r')
                value.erase(value.size() - 1);

            _headers[key] = value;
        }

        // ---- BODY ----
        std::string tmp;
        while (std::getline(stream, line)) {
            tmp += line;
        }
        _body = tmp;

        // ---- CGI DETECTION ----
        // simple logic (you can adapt later)
        if (_path.find(".php") != std::string::npos ||
            _path.find(".py") != std::string::npos ||
            _headers.find("Content-Type") != _headers.end())
        {
            _is_cgi = true;
        }
    }

    // ---- GETTERS ----
    bool isValid() const { return _valid; }
    bool isCGI() const { return _is_cgi; }

    const std::string& getMethod() const { return _method; }
    const std::string& getPath() const { return _path; }
    const std::string& getBody() const { return _body; }
    const std::map<std::string, std::string>&   getHeaders() const { return _headers; }
    std::string getHeader(const std::string& key) const
    {
        std::map<std::string, std::string>::const_iterator it = _headers.find(key);
        if (it != _headers.end())
            return it->second;
        return "";
    }
    void    print(void)
    {
        std::cout << "Method : " << _method<< '\n';
        std::cout << "Path : " <<  _path<<'\n';
        for(std::map<std::string, std::string>::iterator begin = _headers.begin(); begin != _headers.end();begin++)
            std::cout << (*begin).first << " : " << (*begin).second << '\n';
    }
};



#endif