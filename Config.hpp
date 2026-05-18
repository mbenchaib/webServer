#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "ConfigServer.hpp"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstdlib>
#include <iostream>


// hada jibto men chatgpt bach ndipani ou sf 3lamen ysali mohammed
class Config
{
private:
    std::vector<ConfigServer>   _servers;

    // ---- tokenizer ----
    // returns all tokens: words, '{', '}'
    std::vector<std::string> tokenize(const std::string &raw)
    {
        std::vector<std::string> tokens;
        std::string token;

        for (size_t i = 0; i < raw.size(); i++)
        {
            char c = raw[i];

            // skip comments
            if (c == '#')
            {
                while (i < raw.size() && raw[i] != '\n')
                    i++;
                continue;
            }

            if (c == '{' || c == '}')
            {
                if (!token.empty()) { tokens.push_back(token); token.clear(); }
                tokens.push_back(std::string(1, c));
            }
            else if (c == ';')
            {
                // semicolons end a directive — treat like whitespace
                if (!token.empty()) { tokens.push_back(token); token.clear(); }
            }
            else if (std::isspace(c))
            {
                if (!token.empty()) { tokens.push_back(token); token.clear(); }
            }
            else
                token += c;
        }
        if (!token.empty())
            tokens.push_back(token);
        return tokens;
    }

    // ---- parsing helpers ----
    std::string expect(const std::vector<std::string> &t, size_t &i, const std::string &what)
    {
        if (i >= t.size())
            throw std::runtime_error("Config: unexpected end, expected " + what);
        return t[i++];
    }

    void expectToken(const std::vector<std::string> &t, size_t &i, const std::string &val)
    {
        std::string got = expect(t, i, val);
        if (got != val)
            throw std::runtime_error("Config: expected '" + val + "' got '" + got + "'");
    }

    // ---- location parser ----
    ConfigLocation parseLocation(const std::vector<std::string> &t, size_t &i)
    {
        ConfigLocation loc;
        loc.path = expect(t, i, "location path");
        expectToken(t, i, "{");

        while (i < t.size() && t[i] != "}")
        {
            std::string key = t[i++];

            if (key == "methods")
            {
                while (i < t.size() && t[i] != "}" && t[i] != "root"
                    && t[i] != "index" && t[i] != "directory_listing"
                    && t[i] != "redirect" && t[i] != "upload_dir"
                    && t[i] != "cgi" && t[i] != "methods")
                    loc.methods.push_back(t[i++]);
            }
            else if (key == "root")
                loc.root = expect(t, i, "root path");
            else if (key == "index")
                loc.index = expect(t, i, "index file");
            else if (key == "upload_dir")
                loc.upload_dir = expect(t, i, "upload dir");
            else if (key == "directory_listing")
            {
                std::string val = expect(t, i, "on/off");
                loc.directory_listing = (val == "on");
            }
            else if (key == "redirect")
            {
                std::string code = expect(t, i, "redirect code");
                loc.redirect_code = std::atoi(code.c_str());
                loc.redirect_url  = expect(t, i, "redirect url");
            }
            else if (key == "cgi")
            {
                std::string ext = expect(t, i, "cgi extension");
                std::string bin = expect(t, i, "cgi binary");
                loc.cgi[ext] = bin;
            }
            else
                throw std::runtime_error("Config: unknown location key: " + key);
        }
        expectToken(t, i, "}");
        return loc;
    }

    // ---- server parser ----
    ConfigServer parseServer(const std::vector<std::string> &t, size_t &i)
    {
        ConfigServer srv;
        expectToken(t, i, "{");

        while (i < t.size() && t[i] != "}")
        {
            std::string key = t[i++];

            if (key == "listen")
                srv.port = std::atoi(expect(t, i, "port").c_str());
            else if (key == "host")
                srv.host = expect(t, i, "host");
            else if (key == "max_body_size")
                srv.max_body_size = std::atol(expect(t, i, "size").c_str());
            else if (key == "error_page")
            {
                int code = std::atoi(expect(t, i, "error code").c_str());
                srv.error_pages[code] = expect(t, i, "error page path");
            }
            else if (key == "location")
                srv.locations.push_back(parseLocation(t, i));
            else
                throw std::runtime_error("Config: unknown server key: " + key);
        }
        expectToken(t, i, "}");
        return srv;
    }

public:
    Config(const std::string &filename)
    {
        // read file
        std::ifstream file(filename.c_str());
        if (!file.is_open())
            throw std::runtime_error("Config: cannot open file: " + filename);

        std::ostringstream ss;
        ss << file.rdbuf();
        std::string raw = ss.str();

        // tokenize
        std::vector<std::string> tokens = tokenize(raw);

        // parse
        size_t i = 0;
        while (i < tokens.size())
        {
            std::string tok = tokens[i++];
            if (tok == "server")
                _servers.push_back(parseServer(tokens, i));
            else
                throw std::runtime_error("Config: expected 'server', got: " + tok);
        }

        if (_servers.empty())
            throw std::runtime_error("Config: no server block found");
    }

    std::vector<ConfigServer> &getServers() { return _servers; }
    ConfigServer&    Get_Server_By_Host(std::string  HOST)
    {
        for (int i = 0; i < _servers.size(); i++)
        {
            if (_servers[i].host == HOST)
                return _servers[i];
        }
        return _servers[0];
    }

    // debug dump
    void print() const
    {
        for (size_t i = 0; i < _servers.size(); i++)
        {
            const ConfigServer &s = _servers[i];
            std::cout << "Server " << i << ": " << s.host << ":" << s.port
                      << " max_body=" << s.max_body_size << "\n";
            for (size_t j = 0; j < s.locations.size(); j++)
            {
                const ConfigLocation &l = s.locations[j];
                std::cout << "  Location: " << l.path
                          << " root=" << l.root
                          << " index=" << l.index
                          << " listing=" << l.directory_listing << "\n";
            }
        }
    }
};

#endif