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
FIND_LIBRARY(LIB_BOOST_IOSTREAMS NAMES "libboost_iostreams.a" boost_iostreams)
FIND_LIBRARY(LIB_BOOSTER NAMES "libbooster.a" booster)
FIND_LIBRARY(LIB_AEROSPIKE NAMES "libaerospike.a" aerospike)
FIND_LIBRARY(LIB_EV NAMES "libev.a" ev)
FIND_LIBRARY(LIB_MTTY mtty)
FIND_LIBRARY(LIB_CPPCMS NAMES "libcppcms.a" cppcms)

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
    ../service/core/config_types.cpp
    )

ADD_EXECUTABLE(update_from_db ./update_from_db.cpp)

ADD_EXECUTABLE(translate_user ./translate_user.cpp
    ../service/utility/cypher.cpp
    ../service/utility/random.cpp)


ADD_EXECUTABLE(track_user ./track_user.cpp
    ../service/core/core_cm_manager.cpp
    ../service/core/model/user.cpp
    ../service/core/model/idseq.cpp
    ../service/core/model/source_id.cpp
    ../service/utility/mttytime.cpp
    ../service/utility/cypher.cpp
    ../service/utility/url.cpp
    ../service/utility/random.cpp
    ../logging.cpp
)

SET(LINK_LIBRARY
    rt
    dl
    pthread
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
    pthread)

TARGET_LINK_LIBRARIES(update_from_db ${LINK_LIBRARY}
    mysqlcppconn
)

TARGET_LINK_LIBRARIES(track_user  ${LIB_BOOST_IOSTREAMS}
    ${LIB_MTTY}
    ${LIB_AEROSPIKE}
    ${LIB_EV}
    ${LIB_BOOSTER}    
    ${LINK_LIBRARY}
)

TARGET_LINK_LIBRARIES(translate_user
    ${LIB_MTTY}
    ${LIB_AEROSPIKE}
    ${LIB_EV}
    ${LIB_BOOSTER}
    ${LINK_LIBRARY}
)
