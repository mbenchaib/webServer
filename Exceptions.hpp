/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Exceptions.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mben-cha <mben-cha@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/29 19:10:46 by mben-cha          #+#    #+#             */
/*   Updated: 2026/05/19 16:51:16 by mben-cha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <exception>
#include <string>

class FileOpenException : public std::exception
{
public:
    const char *what() const throw();
};

class EmptyConfigException : public std::exception
{
public:
    const char *what() const throw();
};

class ConfigSyntaxError : public std::exception
{
    std::string error_message;
    
public:
    ConfigSyntaxError(std::string msg) : error_message(msg) {}
    ~ConfigSyntaxError() throw() {}
    const char *what() const throw();
};

class ConfigValidationError : public std::exception
{
    std::string error_message;
    
public:
    ConfigValidationError(std::string msg) : error_message(msg) {}
    ~ConfigValidationError() throw() {}
    const char *what() const throw();
};

class SocketSetupError : public std::exception
{
    std::string error_message;
    
public:
    SocketSetupError(std::string msg) : error_message(msg) {}
    ~SocketSetupError() throw() {}
    const char *what() const throw();
};

class NoListenSocketException : public std::exception
{
    std::string error_message;
    
public:
    NoListenSocketException(std::string msg) : error_message(msg) {}
    ~NoListenSocketException() throw() {}
    const char *what() const throw();
};