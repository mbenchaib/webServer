#include <vector>
#include <poll.h>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include "Config.hpp"
#include "Request.hpp"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/wait.h>

//enum status dyal client
typedef enum hala
{
    READ,
    WRITE,
    CLOSE
}   hala;
// class dyal client.
// client 3andi ki9da ou kisared 
class client
{
    public:
        int             fd;             // socket_fd dyal client from accept
        int             child_is_on;    // hada key bach n3raf wach ranit child awla ba9i
        int             pipes[2];       //  pipes fd fhal minishell
        int             pid;            // pid dyal child
        hala            status;         // status wach client ki9ra daba awla kisared respone awla CLOSE "sf sala"
        std::string     request;        // hna fin kanjma3 request from fd
        std::string     response;       // hada fih wahed static response just for test
        int             read_body;      // bhadi kanhseb chehal 9rit f body from client fd
        Request         parsed_request; // hada request ba3d ma tparsa
        unsigned long   bytes_send_to_client;   // bhadi cantba3 chechal sardt n client men response

        //difault constractor just for test 
        client(int fd) : fd(fd), bytes_send_to_client(0), read_body(0), child_is_on(0)
        {
            status = READ;
            response = "HTTP/1.0 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: 20\r\n"
                "\r\n"
                "<h1>Hello World</h1>";
        }
        // hadi member function biha client ki9ra men fd
        void    client_read(void)
        {
            // hna kancheki wach 3ando salahiya y9da awla
            if (status != READ)
            {
                std::cout << "client not in READ mode rn\n";
                return ;
            }
            char buffer[10];
            int bytes = recv(fd, buffer, sizeof(buffer), 0);
            if (bytes < 1)
            {
                std::cout << "client disconnected from read\n";
                status = CLOSE;
                return ;
            }
            // fhad case kan9ra gir header mli kanwsal n "\r\n\r\n" cansared dakchi li9rit n parser bach ntala3 method ou bodylen ou kanchecki wach valid awla
            if (!read_body)
            {
                std::cout << "reading request\n";
                request.append(buffer, bytes);
                size_t pos = request.find("\r\n\r\n");
                if (pos != std::string::npos)
                {
                    parsed_request.body = request.substr(pos + 4);
                    request = request.substr(0, pos);
                    parsed_request.parse_request(request);

                    if (!parsed_request.valid)
                    {
                        std::cout << "request is not valid\n";
                        status = CLOSE;
                        return ;
                    }
                    if (parsed_request.body.size() >= parsed_request.body_len)
                    {
                        parsed_request.body = parsed_request.body.substr(0, parsed_request.body_len);
                        status = WRITE;
                        return ;
                    }
                    if (parsed_request.method == "POST" && parsed_request.body_len > 0)
                        read_body = 1;
                    else
                        status = WRITE;
                }
            }
            // hna kan9da body
            else
            {
                std::cout << "reading body\n";
                parsed_request.body.append(buffer, bytes);
                if (parsed_request.body.size() >= parsed_request.body_len)
                {
                    parsed_request.body = parsed_request.body.substr(0, parsed_request.body_len);
                    status = WRITE;
                }
            }
        }
        /*
            mli kisali client men 9raya kibda ybldi response ou sared
            ana hna kantest gir cgi lamakanch kansared wahed text ou sf
        */
        void    client_send(Config& servers)
        {
            if (status != WRITE)
            {
                std::cout << "client not in WRITE mode rn\n";
                return ;
            }
            if (parsed_request.CGI)
            {
                if (!child_is_on)
                {
                    response.clear();
                    ConfigServer    server = servers.Get_Server_By_Host(parsed_request.host);
                    ConfigLocation    *location = server.matchLocation("/cgi-bin");
                    std::string pathname = location->cgi[".py"];
                    std::string full_path = "./www" + parsed_request.path;
                    std::cout << "path = " << pathname << '\n';
                    std::cout << "test = " << full_path << '\n';
                    char **args = new char*[3];
                    args[0] = (char *)pathname.data();
                    args[1] = (char *)full_path.data();
                    args[3] = NULL;
                    char **envs = new char*[3];
                    envs[0] = (char *)std::string("QUERY_STRING=name=foo&age=42").data();
                    envs[1] = (char *)full_path.data();
                    envs[3] = NULL;
                    if (pipe(pipes) == -1)
                        {perror("pipe"); exit(1);}
                    pid = fork();
                    if (pid == -1)
                        {perror("fork"); exit(1);}
                    if (pid == 0) // child
                    {
                        dup2(STDIN_FILENO, pipes[0]);
                        dup2(STDOUT_FILENO, pipes[1]);
                        if (execve(pathname.c_str(), args, envs) == -1)
                            {perror("execve"); exit(1);}
                    }else
                    {
                        write(pipes[1], parsed_request.body.c_str(), parsed_request.body_len);
                        close(pipes[1]);
                        child_is_on = 1;
                    }
                }
                else
                {
                    int status = 0;
                    waitpid(pid, &status, WNOHANG);
                    if (status)
                        {perror("error in child status"); exit(1);}
                    std::cout << "status = " << status << '\n';
                    char buffer[1000];
                    int bytes = read(pipes[0], buffer, sizeof(buffer));
                    if (bytes == -1)
                        {perror("read from child"); exit(1);}
                    if (bytes == 0)
                        {parsed_request.CGI = 0; close(pipes[0]);return;}
                    response.append(buffer, bytes);
                }
            }else
            {
                // std::cout << response << '\n';
                // parsed_request.print();
                int bytes = response.size() - bytes_send_to_client;
                if (bytes > 1000)
                    bytes = 1000;
                int bytes_send = send(fd, response.c_str() + bytes_send_to_client, bytes, 0);
                if (bytes_send <= 0)
                {
                    std::cout << "client disconnected from read\n";
                    status = CLOSE;
                    return ;
                }
                bytes_send_to_client += bytes_send;
                if (bytes_send_to_client >= response.size())
                    status = CLOSE;
            }
        }
};


// hada class dyal server 
class server
{
    public:
        int                         server_socket;
        struct sockaddr_in          add;
        Config                      config;
        std::vector<client>         clients;
        std::vector<struct pollfd>  polls;
        constractor
        server(Config config) : config(config)
        {
            server_socket = socket(AF_INET, SOCK_STREAM, 0);
            if (server_socket == -1)
                {perror("socker");exit(1);}
            int opt = 1;
            setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            add.sin_family = AF_INET;
            add.sin_port = htons(8080);
            add.sin_addr.s_addr = 0;
            if (bind(server_socket, (struct sockaddr *)&add, sizeof(add)) == -1)
                {perror("bind"); exit(1);}
            if (listen(server_socket, 0) == -1)
                {perror("listen"); exit(1);}
            clients.push_back(client(-1));
            struct pollfd temp;
            temp.fd = server_socket;
            temp.events = POLLIN;
            polls.push_back(temp);
            fcntl(server_socket, F_SETFL, O_NONBLOCK);
        }
        // hna kibda server 
        void run_server(void)
        {
            while (1)
            {
                // poll katchofli clients ou status dyalom
                poll(polls.data(), polls.size(), -1);
                // hna canloupi 3la clients kamlin
                for (size_t i = 0; i < polls.size(); i++)
                {
                    
                    if (i == 0 && polls[i].revents & POLLIN)
                    {
                        int new_client = accept(server_socket, NULL, NULL);
                        struct pollfd temp;
                        temp.fd = new_client;
                        temp.events = POLLIN;
                        polls.push_back(temp);
                        clients.push_back(client(new_client));
                        fcntl(new_client, F_SETFL, O_NONBLOCK);
                        std::cout << "new client enter with index="<<clients.size() - 1<<"\n";
                    }
                    else
                    {
                        if ((i != 0 && clients[i].status == READ )&& polls[i].revents & POLLIN)
                        {
                            std::cout << "client index="<<i<<" read rn\n";
                            clients[i].client_read();
                            if (clients[i].status == WRITE)
                                polls[i].events = POLLOUT;
                        }
                        else if ((i != 0 && clients[i].status == WRITE) && polls[i].revents & POLLOUT)
                        {
                            std::cout << "client index="<<i<<" write rn\n";
                            clients[i].client_send(config);
                            if (clients[i].status == CLOSE)
                                polls[i].events = POLLERR|POLLHUP;
                        }
                        if ((i != 0 && clients[i].status == CLOSE) || (polls[i].revents & (POLLERR | POLLHUP)))
                        {
                            close(clients[i].fd);
                            clients.erase(clients.begin() + i);
                            polls.erase(polls.begin() + i);
                            std::cout << "client index=" << i << " leave\n";
                            i--;
                        }
                    }
                }
                
            }
        }
};

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }
    try
    {
        Config config(argv[1]);
        server server(config);
        server.run_server();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Configuration Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}