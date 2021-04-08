#ifndef TGBOT_BOOSTHTTPCLIENTALIVE_H
#define TGBOT_BOOSTHTTPCLIENTALIVE_H

#include "tgbot/net/HttpClient.h"
#include "tgbot/net/Url.h"
#include "tgbot/net/HttpReqArg.h"
#include "tgbot/net/HttpParser.h"
#include <memory>



#include <string>
#include <vector>

namespace TgBot 
{
    class SocketSslData;
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

        std::unique_ptr<SocketSslData> socket_data_;

        void start_work_socket_(const std::string& host) const;

        void end_work_socket_(bool close_socket) const;

    };

}

#endif //TGBOT_BOOSTHTTPCLIENTALIVE_H
