cmake_minimum_required(VERSION 2.8.4)
project(TgBot)

# options
option(ENABLE_TESTS "Set to ON to enable building of tests" OFF)
option(BUILD_SHARED_LIBS "Build tgbot-cpp shared/static library" OFF)
option(CONAN_BUILD "Use Conan for 3rd-party libraries" OFF)

# sources
if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")
endif()

include_directories(include)
set(SRC_LIST
    src/Api.cpp
    src/EventHandler.cpp
    src/TgException.cpp
    src/TgTypeParser.cpp
    src/net/BoostHttpOnlySslClient.cpp
    src/net/CurlHttpClient.cpp
    src/net/HttpParser.cpp
    src/net/TgLongPoll.cpp
    src/net/Url.cpp
    src/tools/FileTools.cpp
    src/tools/StringTools.cpp
    src/types/InlineQueryResult.cpp
    src/types/InputFile.cpp)

## threads
find_package(Threads REQUIRED)

set(LIB_LIST)

if (CONAN_BUILD)
    # Download automatically, you can also just copy the conan.cmake file
    if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
        message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
        file(DOWNLOAD "https://github.com/conan-io/cmake-conan/raw/v0.15/conan.cmake"
                "${CMAKE_BINARY_DIR}/conan.cmake")
    endif()

    include(${CMAKE_BINARY_DIR}/conan.cmake)

    conan_cmake_run(REQUIRES
            zlib/1.2.11@conan/stable
            OpenSSL/1.1.1c@conan/stable
            libcurl/7.56.1@bincrafters/stable
            boost/1.71.0@conan/stable
            BASIC_SETUP
            CMAKE_TARGETS
            BUILD missing)

    set(LIB_LIST
            ${CMAKE_THREAD_LIBS_INIT}
            CONAN_PKG::zlib
            CONAN_PKG::OpenSSL
            CONAN_PKG::libcurl
            CONAN_PKG::boost
            )
else()
    # libs
    ## zlib
    find_package(ZLIB REQUIRED)

    ## openssl
    find_package(OpenSSL REQUIRED)
    include_directories(${OPENSSL_INCLUDE_DIR})

    ## curl
    find_package(CURL 7.56.0)
    if (CURL_FOUND)
        include_directories(${CURL_INCLUDE_DIRS})
        add_definitions(-DHAVE_CURL)
    endif()

    ## boost
    set(Boost_USE_MULTITHREADED ON)
    if (ENABLE_TESTS)
        find_package(Boost 1.59.0 COMPONENTS system unit_test_framework REQUIRED)
    else()
        find_package(Boost 1.59.0 COMPONENTS system REQUIRED)
    endif()
    include_directories(${Boost_INCLUDE_DIR})

    set(LIB_LIST
            ${CMAKE_THREAD_LIBS_INIT}
            ${ZLIB_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${Boost_LIBRARIES}
            ${CURL_LIBRARIES}
            )
endif()



# building project
add_library(${PROJECT_NAME} ${SRC_LIST})
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_link_libraries(${PROJECT_NAME} ${LIB_LIST})
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

# ABI version
set_property(TARGET ${PROJECT_NAME} PROPERTY SOVERSION 1)

# tests
if (ENABLE_TESTS)
    message(STATUS "Building of tests is enabled")
    enable_testing()
    add_subdirectory(test)
endif()
