#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "Client.hpp"
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <string>

// class dyal response -> kibni HTTP response men request li tvalida
// kireuse checker.root, checker.is_a_dir, server/location ou config

class Client;

class Response
{
    public:
        Client*     client;     // client li 3andna n3awnoh nbniw response dyalo

        Response(Client& client);

        // entry point: kidispatchi 3la method ou kisali response f client->response
        void    build(void);

    private:
        // ===== method handlers =====
        void    handle_get(void);
        void    handle_post(void);
        void    handle_delete(void);

        // ===== static serving helpers =====
        void    serve_file(const std::string& path);
        void    serve_directory(void);
        void    generate_autoindex(const std::string& path);

        // ===== config helpers (kireuse getDirective dyal teammates) =====
        std::string get_index_path(void);
        std::string get_upload_path(void);
        bool        autoindex_enabled(void);

        // ===== low level helpers =====
        bool        file_exists(const std::string& path, bool& is_dir);
        std::string get_mime_type(const std::string& path);
        std::string build_response(int code, const std::string& content_type, const std::string& body);

        void        handleMultipartUpload();
        bool        isMultipartRequest() const;
};

#endif
