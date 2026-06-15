/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Exceptions.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sael-kha <sael-kha@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/02 21:51:54 by mben-cha          #+#    #+#             */
/*   Updated: 2026/06/07 16:11:45 by sael-kha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Exceptions.hpp"

const char* ConfigSyntaxError::what() const throw()
{
    return (error_message.c_str());
}

const char* EmptyConfigException::what() const throw()
{
    return ("Error: Configuration file is empty");
}

const char* FileOpenException::what() const throw()
{
    return ("Error: Failed to open configuration file");
}

const char* ConfigValidationError::what() const throw()
{
    return (error_message.c_str());
}

const char* SocketSetupError::what() const throw()
{
    return (error_message.c_str());
}
const char* NoListenSocketException::what() const throw()
{
    return (error_message.c_str());
}