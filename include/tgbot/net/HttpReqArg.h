#ifndef TGBOT_HTTPPARAMETER_H
#define TGBOT_HTTPPARAMETER_H

#include "tgbot/export.h"

#include <boost/lexical_cast.hpp>
#include <boost/variant.hpp>
#include "tgbot/types/InputFile.h"
#include "tgbot/types/InputMedia.h"
#include <string>
#include <vector>
#include <functional>

namespace TgBot {

/**
 * @brief This class represents argument in POST http requests.
 *
 * @ingroup net
 */
class TGBOT_API HttpReqArg {

public:
    template<typename T>
    HttpReqArg(std::string name, const T& value, bool isFile = false, std::string mimeType = "text/plain", std::string fileName = "") :
            name(std::move(name)), value(boost::lexical_cast<std::string>(value)), isFile(isFile), mimeType(std::move(mimeType)), fileName(std::move(fileName))
    {
    }

    HttpReqArg(std::string name, InputFile::Ptr& image) :
        name(std::move(name)), value(image->data), isFile(true), mimeType(image->mimeType), fileName(image->fileName)
    {
    } 

    HttpReqArg(const InputMedia::Ptr& media ) 
    {
        if (media->media.which() != 0)
            throw std::logic_error("HttpReqArg can take only local media");

        auto& tmp = boost::get<LocalMedia>(media->media);
        name = tmp.fileName;
        value = tmp.data;
        isFile = true;
        mimeType = tmp.mimeType;
        fileName = tmp.fileName;
    }
    
    /**
     * @brief Name of an argument.
     */
    std::string name;

    /**
     * @brief Value of an argument.
     */
    boost::variant<std::shared_ptr<std::vector<std::uint8_t>>, std::string> value;
    //std::string value;

    /**
     * @brief Should be true if an argument value hold some file contents
     */
    bool isFile = false;

    /**
     * @brief Mime type of an argument value. This field makes sense only if isFile is true.
     */
    std::string mimeType = "text/plain";

    /**
     * @brief Should be set if an argument value hold some file contents
     */
    std::string fileName;
};

}


#endif //TGBOT_HTTPPARAMETER_H
