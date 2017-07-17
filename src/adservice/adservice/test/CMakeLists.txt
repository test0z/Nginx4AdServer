CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
SET(CMAKE_CXX_FLAGS "-std=c++11 -g -O2 -march=native -DMUDUO_STD_STRING ")

INCLUDE_DIRECTORIES(.
    ..
    ../service/
    ${MTTY_INCLUDE_DIR})

FIND_LIBRARY(LIB_BOOST_SERIALIZATION NAMES "libboost_serialization.a" boost_serialization)
FIND_LIBRARY(LIB_BOOST_IOSTREAMS NAMES "libboost_iostreams.a" boost_iostreams)
FIND_LIBRARY(LIB_BOOST_LOG  NAMES "libboost_log.a" boost_log)
FIND_LIBRARY(LIB_BOOST_THREAD NAMES "libboost_thread.a" boost_thread)
FIND_LIBRARY(LIB_BOOST_FILESYSTEM NAMES "libboost_filesystem.a" boost_filesystem)
FIND_LIBRARY(LIB_BOOST_SYSTEM NAMES  "libboost_system.a" boost_system)
FIND_LIBRARY(LIB_BOOST_REGEX "libboost_regex.a" boost_regex)
FIND_LIBRARY(LIB_BOOSTER NAMES "libbooster.a" booster)
FIND_LIBRARY(LIB_CRYPTO NAMES "libcrypto.a" crypto)
FIND_LIBRARY(LIB_CRYPTOPP NAMES "libcryptopp.a" cryptopp)
FIND_LIBRARY(LIB_Z NAMES "libz.a" z)
FIND_LIBRARY(LIB_AEROSPIKE NAMES "libaerospike.a" aerospike)
FIND_LIBRARY(LIB_EV NAMES "libev.a" ev)
FIND_LIBRARY(LIB_MTTY mtty)
FIND_LIBRARY(LIB_CPPCMS NAMES "libcppcms.a" cppcms)

ADD_EXECUTABLE(cookiemapping_test ./cookiemapping_test.cpp
    ../service/core/core_cm_manager.cpp
    ../service/core/model/user.cpp
    ../service/core/model/idseq.cpp
    ../service/utility/mttytime.cpp
    ../service/utility/cypher.cpp
    ../service/utility/url.cpp
    ../service/utility/random.cpp)

ADD_EXECUTABLE(url_test ./urlhelper_test.cpp
    ../service/utility/cypher.cpp
    ../service/utility/url.cpp
    ../service/utility/random.cpp)

ADD_EXECUTABLE(te_test ./template_engine_test.cpp
    ../logging.cpp
    ../service/utility/template_engine.cpp
    ../service/utility/file.cpp
    ../service/utility/mttytime.cpp)

ADD_EXECUTABLE(price_test ./price_test.cpp
    ../service/protocol/360/360max_price.cpp
    ../service/utility/cypher.cpp
    ../service/utility/url.cpp
    ../service/utility/random.cpp)

ADD_EXECUTABLE(ip_test ./ip_test.cpp
    ../service/core/core_ip_manager.cpp
    ../service/core/core_scenario_manager.cpp
    ../service/utility/ip.cpp)

ADD_EXECUTABLE(json_test ./json_test.cpp
    ../service/utility/json.cpp)

ADD_EXECUTABLE(executor_test ./executor_test.cpp
    ../service/core/core_executor.cpp
    ../service/utility/random.cpp
    ../service/utility/json.cpp)

SET(LINK_LIBRARY 
#    rt
    dl
    pthread
    ssl
#    ${LIB_BOOSTER}
    ${LIB_BOOST_SERIALIZATION}
    ${LIB_BOOST_LOG}
    ${LIB_BOOST_IOSTREAMS}
    ${LIB_BOOST_THREAD}
    ${LIB_BOOST_FILESYSTEM}
    ${LIB_BOOST_SYSTEM}
    ${LIB_BOOST_REGEX}
    ${LIB_MTTY}
    ${LIB_AEROSPIKE}
    ${LIB_CRYPTO}
    ${LIB_CRYPTOPP}
    ${LIB_Z}
    ${LIB_EV}
    ${LIB_BOOSTER})

TARGET_LINK_LIBRARIES(cookiemapping_test ${LINK_LIBRARY})
TARGET_LINK_LIBRARIES(url_test ${LINK_LIBRARY})
TARGET_LINK_LIBRARIES(price_test ${LINK_LIBRARY})
TARGET_LINK_LIBRARIES(ip_test ${LINK_LIBRARY})
TARGET_LINK_LIBRARIES(json_test ${LINK_LIBRARY} ${LIB_CPPCMS})
TARGET_LINK_LIBRARIES(executor_test ${LINK_LIBRARY} ${LIB_CPPCMS})
TARGET_LINK_LIBRARIES(te_test ${LINK_LIBRARY})
