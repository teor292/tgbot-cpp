#include "tgbot/net/HttpParser.h"

#include "tgbot/tools/StringTools.h"

#include <boost/algorithm/string.hpp>
#include <boost/utility/string_view.hpp>

#include <cstddef>
#include <string>
#include <vector>
#include <unordered_map>

using namespace std;
using namespace boost;

namespace TgBot {

    namespace
    {
        const std::string GET = "GET ";
        const std::string POST = "POST ";
        const std::string HTTP1_1 = " HTTP/1.1\r\n";
        const std::string HOST = "Host: ";
        const std::string CONNECTION = "\r\nConnection: ";
        const std::string KEEP_ALIVE = "keep-alive";
        const std::string CLOSE = "close";
        const std::string NEW_LINE = "\r\n";
        const std::string ENCODED_URL_CONTENT = "Content-Type: application/x-www-form-urlencoded\r\n";
        const std::string MULTIPART_CONTENT = "Content-Type: multipart/form-data; boundary=";
        const std::string CONTENT_LENGTH = "Content-Length: ";
        const std::string CONTENT_DISPOSITION = "\r\nContent-Disposition: form-data; name=\"";
        const std::string FILE_NAME = "\"; filename=\"";
        const std::string CONTENT_TYPE = "Content-Type: ";
    }

    size_t HttpParser::calculate_approximate_buffer_size(const Url& url, 
        const std::vector<HttpReqArg>& args) const
    {
        size_t result = 0;
        if (args.empty()) {
            result += GET.size();
        }
        else {
            result += POST.size();
        }
        result += url.path.size();
        result += url.query.empty() ? 0 : 1 + url.query.size();
        result += HTTP1_1.size();
        result += HOST.size();
        result += url.host.size();
        result += CONNECTION.size();

        result += KEEP_ALIVE.size();

        result += NEW_LINE.size();
        if (args.empty()) {
            result += NEW_LINE.size();
        }
        else {


            result += MULTIPART_CONTENT.size() + 100;
            result += NEW_LINE.size();
            result += calculate_approximate_multipart_size(args);

            result += CONTENT_LENGTH.size();
            result += 8;
            result += NEW_LINE.size() * 2;
        }
        if (result < 1024 * 10)
        {
            return result * 2;
        }
        return result + 1024 * 10; //+ 10КБ;
    }

    size_t HttpParser::calculate_approximate_multipart_size(const std::vector<HttpReqArg>& args) const
    {
        size_t result = 0;
        for (const HttpReqArg& item : args) {
            result += 102;
            result += CONTENT_DISPOSITION.size();
            result += item.name.size();
            if (item.isFile) {
                result += FILE_NAME.size() + item.fileName.size();
            }
            result += 1 + NEW_LINE.size();
            if (item.isFile) {
                result += CONTENT_TYPE.size();
                result += item.mimeType.size();
                result += NEW_LINE.size();
            }
            result += NEW_LINE.size();
            if (item.value.which() == 0)
            {
                auto& value = boost::get<std::shared_ptr<std::vector<std::uint8_t>>>(item.value);

                result += value->size();

            }
            else
            {
                result += boost::get<std::string>(item.value).size();
            }

            result += NEW_LINE.size();
        }
        result += 24 + NEW_LINE.size();
        return result;
    }

    size_t HttpParser::calculate_exactly_multipart_size(const std::vector<HttpReqArg>& args, 
        const std::string& bondary) const
    {
        size_t result = 0;
        for (const HttpReqArg& item : args) {
            result += 2;
            result += bondary.size();
            result += CONTENT_DISPOSITION.size();
            result += item.name.size();
            if (item.isFile) {
                result += FILE_NAME.size() + item.fileName.size();
            }
            result += 1 + NEW_LINE.size();
            if (item.isFile) {
                result += CONTENT_TYPE.size();
                result += item.mimeType.size();
                result += NEW_LINE.size();
            }
            result += NEW_LINE.size();
            if (item.value.which() == 0)
            {
                auto& value = boost::get<std::shared_ptr<std::vector<std::uint8_t>>>(item.value);

                result += value->size();
            }
            else
            {
                result += boost::get<std::string>(item.value).size();
            }

            result += NEW_LINE.size();
        }
        result += 2 + bondary.size() + 2 + NEW_LINE.size();
        return result;
    }

string HttpParser::generateRequest(const Url& url, const vector<HttpReqArg>& args, bool isKeepAlive) const {

    size_t approx_buffer =  calculate_approximate_buffer_size(url, args);
    string result;
    result.reserve(approx_buffer);
    if (args.empty()) {
        result += "GET ";
    } else {
        result += "POST ";
    }
    result += url.path;
    result += url.query.empty() ? "" : "?" + url.query;
    result += " HTTP/1.1\r\n";
    result += "Host: ";
    result += url.host;
    result += "\r\nConnection: ";
    if (isKeepAlive) {
        result += "keep-alive";
    } else {
        result += "close";
    }
    result += "\r\n";
    if (args.empty()) {
        result += "\r\n";
    } else {

        string bondary = generateMultipartBoundary(args);
        if (bondary.empty()) {
            result += "Content-Type: application/x-www-form-urlencoded\r\n";
            string requestData = generateWwwFormUrlencoded(args);
            result += "Content-Length: ";
            result += std::to_string(requestData.length());
            result += "\r\n\r\n";
            result += requestData;
        } else {

            result += "Content-Type: multipart/form-data; boundary=";
            result += bondary;
            result += "\r\n";
            auto requestSize = calculate_exactly_multipart_size(args, bondary);
            result += "Content-Length: ";
            result += std::to_string(requestSize);
            result += "\r\n\r\n";
            auto resultSize = generateMultipartFormData(args, bondary, result);
            if (requestSize != resultSize)
                throw std::logic_error("requestSize != resultSize");
        }


    }
    return result;
}

size_t HttpParser::generateMultipartFormData(const vector<HttpReqArg>& args, const string& bondary, std::string& result) const {

    size_t old_size = result.size();

    for (const HttpReqArg& item : args) {
        result += "--";
        result += bondary;
        result += "\r\nContent-Disposition: form-data; name=\"";
        result += item.name;
        if (item.isFile) {
            result += "\"; filename=\"" + item.fileName;
        }
        result += "\"\r\n";
        if (item.isFile) {
            result += "Content-Type: ";
            result += item.mimeType;
            result += "\r\n";
        }
        result += "\r\n";
        if (item.value.which() == 0)
        {
            auto& value = boost::get<std::shared_ptr<std::vector<unsigned char>>>(item.value);
            //result += reinterpret_cast<char*>(
            //    boost::get<std::shared_ptr<std::vector<unsigned char>>>(item.value)->data());
            result.append(reinterpret_cast<char*>(value->data()),
                value->size());
        }
        else
        {
            result += boost::get<std::string>(item.value);
        }

        result += "\r\n";
    }
    result += "--" + bondary + "--\r\n";
    return result.size() - old_size;
}

string HttpParser::generateMultipartBoundary(const vector<HttpReqArg>& args) const {
    string result;
    for (const HttpReqArg& item : args) {
        if (item.isFile) {
            auto& file_data = boost::get<std::shared_ptr<std::vector<unsigned char>>>(item.value);
            boost::string_view value(reinterpret_cast<char*>(file_data->data()), file_data->size());

            //while (result.empty() || item.value.find(result) != string::npos) {
            //    result += StringTools::generateRandomString(4);
            //}
            while (result.empty() || value.find(result) != string::npos) {
                result += StringTools::generateRandomString(4);
            }
        }
    }
    return result;
}

string HttpParser::generateWwwFormUrlencoded(const vector<HttpReqArg>& args) const {
    string result;

    bool firstRun = true;
    for (const HttpReqArg& item : args) {
        if (firstRun) {
            firstRun = false;
        } else {
            result += '&';
        }
        result += StringTools::urlEncode(item.name);
        result += '=';
        if (item.value.which() == 0)
        {
            throw std::logic_error("generateWwwFormUrlencoded but has file");
            //auto& value = boost::get<std::shared_ptr<std::vector<unsigned char>>>(item.value);
            //result += StringTools::urlEncode(std::string(reinterpret_cast<char*>(
            //    value->data()), value->size()));
        }
        else
        {
            result += StringTools::urlEncode(boost::get<std::string>(item.value));
        }

    }

    return result;
}

string HttpParser::generateResponse(const string& data, const string& mimeType, unsigned short statusCode, const string& statusStr, bool isKeepAlive) const {
    string result;
    result += "HTTP/1.1 ";
    result += std::to_string(statusCode);
    result += ' ';
    result += statusStr;
    result += "\r\nContent-Type: ";
    result += mimeType;
    result += "\r\nContent-Length: ";
    result += std::to_string(data.length());
    result += "\r\nConnection: ";
    if (isKeepAlive) {
        result += "keep-alive";
    } else {
        result += "close";
    }
    result += "\r\n\r\n";
    result += data;
    return result;
}

unordered_map<string, string> HttpParser::parseHeader(const string& data, bool isRequest) const {
    unordered_map<string, string> headers;

    std::size_t lineStart = 0;
    std::size_t lineEnd = 0;
    std::size_t lineSepPos = 0;
    std::size_t lastLineEnd = string::npos;
    while (lastLineEnd != lineEnd) {
        lastLineEnd = lineEnd;
        bool isFirstLine = lineEnd == 0;
        if (isFirstLine) {
            if (isRequest) {
                lineSepPos = data.find(' ');
                lineEnd = data.find("\r\n");
                headers["_method"] = data.substr(0, lineSepPos);
                headers["_path"] = data.substr(lineSepPos + 1, data.find(' ', lineSepPos + 1) - lineSepPos - 1);
            } else {
                lineSepPos = data.find(' ');
                lineEnd = data.find("\r\n");
                headers["_status"] = data.substr(lineSepPos + 1, data.find(' ', lineSepPos + 1) - lineSepPos - 1);
            }
        } else {
            lineStart = lineEnd;
            lineStart += 2;
            lineEnd = data.find("\r\n", lineStart);
            lineSepPos = data.find(':', lineStart);
            if (lastLineEnd == lineEnd || lineEnd == string::npos) {
                break;
            }
            headers[data.substr(lineStart, lineSepPos - lineStart)] = trim_copy(data.substr(lineSepPos + 1, lineEnd - lineSepPos - 1));
        }
    }

    return headers;
}

string HttpParser::extractBody(const string& data) const {
    std::size_t headerEnd = data.find("\r\n\r\n");
    if (headerEnd == string::npos) {
        return data;
    }
    headerEnd += 4;
    return data.substr(headerEnd);
}

}
