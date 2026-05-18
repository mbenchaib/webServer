#include "client.hpp"

void    check_status(int status, int throw_or_not, std::string msg)
{
    if (status == -1)
    {
        if (throw_or_not)
            throw std::runtime_error(msg.c_str());
        perror(msg.c_str());
        exit(1);
    }
}

void    client::read_body(struct pollfd &fds)
{
    if (is_body_full)
        return ;
    char buffer[1000];
    int bytes = recv(fd, buffer, sizeof(buffer), 0);
    if (bytes <= 0)
    {
        state = CLOSE;
        return ;
    }
    check_status(bytes, 0, "recv");
    body.append(buffer, bytes);
    if (body.size() >= content_length)
    {
        is_body_full = 1;
        state = WRITE;
        fds.events = POLLOUT;
        std::cout << request << '\n';
        parcing_req = HttpRequest(request.append(body, content_length));
        parcing_req.print();
        built_request();
    }
}

void client::read_request(struct pollfd &fds)
{
    if (is_header_full) {
        read_body(fds);
        return;
    }

    char buffer[1000];
    int bytes = recv(fd, buffer, sizeof(buffer), 0);
    if (bytes <= 0) {
        state = CLOSE;
        return;
    }

    request.append(buffer, bytes);

    size_t pos = request.find("\r\n\r\n");
    if (pos != std::string::npos)
    {
        std::string headers = request.substr(0, pos + 4);
        std::string extra   = request.substr(pos + 4);

        request = headers;
        body += extra;

        HttpRequest temp(request);
        std::string len = temp.getHeader("Content-Length");

        if (!len.empty())
            content_length = atol(len.c_str());
        else {
            parcing_req = HttpRequest(request);
            parcing_req.print();
            built_request();
            fds.events = POLLOUT;
            state = WRITE;
            is_body_full = 1;
        }
        is_header_full = 1;
    }
}

void    client::built_request(void)
{
    std::cout << parcing_req.getPath() << '\n';
    HttpResponse res(parcing_req);
    response = res.getRaw();
}
void    client::send_response(struct pollfd &fds)
{
    int bytes_to_send = response.size() - bytes_send_client;
    if (bytes_to_send > 1000)
        bytes_to_send = 1000;
    int bytes = send(fd, response.c_str() + bytes_send_client, bytes_to_send, 0);
    check_status(bytes, 0, "send");
    if (bytes == 0)
    {
        state = CLOSE;
        return;
    }
    bytes_send_client += bytes;
    if (bytes_send_client == response.size())
    {
        state = CLOSE;
        fds.events = POLLIN;
        bytes_send_client = 0;
    }
}
client::~client(void)
{
    std::cout << "client "<< fd << " have gone with state " << state << "\n";
}