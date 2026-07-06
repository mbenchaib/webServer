# webserv

*This project has been created as part of the 42 curriculum by sael-kha, mben-cha, roubelka.*

## Description

**webserv** is a simplified HTTP/1.0 web server developed in C++. The objective of the project is to understand how web servers operate by implementing the core networking and HTTP concepts without relying on external web server libraries.

The server supports multiple virtual servers through configuration files and is capable of handling multiple client connections simultaneously using I/O multiplexing with `poll()`. It processes incoming HTTP requests, maps them to server resources according to the configuration, generates appropriate HTTP responses, and supports features such as file serving, request parsing, CGI execution, error handling, and configurable server behavior.

This project provided practical experience with:

* TCP/IP socket programming
* HTTP request and response handling
* Event-driven server design
* Non-blocking I/O
* CGI execution
* Process management
* Configuration parsing
* File system interaction
* Error handling and robustness

---

## Instructions

### Requirements

* C++98 compatible compiler
* Make

### Compilation

Compile the project using:

```bash
make
```

To remove object files:

```bash
make clean
```

To remove all generated files:

```bash
make fclean
```

To rebuild the project:

```bash
make re
```

### Running the server

Launch the server by providing a configuration file:

```bash
./webserv config.config
```

Replace `config.config` with the desired configuration file if necessary.

### Testing

The server can be tested using tools such as:

* curl
* netcat (nc)
* Siege
* Web browsers

Example:

```bash
curl http://localhost:8080/
```

---

## Resources

### Documentation

The following resources were consulted during the development of this project:

* RFC 1945 — Hypertext Transfer Protocol HTTP/1.0
* RFC 7230 — Hypertext Transfer Protocol (Message Syntax and Routing)
* RFC 7231 — HTTP/1.1 Semantics and Content
* Linux/macOS manual pages (`man`)

  * socket
  * bind
  * listen
  * accept
  * poll
  * recv
  * send
  * fcntl
  * waitpid
  * execve
  * dup2
  * pipe
* Beej's Guide to Network Programming
* UNIX Network Programming, Volume 1: The Sockets Networking (book)
* HTTP: The Definitive Guide (book)
* cppreference.com

### Use of AI

ChatGPT was used as a learning and documentation assistant throughout the project. AI assistance included:

* Explaining networking concepts (TCP, sockets, HTTP, polling, CGI, process management).
* Clarifying the behavior of POSIX system calls and C++ standard library features.
* Discussing implementation strategies and software design choices.
* Assisting with debugging by explaining error messages and expected system behavior.
