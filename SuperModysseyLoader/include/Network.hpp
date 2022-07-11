#pragma once
#include <string>
#include <curl/curl.h>
#include <switch.h>
#include <arpa/inet.h>

namespace net {
    std::string RetrieveContent(const std::string &url, const std::string &mime_type = "");
    void RetrieveToFile(const std::string &url, const std::string &path);
    bool HasConnection();
}
