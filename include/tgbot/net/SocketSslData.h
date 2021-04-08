#pragma once


#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <memory>
#include "tgbot/export.h"

using namespace boost::asio;
using namespace boost::asio::ip;

namespace TgBot {

    /**
     * @brief This is helper class for BoostHttpOnlySslClientAlive class
     *
     * @ingroup net
     */
    class TGBOT_API SocketSslData
    {
    public:

        boost::asio::io_service& service()
        {
            return service_;
        }

        ssl::stream<tcp::socket>& sock()
        {
            return *sock_ptr_;
        }

        bool empty() const
        {
            return nullptr == sock_ptr_;
        }

        bool same_host(const std::string& host) const
        {
            return socket_host_ == host;
        }

        void reset()
        {
            sock_ptr_.reset();
            ctx_ptr_.reset();
        }

        /**
         * @brief Must be called when start work with Telegram server
         * 
         * @param host - host address
         * 
         * @details Close previous socket if error occurred 
         * and open a new one. 
         * If no error -> continue work on same socket
         */
        void start_work(const std::string& host);

        /**
         * @brief Must be called when work with server successfully finished
         * 
         * @param close_socket - if true then close socket
         * (in case when 'connection' http header field doesn't contain 'keep-alive')
         */
        void end_work(bool close_socket);

    protected:

        bool start_work_{ false };

        boost::asio::io_service service_;
        std::unique_ptr<ssl::context> ctx_ptr_;
        std::unique_ptr<ssl::stream<tcp::socket>> sock_ptr_;
        std::string socket_host_;

    };
}