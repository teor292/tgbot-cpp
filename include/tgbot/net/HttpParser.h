#ifndef TGBOT_HTTPPARSER_H
#define TGBOT_HTTPPARSER_H

#include "tgbot/net/Url.h"
#include "tgbot/net/HttpReqArg.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace TgBot {

class TGBOT_API HttpParser {

public:
    std::string generateRequest(const Url& url, const std::vector<HttpReqArg>& args, bool isKeepAlive = false) const;
    size_t generateMultipartFormData(const std::vector<HttpReqArg>& args, const std::string& boundary, std::string& result) const;
    std::string generateMultipartBoundary(const std::vector<HttpReqArg>& args) const;
    std::string generateWwwFormUrlencoded(const std::vector<HttpReqArg>& args) const;
    std::string generateResponse(const std::string& data, const std::string& mimeType, unsigned short statusCode, const std::string& statusStr, bool isKeepAlive) const;
    std::unordered_map<std::string, std::string> parseHeader(const std::string& data, bool isRequest) const;
    std::string extractBody(const std::string& data) const;
    size_t calculate_approximate_buffer_size(const Url& url, const std::vector<HttpReqArg>& args) const;
    size_t calculate_approximate_multipart_size(const std::vector<HttpReqArg>& args) const;
    size_t calculate_exactly_multipart_size(const std::vector<HttpReqArg>& args, const std::string& bondary) const;
};

}

#endif //TGBOT_HTTPPARSER_H
