cmake_minimum_required(VERSION 3.11)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

project(img-panda LANGUAGES C CXX)

file(DOWNLOAD https://raw.githubusercontent.com/simdjson/simdjson/master/singleheader/simdjson.h ${CMAKE_CURRENT_SOURCE_DIR}/deps/simdjson/simdjson.h)

file(DOWNLOAD https://raw.githubusercontent.com/simdjson/simdjson/master/singleheader/simdjson.cpp ${CMAKE_CURRENT_SOURCE_DIR}/deps/simdjson/simdjson.cc)

add_subdirectory(deps/lexbor)

add_executable(ripper
    src/main.c
    
    # Comics
    src/comics/index.c
    src/comics/identify/metadata.c
    src/comics/identify/wordpress.cc
    src/comics/scrapers/comiceasel.c

    # Networking
    src/http/http.c
    src/http/net.c 
    src/http/tcp.c 
    src/http/pool.c
    
    # Parsers
    src/parsers/url_parser.c 
    src/parsers/url.c

    # Utilities
    src/utils.c
    src/b64.c

    # Database abstractions
    src/db/sqlite.c

    # Reverse Image Engine
    src/reverse_image/match.cc
    src/reverse_image/db/sqlite.cc
    src/reverse_image/db/encode.cc

    # Dependencies
    deps/simdjson/simdjson.cc
    deps/hashmap/hashmap.c
)

set_target_properties(ripper PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

find_package(OpenSSL REQUIRED)
find_package(UV REQUIRED)
find_package(OpenCV REQUIRED core imgproc imgcodecs features2d)
find_package(SQLite3 REQUIRED)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include 
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/lexbor/source
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/llhttp/build
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/simdjson
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/hashmap

    ${OpenCV_INCLUDE_DIRS}
    ${UV_INCLUDE_DIR}
    ${OPENSSL_INCLUDE_DIR}
    ${SQLite3_INCLUDE_DIRS}
)

target_link_libraries(ripper 
    # Parsers
    lexbor # HTML
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/llhttp/build/libllhttp.a # HTTP
    # TLS
    OpenSSL::SSL 
    OpenSSL::Crypto 
    # Event Loop
    ${UV_LIBRARIES} 
    # Image Manipulation
    ${OpenCV_LIBS} 
    # DB Clients
    SQLite::SQLite3
)
