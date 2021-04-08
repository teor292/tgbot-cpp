#include "tgbot/net/SocketSslData.h"

namespace TgBot {

    void SocketSslData::start_work(const std::string& host)
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
        socket_host_ = host;

        tcp::resolver resolver(service_);
        tcp::resolver::query query(host, "443");

        ctx_ptr_.reset(new ssl::context(ssl::context::tlsv12_client));

        ctx_ptr_->set_default_verify_paths();
        sock_ptr_.reset(new ssl::stream<tcp::socket>(service_, *ctx_ptr_));

        connect(sock_ptr_->lowest_layer(), resolver.resolve(query));

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
        sock_ptr_->set_verify_mode(ssl::verify_none);
        sock_ptr_->set_verify_callback(ssl::rfc2818_verification(host));

        sock_ptr_->handshake(ssl::stream<tcp::socket>::client);

    }

    void SocketSslData::end_work(bool close_socket)
    {
        start_work_ = false;
        if (close_socket)
        {
            reset();
        }
    }
}