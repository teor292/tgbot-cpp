#include "tgbot/net/BoostHttpOnlySslClientAlive.h"
#include "tgbot/net/SocketSslData.h"
#include "tgbot/tools/BoostTools.h"

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/optional.hpp>

#include <cstddef>
#include <vector>
#include <exception>

using namespace std;
using namespace boost::asio;
using namespace boost::asio::ip;
using namespace BoostTools;

namespace TgBot 
{
  
     BoostHttpOnlySslClientAlive::BoostHttpOnlySslClientAlive() : _httpParser(),
         socket_data_(new SocketSslData())
     {}

    BoostHttpOnlySslClientAlive::~BoostHttpOnlySslClientAlive() {}

    string BoostHttpOnlySslClientAlive::makeRequest(const Url& url, const vector<HttpReqArg>& args) const
    {
        const static char DELIMETER[] = "\r\n\r\n";
        const static uint32_t HEADER_TIMEOUT = 5; //5 seconds timeout to get HTTP header
        const static uint32_t DATA_TIMEOUT = 10; //10 seconds timeout to get HTTP data

        start_work_socket_(url.host);
        string requestText = _httpParser.generateRequest(url, args, true);

        boost::asio::streambuf b(1024 * 1024);



        boost::system::error_code error;

        write(socket_data_->sock(), buffer(requestText.c_str(), requestText.length()), error);

        if (error)
        {
            //try reopen socket
            start_work_socket_(url.host);

            //this throw exception if error
            write(socket_data_->sock(), buffer(requestText.c_str(), requestText.length()));
        }

        read_until_timeout(socket_data_->service(), socket_data_->sock(), b, DELIMETER, HEADER_TIMEOUT, error);

        if (error)
        {
            //in some cases it is possible to write to socket but not read from it
            //(if "Connection: close" in http and we continue use socket), and I don't know the
            //behaviour of the server: Is it close connection or just stop send response messages?
            //so try reopen socket and do write and read again
            start_work_socket_(url.host);

            //throw exception if error
            write(socket_data_->sock(), buffer(requestText.c_str(), requestText.length()));

            //remove characters that read_until_timeout() write to buffer
            b.consume(b.size() + 1);

            //throw exception if error
            read_until_timeout(socket_data_->service(), socket_data_->sock(), b, DELIMETER, HEADER_TIMEOUT);
        }

        std::istream is(&b);
        std::string line;
        uint16_t content_length = 0;
        bool close_socket = true;
        bool found_connection = false;
        while ((0 == content_length || !found_connection) && std::getline(is, line))
        { 
            const static char CONTENT_LENGTH[] = "content-length:";
            const static char CONNECTION[] = "connection:";
            const static char KEEP_ALIVE[] = "keep-alive";
            boost::to_lower(line);

            if (0 == content_length)
            {
                auto pos = line.find(CONTENT_LENGTH);

                if (std::string::npos != pos)
                {
                    auto size_str = line.substr(pos + sizeof(CONTENT_LENGTH));
                    boost::trim(size_str);
                    content_length = boost::lexical_cast<uint16_t>(size_str);
                    continue;
                }
            }
            if (!found_connection)
            {
                auto pos = line.find(CONNECTION);
                if (std::string::npos != pos)
                {
                    found_connection = true;
                    pos = line.find(KEEP_ALIVE, pos + sizeof(CONNECTION));
                    close_socket = std::string::npos == pos;
                }
            }

        }

        if (0 == content_length)
        {
            throw std::logic_error("Content-Length not found in header or it is 0");
        }


        std::string remain{ (std::istreambuf_iterator<char>(&b)), std::istreambuf_iterator<char>() };

        auto delimeter_pos = remain.find(DELIMETER);
        if (std::string::npos == delimeter_pos)
            throw std::logic_error("std::string::npos == delimeter_pos"); //must never happened

        auto response = remain.substr(delimeter_pos + sizeof(DELIMETER) - 1);

        if (response.size() < content_length)
        {
            read_timeout(socket_data_->service(),
                socket_data_->sock(),
                b,
                boost::asio::transfer_exactly(content_length - response.size()),
                DATA_TIMEOUT);

            response += std::string{ (std::istreambuf_iterator<char>(&b)), std::istreambuf_iterator<char>() };
        }

        end_work_socket_(close_socket);

        //can occurred when server send some invalid data and
        //read_until_timeout read a lot of data
        if (response.size() > content_length) 
        {
            response.resize(content_length);
        }
        
        return response;
    }

    void BoostHttpOnlySslClientAlive::start_work_socket_(const std::string& host) const
    {
        socket_data_->start_work(host);
    }

    void BoostHttpOnlySslClientAlive::end_work_socket_(bool close_socket) const
    {
        socket_data_->end_work(close_socket);
    }



}
