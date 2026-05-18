#include "server.hpp"

int main()
{
    try
    {
        server webserver;
        webserver.start_server();
    }
    catch(const std::exception& e)
    {
        std::cout << e.what() << '\n';
    }
}