/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sael-kha <sael-kha@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/26 22:12:10 by mben-cha          #+#    #+#             */
/*   Updated: 2026/06/13 18:22:11 by sael-kha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include "webServer.hpp"

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: ./webserv <configuration_file>" << std::endl;
        return (1);
    }
    try
    {
        WebServer ws(argv[1]);
        ws.run();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}