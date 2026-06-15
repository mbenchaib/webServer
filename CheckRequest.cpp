#include "CheckRequest.hpp"
#include "Client.hpp"

std::string create_301_302_response(std::string code, std::string path)
{
    std::string response = "HTTP/1.1 "+ code + " " + get_http_msg(atoi(code.c_str()))+ "\r\n"+
                            "Location: "+ path + "\r\n"
                            +"Content-Length: 0\r\n"+
                            +"Connection: close\r\n"+
                            +"\r\n";
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
    std::cout << "hello\n";
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
            return (client->response = create_301_302_response(dirs->values[0], dirs->values[1]), client->status = WRITE, -1);
        return 1;
    }
    else
    {
        std::cout << "location not found\n";
        Directive *dirs = server->getDirective("return");
        if (dirs)
            return (client->response = create_301_302_response(dirs->values[0], dirs->values[1]), client->status = WRITE, -1);
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
    
    root += client->parsed_request.path;
    
    if (root.size() > 4096)
        return (client->generate_error_response(414), client->status = WRITE, -1);
    return 1;
}
void CheckRequest::cgi_or_static(void)
{
    if (root[root.size() - 1] == '/')
        return (client->status = STATIC, (void)0);
    size_t pos = root.rfind(".");
    if (pos != std::string::npos)
    {
        std::string ext = root.substr(pos);
        std::string inter;
        if (location)
        {
            inter = location->getCgiHandler(ext);
            if (!inter.empty())
            {
                client->status = CGI_RUNNING;
                return ;
            }
        }
        if (server)
        {
            inter = server->getCgiHandler(ext);
            if (!inter.empty())
            {
                client->status = CGI_RUNNING;
                return ;
            }
        }
    }
    client->status = STATIC;
}

CheckRequest::CheckRequest() : server(NULL), location(NULL), client(NULL) {};
CheckRequest::CheckRequest(Client& client) : server(NULL), location(NULL), client(&client) {};

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
}