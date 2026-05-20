/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Exceptions.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mben-cha <mben-cha@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/02 21:51:54 by mben-cha          #+#    #+#             */
/*   Updated: 2026/05/17 21:57:06 by mben-cha         ###   ########.fr       */
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