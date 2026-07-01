#include "Response.hpp"
#include "Client.hpp"
#include <fstream>
#include <sstream>
#include <dirent.h>
#include <sys/stat.h>
#include <cstdio>

Response::Response(Client& client) : client(&client) {}

// ===== low level: stat path, kn3rfo wach kayn ou wach dir =====

bool Response::file_exists(const std::string& path, bool& is_dir)
{
    struct stat st;

    if (stat(path.c_str(), &st) != 0)
        return (false);
    is_dir = S_ISDIR(st.st_mode);
    return (true);
}

// ===== MIME type men extension dyal file =====

std::string Response::get_mime_type(const std::string& path)
{
    size_t pos = path.find_last_of('.');
    if (pos == std::string::npos)
        return ("application/octet-stream");

    std::string ext = path.substr(pos + 1);

    if (ext == "html" || ext == "htm")  return ("text/html");
    if (ext == "css")                   return ("text/css");
    if (ext == "js")                    return ("application/javascript");
    if (ext == "json")                  return ("application/json");
    if (ext == "txt")                   return ("text/plain");
    if (ext == "xml")                   return ("application/xml");
    if (ext == "jpg" || ext == "jpeg")  return ("image/jpeg");
    if (ext == "png")                   return ("image/png");
    if (ext == "gif")                   return ("image/gif");
    if (ext == "svg")                   return ("image/svg+xml");
    if (ext == "ico")                   return ("image/x-icon");
    if (ext == "pdf")                   return ("application/pdf");
    return ("application/octet-stream");
}

// ===== kibni response line + headers + body (nafs style dyal generate_error_response) =====

std::string Response::build_response(int code, const std::string& content_type, const std::string& body)
{
    std::ostringstream oss;

    oss << "HTTP/1.1 " << code << " " << get_http_msg(code) << "\r\n"
        << "Content-Type: " << content_type << "\r\n"
        << "Content-Length: " << body.size() << "\r\n"
        << "Connection: close\r\n"
        << "\r\n"
        << body;
    return (oss.str());
}

// ===== config helpers =====

// index: location lwel, men b3d server ila location ma3ndosh index directive
std::string Response::get_index_path(void)
{
    Directive* dir = NULL;

    if (client->checker.location)
        dir = client->checker.location->getDirective("index");
    else
        dir = client->checker.server->getDirective("index");
    if (!dir)
        return ("");

    const std::vector<std::string>& names = dir->getValues();

    std::string base = client->checker.root;
    if (!base.empty() && base[base.size() - 1] != '/')
        base += "/";

    for (size_t i = 0; i < names.size(); i++)
    {
        std::string candidate = base + names[i];
        bool        is_dir = false;

        if (file_exists(candidate, is_dir) && !is_dir)
            return (candidate);
    }
    return ("");
}

// autoindex: directive dyal location only (hakda f initRules)
bool Response::autoindex_enabled(void)
{
    Directive* dir = NULL;

    if (client->checker.location)
        dir = client->checker.location->getDirective("autoindex");
    else
        dir = client->checker.server->getDirective("autoindex");
    if (!dir)
        return (false);
    return (dir->getValues()[0] == "on");
}

// upload_path: directive dyal location only
std::string Response::get_upload_path(void)
{
    if (client->checker.location)
    {
        Directive* dir = client->checker.location->getDirective("upload_path");
        if (dir)
            return (dir->getValues()[0]);
    }else
    {
        Directive* dir = client->checker.server->getDirective("upload_path");
        if (dir)
            return (dir->getValues()[0]);
    }
    return ("");
}

// ===== static serving =====

void Response::serve_file(const std::string& path)
{
    std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        // file kayn (tvalida) walakin ma9drnach n9rawh -> 403
        client->generate_error_response(403);
        return;
    }

    std::ostringstream oss;
    oss << file.rdbuf();
    std::string body = oss.str();
    file.close();

    client->response = build_response(200, get_mime_type(path), body);
}

void Response::generate_autoindex(const std::string& path)
{
    DIR* dir = opendir(path.c_str());
    if (!dir)
    {
        client->generate_error_response(403);
        return;
    }

    // request path khass ysali b '/' bach links ykono s7a7
    std::string req_path = client->parsed_request.path;
    if (req_path.empty() || req_path[req_path.size() - 1] != '/')
        req_path += "/";

    std::ostringstream html;
    html << "<html>\r\n<head><title>Index of " << req_path << "</title></head>\r\n"
         << "<body>\r\n<h1>Index of " << req_path << "</h1>\r\n<hr>\r\n<pre>\r\n";

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;
        if (name == ".")
            continue;
        html << "<a href=\"" << req_path << name << "\">" << name << "</a>\r\n";
    }
    closedir(dir);

    html << "</pre>\r\n<hr>\r\n</body>\r\n</html>";
    client->response = build_response(200, "text/html", html.str());
}

void Response::serve_directory(void)
{
    // 1. index file ila kayn n serviwh
    std::string index_path = get_index_path();
    if (!index_path.empty())
    {
        serve_file(index_path);
        return;
    }

    // 2. ma kayn index -> autoindex ila on
    else if (autoindex_enabled())
    {
        generate_autoindex(client->checker.root);
        return;
    }

    // 3. la la -> 403
    client->generate_error_response(403);
}

// ===== method handlers =====

void Response::handle_get(void)
{
    struct stat st;
    bool is_dir = false;

    if (stat(client->checker.root.c_str(), &st) != 0)
        return (client->generate_error_response(404));

    if (S_ISDIR(st.st_mode))
        is_dir = true;
    else if (!S_ISREG(st.st_mode))
        return (client->generate_error_response(403));
    else if (access(client->checker.root.c_str(), R_OK) != 0)
        return (client->generate_error_response(403));

    if (is_dir)
        serve_directory();
    else
        serve_file(client->checker.root);
}

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

void Response::handle_post(void)
{
    std::string upload_dir = get_upload_path();
    if (upload_dir.empty())
    {
        client->generate_error_response(403);
        return;
    }

    struct stat st;
    if (stat(upload_dir.c_str(), &st) != 0)
    {
        client->generate_error_response(500);
        return;
    }

    if (!S_ISDIR(st.st_mode))
    {
        client->generate_error_response(500);
        return;
    }

    if (access(upload_dir.c_str(), W_OK) != 0)
    {
        client->generate_error_response(403);
        return;
    }

    std::string filename;
    std::string uri = client->parsed_request.path;
    size_t pos = uri.find_last_of('/');

    if (pos == std::string::npos)
        filename = uri;
    else
        filename = uri.substr(pos + 1);

    if (filename.empty())
    {
        client->generate_error_response(400);
        return;
    }

    std::string full_path = upload_dir;
    if (full_path[full_path.size() - 1] != '/')
        full_path += "/";
    full_path += filename;

    int fd = open(full_path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0)
    {
        if (errno == EACCES)
            client->generate_error_response(403);
        else
            client->generate_error_response(500);
        return;
    }

    const char *data = client->parsed_request.body.data();
    size_t total = client->parsed_request.body.size();
    size_t written = 0;

    while (written < total)
    {
        ssize_t n = write(fd, data + written, total - written);
        if (n <= 0)
        {
            close(fd);
            client->generate_error_response(500);
            return;
        }
        written += n;
    }

    close(fd);

    client->response = build_response(
        201,
        "text/html",
        "<html>\r\n"
        "<body>\r\n"
        "<h1>201 Created</h1>\r\n"
        "</body>\r\n"
        "</html>");
}

void Response::handle_delete(void)
{
    std::string path = client->checker.root;
    bool        is_dir = false;

    if (!file_exists(path, is_dir))
    {
        client->generate_error_response(404);
        return;
    }
    if (is_dir)
    {
        client->generate_error_response(403);
        return;
    }
    if (std::remove(path.c_str()) != 0)
    {
        client->generate_error_response(403);
        return;
    }
    client->response = build_response(200, "text/html",
        "<html>\r\n<body>\r\n<h1>200 OK</h1>\r\n<p>File deleted</p>\r\n</body>\r\n</html>");
}

// ===== entry point =====

void Response::build(void)
{
    const std::string& method = client->parsed_request.method;

    if (method == "GET")
        handle_get();
    else if (method == "POST")
        handle_post();
    else if (method == "DELETE")
        handle_delete();
    else
        client->generate_error_response(405);

    // wakha generate_error_response kayseti WRITE, kanseth hna 7it path dyal success
    client->status = WRITE;
}
