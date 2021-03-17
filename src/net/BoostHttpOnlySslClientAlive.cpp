#include "tgbot/net/BoostHttpOnlySslClientAlive.h"


#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <cstddef>
#include <vector>
#include <exception>

using namespace std;
using namespace boost::asio;
using namespace boost::asio::ip;

namespace TgBot 
{
    class SocketSslData
    {
    public:

        boost::asio::io_service io_service;
        std::unique_ptr<ssl::context> ctx_ptr;
        std::unique_ptr<ssl::stream<tcp::socket>> sock_ptr;
        std::string socket_host;

        bool empty() const
        {
            return nullptr == sock_ptr;
        }

        bool same_host(const std::string& host) const
        {
            return socket_host == host;
        }

        void reset()
        {
            sock_ptr.reset();
            ctx_ptr.reset();
        }

        void start_work(const std::string& host)
        {
            //if start_work is true here => some error occurred => close socket
            if (!empty() && same_host(host) && !start_work_)
            {
                start_work_ = true;
                return;
            }
            reset();

            //assign start_work_ here because if exception during connect
            //=> reset socket on next start_work() call
            start_work_ = true;
            socket_host = host;

            tcp::resolver resolver(io_service);
            tcp::resolver::query query(host, "443");

            ctx_ptr.reset(new ssl::context(ssl::context::tlsv12_client));

            ctx_ptr->set_default_verify_paths();
            sock_ptr.reset(new ssl::stream<tcp::socket>(io_service, *ctx_ptr));

            connect(sock_ptr->lowest_layer(), resolver.resolve(query));

#ifdef TGBOT_DISABLE_NAGLES_ALGORITHM
            sock_ptr->lowest_layer().set_option(tcp::no_delay(true));
#endif //TGBOT_DISABLE_NAGLES_ALGORITHM
#ifdef TGBOT_CHANGE_SOCKET_BUFFER_SIZE
#if _WIN64 || __amd64__ || __x86_64__ || __MINGW64__ || __aarch64__ || __powerpc64__
            sock_ptr->lowest_layer().set_option(socket_base::send_buffer_size(65536));
            sock_ptr->lowest_layer().set_option(socket_base::receive_buffer_size(65536));
#else //for 32-bit
            sock_ptr->lowest_layer().set_option(socket_base::send_buffer_size(32768));
            sock_ptr->lowest_layer().set_option(socket_base::receive_buffer_size(32768));
#endif //Processor architecture
#endif //TGBOT_CHANGE_SOCKET_BUFFER_SIZE
            sock_ptr->set_verify_mode(ssl::verify_none);
            sock_ptr->set_verify_callback(ssl::rfc2818_verification(host));

            sock_ptr->handshake(ssl::stream<tcp::socket>::client);

        }

        void end_work(bool close_socket)
        {
            start_work_ = false;
            if (close_socket)
            {
                reset();
            }
        }

    private:

        bool start_work_{ false };
    };

     BoostHttpOnlySslClientAlive::BoostHttpOnlySslClientAlive() : _httpParser(),
         socket_data_(new SocketSslData())
     {}

    BoostHttpOnlySslClientAlive::~BoostHttpOnlySslClientAlive() {}

    string BoostHttpOnlySslClientAlive::makeRequest(const Url& url, const vector<HttpReqArg>& args) const
    {
        start_work_socket_(url.host);
        string requestText = _httpParser.generateRequest(url, args, true);

        boost::system::error_code error;

        write(*socket_data_->sock_ptr, buffer(requestText.c_str(), requestText.length()), error);

        if (error)
        {
            //try reopen socket
            start_work_socket_(url.host);

            write(*socket_data_->sock_ptr, buffer(requestText.c_str(), requestText.length()));
        }

        boost::asio::streambuf b(1024 * 1024);


        const static char DELIMETER[] = "\r\n\r\n";
        read_until(*socket_data_->sock_ptr, b, DELIMETER, error);

        if (error)
        {
            //in some cases it is possible to write to socket but not read from it
            //(if "Connection: close" in http and we continue use socket), and I don't know the
            //behaviour of server: Is it close connection or just stop send response messages?
            //so try reopen socket and do write and read again
            start_work_socket_(url.host);

            write(*socket_data_->sock_ptr, buffer(requestText.c_str(), requestText.length()));

            //remove characters that read_until() write to buffer
            b.consume(b.size() + 1);

            read_until(*socket_data_->sock_ptr, b, DELIMETER);
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
            throw std::logic_error("Content-Length not found in header");
        }


        std::string remain{ (std::istreambuf_iterator<char>(&b)), std::istreambuf_iterator<char>() };

        auto delimeter_pos = remain.find(DELIMETER);
        if (std::string::npos == delimeter_pos)
            throw std::logic_error("std::string::npos == delimeter_pos"); //must never happened

        auto response = remain.substr(delimeter_pos + sizeof(DELIMETER) - 1);

        if (response.size() > content_length)
            throw std::logic_error("response.size() > content_length"); //must never happened

        read(*socket_data_->sock_ptr, b, boost::asio::transfer_exactly(content_length - response.size()));

        end_work_socket_(close_socket);

        response += std::string{ (std::istreambuf_iterator<char>(&b)), std::istreambuf_iterator<char>() };
        
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
