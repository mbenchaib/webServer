#ifndef REQUEST_HPP
#define REQUEST_HPP
#include <map>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
/*
    hada class dyal request
    kat3ayet nhad class hta katwsal n "\r\n\r\n"
    ou men ba3d ana kanjbedlek method ou bodylen bihom thaded wach hat9ra body awla
    check client dyali kifa ki9ra request men socket bach tfham
*/
class Request
{
    public:
        // THIS PART FOR FIRST LINE
        std::string                         method; // GET, DELETE, POST
        std::string                         path;   // PATH OF THE FILE CLIENT WANT IT
        std::string                         quere_string;// anything after ? in path
        std::string                         version;// HTTP1.1 OR HTTP1.0
        // THIS PART FOR IMPORTANT HEADERS
        std::string                         host;   // host request from client
        std::string                         body;   // CONTENT AFTER "\r\n\r\n"
        std::string                         content_type; // type dyall content li sared client
        unsigned long                       body_len;// TOUL DYAL BODY

        bool                                valid;  //  IN CASE OF UNSUPORTED METHOD OR UNSUPORTED HTTP VERSION OR BODY_LEN HAVE CHARS IN IT
        bool                                CGI;    //IF IT TRUE IT MY JOB IF IT NOT IT UR JOB
        std::map<std::string, std::string>  headers;
        Request(void): valid(true), body_len(0), CGI(false){};
        void    parse_request(const std::string& raw);
        void    check_first_line(std::stringstream& first);
        void    print(void);
};


#endif