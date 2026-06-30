#include "CheckRequest.hpp"
#include "Client.hpp"

static std::string size_to_string(size_t value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

std::string retrun_response(std::string code, std::string path)
{
    std::string response = "HTTP/1.1 "+ code + " " + get_http_msg(atoi(code.c_str()))+ "\r\n"+
                            "Location: "+ path + "\r\n"
                            +"Content-Length: " + size_to_string(13 + path.size()) + "\r\n"
                            +"Connection: close\r\n"
                            +"\r\n"
                            +"returning to " + path;
    return response;
}

void    CheckRequest::set_client(Client& clian)
{
    client = &clian;
}

int CheckRequest::get_server(void)
{
    if (!client)
        return 0;
    server = &client->config->findServerByHost(client->parsed_request.host);
    if (!server)
    {
        client->status = CLOSE;
        std::cerr << "error in server config\n";
        return 0;
    }
    return 1;
}
int    CheckRequest::get_location(void)
{
    location = server->findLocation(client->parsed_request.path);
    if (location)
    {
        std::cout << "location found = " << location->path << "\n";
        Directive *dirs = location->getDirective("return");
        if (dirs)
            return (client->response = retrun_response(dirs->values[0], (dirs->values.size() == 2) ? dirs->values[1] : "/"), client->status = WRITE, -1);
        return 1;
    }
    else
    {
        std::cout << "location not found\n";
        Directive *dirs = server->getDirective("return");
        if (dirs)
            return (client->response = retrun_response(dirs->values[0], (dirs->values.size() == 2) ? dirs->values[1] : "/"), client->status = WRITE, -1);
        return 0;
    }
    return 3;
}
int CheckRequest::check_methods(void)
{
    if (location)
    {
        Directive *dir = location->getDirective("methods");
        if (dir)
        {
            for (size_t i = 0; i < dir->values.size(); i++)
            {
                if (dir->values[i] == client->parsed_request.method)
                    return 1;
            }
            return (client->generate_error_response(405), client->status = WRITE, -1);
        }
    }
    if (server)
    {
        Directive *dir = server->getDirective("methods");
        if (dir)
        {
            for (size_t i = 0; i < dir->values.size(); i++)
            {
                if (dir->values[i] == client->parsed_request.method)
                    return 1;
            }
            return (client->generate_error_response(405), client->status = WRITE, -1);
        }
    }
    return 1;
}

int CheckRequest::check_max_body(void)
{
    if (location && location->getDirective("max_body"))
    {
        size_t  max_body = location->getMaxBody();
        if (client->parsed_request.body_len > max_body)
            return (client->generate_error_response(413), client->status = WRITE, -1);
        return 1;
    }
    if (server && server->getDirective("max_body"))
    {
        size_t  max_body = server->getMaxBody();
        if (client->parsed_request.body_len > max_body)
            return (client->generate_error_response(413), client->status = WRITE, -1);
        return 1;
    }
    if (client->parsed_request.body_len > 1048576)
        return (client->generate_error_response(413), client->status = WRITE, -1);
    return 1;
}

std::string removeLocationFromUri(const std::string& uri, const std::string& locationName)
{
    if (locationName.empty())
        return uri;

    if (uri.find(locationName) == 0)
        return uri.substr(locationName.length());
    
    return uri;
}

int CheckRequest::check_root(void)
{
    if (client->parsed_request.path.find("../") != std::string::npos)
        return (client->generate_error_response(400), client->status = WRITE, -1);
    
    if (location)
        root = location->getRoot();
    if (server && root.empty())
        root = server->getRoot();
    if (root.empty())
        root = ".";

    std::string locationName = location ? location->path : "";
    std::string stripped_uri = removeLocationFromUri(client->parsed_request.path, locationName);

    if (!root.empty() && root[root.size() - 1] != '/' && !stripped_uri.empty() && stripped_uri[0] != '/')
        root += "/";
    else if (!root.empty() && root[root.size() - 1] == '/' && !stripped_uri.empty() && stripped_uri[0] == '/')
        stripped_uri.erase(0, 1);

    root += stripped_uri;

    if (root.size() > 4096)
        return (client->generate_error_response(414), client->status = WRITE, -1);
        
    return 1;
}

void CheckRequest::cgi_or_static(void)
{
    if (root.empty())
    {
        client->status = STATIC;
        return;
    }

    size_t last_slash = root.find_last_of("/");
    std::string filename = (last_slash == std::string::npos) ? root : root.substr(last_slash + 1);

    size_t dot_pos = filename.find_last_of(".");
    
    if (dot_pos == std::string::npos || dot_pos == filename.size() - 1)
    {
        client->status = STATIC;
        return;
    }

    std::string ext = filename.substr(dot_pos + 1);
    std::string handler;

    if (location)
    {
        handler = location->getCgiHandler(ext);
        if (!handler.empty())
        {
            compailer = handler;
            client->status = CGI_RUNNING;
            return;
        }
    }
    else if (server)
    {
        handler = server->getCgiHandler(ext);
        if (!handler.empty())
        {
            compailer = handler;
            client->status = CGI_RUNNING;
            return;
        }
    }

    client->status = STATIC;
}

CheckRequest::CheckRequest() : server(NULL), location(NULL), client(NULL),
    is_a_dir(false), root(), compailer() {}

CheckRequest::CheckRequest(const CheckRequest& other) : server(other.server),
    location(other.location), client(other.client), is_a_dir(other.is_a_dir),
    root(other.root), compailer(other.compailer) {}

CheckRequest::CheckRequest(Client& client) : server(NULL), location(NULL),
    client(&client), is_a_dir(false), root(), compailer() {}

CheckRequest& CheckRequest::operator=(const CheckRequest& other)
{
    if (this == &other)
        return *this;
    server = other.server;
    location = other.location;
    client = other.client;
    is_a_dir = other.is_a_dir;
    root = other.root;
    compailer = other.compailer;
    return *this;
}

CheckRequest::~CheckRequest() {}

void    CheckRequest::validate()
{
    if (!client)
        return ;
    std::cout << "checking server\n";
    if (!get_server())
        return ;
    std::cout << "checking location\n";
    if (get_location() == -1)
        return ;
    if (location)
        std::cout << "location_name = " << location->path << '\n';
    else
        std::cout << "no location found\n";
    std::cout << "checking methods\n";
    if (check_methods() == -1)
        return ;
    std::cout << "checking max_body\n";
    if (check_max_body() == -1)
        return ;
    std::cout << "checking root\n";
    if (check_root() == -1)
        return ;
    std::cout << "checking cgi or not\n";
    cgi_or_static();
    std::cout << '\n' << root << '\n';
}