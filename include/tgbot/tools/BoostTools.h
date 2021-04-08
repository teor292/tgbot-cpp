#pragma once

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/optional.hpp>


namespace BoostTools
{
    template<typename AsyncReadStream, typename Allocator>
    inline size_t read_until_timeout(
        boost::asio::io_service& service,
        AsyncReadStream& stream,
        boost::asio::basic_streambuf<Allocator>& b,
        BOOST_ASIO_STRING_VIEW_PARAM DELIMETER,
        uint32_t seconds_timeout,
        boost::system::error_code& error)
    {

        boost::optional<boost::system::error_code> timer_result;

        boost::asio::deadline_timer timer(service);
        timer.expires_from_now(boost::posix_time::seconds(seconds_timeout));
        timer.async_wait([&timer_result](const boost::system::error_code& error)
            {
                timer_result.reset(error);
            });

        boost::optional<boost::system::error_code> read_result;
        std::size_t readed_size = 0;

        boost::asio::async_read_until(stream, b, DELIMETER,
            [&read_result, &readed_size](const boost::system::error_code& e, std::size_t size)
            {
                readed_size = size;
                read_result.reset(e);
            });

        service.restart();
        while (service.run_one())
        {
            if (read_result)
            {
                timer.cancel();
            }
            else if (timer_result)
            {
                stream.lowest_layer().cancel();
            }
        }

        if (!read_result.has_value()) //must not occurred
            throw std::logic_error("read_result has no value!!!");

        error = *read_result;

        return readed_size;
    }

    template<typename AsyncReadStream, typename Allocator>
    inline size_t read_until_timeout(
        boost::asio::io_service& service,
        AsyncReadStream& stream,
        boost::asio::basic_streambuf<Allocator>& b,
        BOOST_ASIO_STRING_VIEW_PARAM DELIMETER,
        uint32_t seconds_timeout)
    {
        boost::system::error_code error;
        auto result = read_until_timeout(service, stream, b, DELIMETER, seconds_timeout, error);

        if (error)
            throw boost::system::system_error(error);

        return result;
    }

    template <typename AsyncReadStream, typename Allocator,
        typename CompletionCondition>
    inline std::size_t read_timeout(
        boost::asio::io_service& service, 
        AsyncReadStream& stream,
        boost::asio::basic_streambuf<Allocator>& b,
        CompletionCondition completion_condition,
        uint32_t seconds_timeout,
        boost::system::error_code& error)
    {

        boost::optional<boost::system::error_code> timer_result;

        boost::asio::deadline_timer timer(service);
        timer.expires_from_now(boost::posix_time::seconds(seconds_timeout));
        timer.async_wait([&timer_result](const boost::system::error_code& error)
            {
                timer_result.reset(error);
            });

        boost::optional<boost::system::error_code> read_result;
        std::size_t readed_size = 0;

        boost::asio::async_read(stream, b, completion_condition,
            [&read_result, &readed_size](const boost::system::error_code& e, std::size_t size)
            {
                readed_size = size;
                read_result.reset(e);
            });


        service.restart();
        while (service.run_one())
        {
            if (read_result)
            {
                timer.cancel();
            }
            else if (timer_result)
            {
                stream.lowest_layer().cancel();
            }
        }

        if (!read_result.has_value()) //must not occurred
            throw std::logic_error("read_result has no value!!!");

        error = *read_result;

        return readed_size;
    }

    template <typename AsyncReadStream, typename Allocator,
        typename CompletionCondition>

        inline std::size_t read_timeout(
            boost::asio::io_service& service, 
            AsyncReadStream& s,
            boost::asio::basic_streambuf<Allocator>& b,
            CompletionCondition completion_condition,
            uint32_t seconds_timeout)
    {
        boost::system::error_code error;
        auto result = read_timeout(service, s, b, completion_condition, seconds_timeout, error);

        if (error)
            throw boost::system::system_error(error);

        return result;
    }
}