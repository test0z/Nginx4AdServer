CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
SET(CMAKE_CXX_FLAGS "-std=c++11 -g -O2 -march=native -DMUDUO_STD_STRING")

INCLUDE_DIRECTORIES(.
    ..
    ../service/
    ${MTTY_INCLUDE_DIR}
    ./include
    )

FIND_LIBRARY(LIB_KAFKA NAMES "librdkafka.a" rdkafka)
FIND_LIBRARY(LIB_KAFKAPP NAMES "librdkafka++.a" rdkafka++)
FIND_LIBRARY(LIB_BOOST_SERIALIZATION NAMES "libboost_serialization.a" boost_serialization)
FIND_LIBRARY(LIB_BOOST_LOG  NAMES "libboost_log.a" boost_log)
FIND_LIBRARY(LIB_BOOST_THREAD NAMES "libboost_thread.a" boost_thread)
FIND_LIBRARY(LIB_BOOST_FILESYSTEM NAMES "libboost_filesystem.a" boost_filesystem)
FIND_LIBRARY(LIB_BOOST_SYSTEM NAMES  "libboost_system.a" boost_system)
FIND_LIBRARY(LIB_CRYPTO NAMES "libcrypto.a" crypto)
FIND_LIBRARY(LIB_CRYPTOPP NAMES "libcryptopp.a" cryptopp)
FIND_LIBRARY(LIB_Z NAMES "libz.a" z)
FIND_LIBRARY(LIB_CRYPT NAMES "libcrypt.a" crypt)
FIND_LIBRARY(LIB_AVRO NAMES "libavrocpp_s.a" avrocpp_s)
FIND_LIBRARY(LIB_MUDUO_BASE11 NAMES "libmuduo_base_cpp11.a" muduo_base_cpp11)
FIND_LIBRARY(LIB_TBB NAMES "libtbb.a" tbb)

SET(COMMON_DEPEND_SRC ../service/protocol/log/logstring.cpp
    ../service/protocol/log/log.cpp
    ../service/utility/mttytime.cpp
    ../service/utility/cypher.cpp
    ../service/utility/url.cpp
    ../service/utility/random.cpp)

ADD_EXECUTABLE(kafka_checker ./kafka_checker.cpp
    ${COMMON_DEPEND_SRC})

ADD_EXECUTABLE(locallog_collector ./locallog_collector.cpp
    ${COMMON_DEPEND_SRC}
    ../service/core/core_executor.cpp
    ../service/core/logpusher/log_pusher.cpp
    ../service/core/logpusher/kafka_log_producer.cpp
    ../service/core/core_threadlocal_manager.cpp
    ../service/core/config_types.cpp
    )

SET(LINK_LIBRARY
    rt
    dl
    pthread
    sasl2
    ssl
    ${LIB_KAFKAPP}
    ${LIB_KAFKA}
    ${LIB_BOOST_SERIALIZATION}
    ${LIB_BOOST_LOG}
    ${LIB_BOOST_THREAD}
    ${LIB_BOOST_FILESYSTEM}
    ${LIB_BOOST_SYSTEM}
    ${LIB_CRYPTO}
    ${LIB_CRYPTOPP}
    ${LIB_Z}
    ${LIB_CRYPT}
    ${LIB_AVRO}
    )

TARGET_LINK_LIBRARIES(kafka_checker ${LINK_LIBRARY})

TARGET_LINK_LIBRARIES(locallog_collector ${LINK_LIBRARY}
    ${LIB_MUDUO_BASE11}
    ${LIB_TBB}
    pthread)
