#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include "HttpRequest.hpp"
#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>

class HttpResponse
{
private:
    int         _status_code;
    std::string _status_text;
    std::string _body;
    std::string _content_type;
    std::string _raw;
    bool        _built;

    // ---------- helpers ----------

    static std::string getMimeType(const std::string &path)
    {
        size_t dot = path.rfind('.');
        if (dot == std::string::npos)
            return "application/octet-stream";

        std::string ext = path.substr(dot);
        if (ext == ".html" || ext == ".htm") return "text/html";
        if (ext == ".css")                   return "text/css";
        if (ext == ".js")                    return "application/javascript";
        if (ext == ".json")                  return "application/json";
        if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
        if (ext == ".png")                   return "image/png";
        if (ext == ".gif")                   return "image/gif";
        if (ext == ".ico")                   return "image/x-icon";
        if (ext == ".txt")                   return "text/plain";
        return "application/octet-stream";
    }

    static bool fileExists(const std::string &path)
    {
        struct stat st;
        return (stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode));
    }

    static std::string readFile(const std::string &path)
    {
        std::ifstream file(path.c_str(), std::ios::binary);
        if (!file.is_open())
            return "";
        std::ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    static bool deleteFile(const std::string &path)
    {
        return (remove(path.c_str()) == 0);
    }

    void setStatus(int code)
    {
        _status_code = code;
        switch (code)
        {
            case 200: _status_text = "OK";                    break;
            case 201: _status_text = "Created";               break;
            case 204: _status_text = "No Content";            break;
            case 400: _status_text = "Bad Request";           break;
            case 404: _status_text = "Not Found";             break;
            case 405: _status_text = "Method Not Allowed";    break;
            case 500: _status_text = "Internal Server Error"; break;
            default:  _status_text = "Unknown";               break;
        }
    }

    // ---------- method handlers ----------

    void handleGET(const HttpRequest &req)
    {
        // Sanitize path — strip query string and leading slash
        std::string path = req.getPath();
        size_t q = path.find('?');
        if (q != std::string::npos)
            path = path.substr(0, q);

        // Map "/" to a default file
        if (path == "/")
            path = "/index.html";

        // Prepend your www root — adjust as needed
        std::string full_path = "." + path;

        if (!fileExists(full_path))
        {
            setStatus(404);
            _content_type = "text/html";
            _body = "<html><body><h1>404 Not Found</h1></body></html>";
            return;
        }

        _body = readFile(full_path);
        if (_body.empty())
        {
            setStatus(500);
            _content_type = "text/html";
            _body = "<html><body><h1>500 Internal Server Error</h1></body></html>";
            return;
        }

        setStatus(200);
        _content_type = getMimeType(full_path);
    }

    void handlePOST(const HttpRequest &req)
    {
        std::string body = req.getBody();

        if (body.empty())
        {
            setStatus(400);
            _content_type = "text/plain";
            _body = "Bad Request: empty body";
            return;
        }

        // Echo body back for now — replace with your real logic
        setStatus(200);
        _content_type = "text/plain";
        _body = "Received: " + body;
    }

    void handleDELETE(const HttpRequest &req)
    {
        std::string path = req.getPath();
        size_t q = path.find('?');
        if (q != std::string::npos)
            path = path.substr(0, q);

        std::string full_path = "." + path;

        if (!fileExists(full_path))
        {
            setStatus(404);
            _content_type = "text/plain";
            _body = "Not Found";
            return;
        }

        if (!deleteFile(full_path))
        {
            setStatus(500);
            _content_type = "text/plain";
            _body = "Internal Server Error: could not delete file";
            return;
        }

        setStatus(204);
        _content_type = "text/plain";
        _body = "";
    }

public:
    HttpResponse(const HttpRequest &req) : _status_code(200), _built(false)
    {
        if (!req.isValid())
        {
            setStatus(400);
            _content_type = "text/html";
            _body = "<html><body><h1>400 Bad Request</h1></body></html>";
        }
        else
        {
            const std::string &method = req.getMethod();
            if      (method == "GET")    handleGET(req);
            else if (method == "POST")   handlePOST(req);
            else if (method == "DELETE") handleDELETE(req);
            else
            {
                setStatus(405);
                _content_type = "text/plain";
                _body = "Method Not Allowed";
            }
        }
        build();
    }

    // Assembles the raw HTTP response string once
    void build()
    {
        if (_built)
            return;
        _built = true;

        _raw =  "HTTP/1.0 " + std::to_string(_status_code) + " " + _status_text + "\r\n";
        _raw += "Content-Type: "   + _content_type + "\r\n";
        _raw += "Content-Length: " + std::to_string(_body.size()) + "\r\n";
        _raw += "Connection: close\r\n";
        _raw += "\r\n";
        _raw += _body;
    }

    const std::string &getRaw()  const { return _raw; }
    int                getCode() const { return _status_code; }
};

#endif