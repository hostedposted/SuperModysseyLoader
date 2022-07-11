#include <Network.hpp>

#define USERAGENT "SuperModysseyLoader"

namespace net {

    namespace {
        std::size_t StringWriteImpl(const char* in, std::size_t size, std::size_t count, std::string *out) {
            const auto total_size = size * count;
            out->append(in, total_size);
            return total_size;
        }

        std::size_t FileWriteImpl(const char* in, std::size_t size, std::size_t count, FILE *out) {
            fwrite(in, size, count, out);
            return size * count;
        }
    }

    std::string RetrieveContent(const std::string &url, const std::string &mime_type) {
        const auto rc = socketInitializeDefault();
        if (R_FAILED(rc)) {
            return "";
        }

        std::string content;
        auto curl = curl_easy_init();
        if(!mime_type.empty()) {
            curl_slist *header_data = curl_slist_append(header_data, ("Content-Type: " + mime_type).c_str());
            header_data = curl_slist_append(header_data, ("Accept: " + mime_type).c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_data);
        }
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERAGENT, USERAGENT);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, StringWriteImpl);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        socketExit();
        return content;
    }

    void RetrieveToFile(const std::string &url, const std::string &path) {
        const auto rc = socketInitializeDefault();
        if(R_FAILED(rc)) {
            return;
        }

        auto f = fopen(path.c_str(), "wb");
        if(f) {
            auto curl = curl_easy_init();
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_USERAGENT, USERAGENT);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, FileWriteImpl);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, f);
            curl_easy_perform(curl);
            curl_easy_cleanup(curl);
        }
        fclose(f);
        socketExit();
    }
    
    bool HasConnection() {
        return gethostid() == INADDR_LOOPBACK;
    }
}