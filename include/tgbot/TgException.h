#ifndef TGBOT_TGEXCEPTION_H
#define TGBOT_TGEXCEPTION_H

#include "tgbot/export.h"

#include <string>
#include <stdexcept>

namespace TgBot {

/**
 * @brief Exception type which is only thrown when Telegram refuses API request.
 *
 * @ingroup general
 */
class TGBOT_API TgException : public std::runtime_error {

public:
    explicit TgException(const std::string& description);
};

class TGBOT_API TgBlockedException : public TgException {
public:
    using TgException::TgException;
};

class TGBOT_API TgGateWaitTimeoutException : public TgException {
public:
    using TgException::TgException;
};

}

#endif //TGBOT_TGEXCEPTION_H
