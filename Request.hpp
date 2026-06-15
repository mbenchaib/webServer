#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <cstdlib>

enum ChunkState {
    CHUNK_SIZE,
    CHUNK_DATA,
    CHUNK_CRLF,
    CHUNK_DONE,
    CHUNK_ERROR
};

class Request
{
    public:
        int                                 error_code;

        std::string                         method;
        std::string                         path;
        std::string                         query_string;
        std::string                         version;
        std::string                         host;
        std::string                         body;
        std::string                         content_type;
        unsigned long                       body_len;

        bool                                valid;
        bool                                is_chunked;
        
        ChunkState                          chunk_state;
        size_t                              current_chunk_size;

        std::map<std::string, std::string>  headers;

        Request(void) : body_len(0), valid(true), is_chunked(false),
            chunk_state(CHUNK_SIZE), current_chunk_size(0), error_code(0) {};
        
        void    print(void);
        void    check_body_len(void);
        void    parse_request(const std::string& raw);
        void    check_first_line(std::stringstream& first);
        void    parse_chunked_body(std::string& raw_buffer);
};

#endif