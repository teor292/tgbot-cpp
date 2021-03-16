#ifndef TGBOT_BOOSTHTTPCLIENTALIVE_H
#define TGBOT_BOOSTHTTPCLIENTALIVE_H

#include "tgbot/net/HttpClient.h"
#include "tgbot/net/Url.h"
#include "tgbot/net/HttpReqArg.h"
#include "tgbot/net/HttpParser.h"
#include <memory>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <string>
#include <vector>

namespace TgBot 
{
    using namespace boost::asio;
    using namespace boost::asio::ip;
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
    /**
     * @brief This class makes http requests via boost::asio and keep-alive connection
     *
     * @ingroup net
     */
    class TGBOT_API BoostHttpOnlySslClientAlive : public HttpClient {

    public:
        BoostHttpOnlySslClientAlive();
        ~BoostHttpOnlySslClientAlive() override;

        /**
         * @brief Sends a request to the url.
         *
         * If there's no args specified, a GET request will be sent, otherwise a POST request will be sent.
         * If at least 1 arg is marked as file, the content type of a request will be multipart/form-data, otherwise it will be application/x-www-form-urlencoded.
         */
        std::string makeRequest(const Url& url, const std::vector<HttpReqArg>& args) const override;

    private:
        const HttpParser _httpParser;

        mutable SocketSslData socket_data_;

        void start_work_socket_(const std::string& host) const;

        void end_work_socket_(bool close_socket) const;
    };

}

#endif //TGBOT_BOOSTHTTPCLIENTALIVE_H
