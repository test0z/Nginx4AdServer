//
// Created by guoze.lin on 16/2/15.
//

#ifndef ADCORE_CONSTANTS_H
#define ADCORE_CONSTANTS_H

/**日志类别相关常量**/
// 点击日志生成器名
#define MTTY_SERVICE_LOGGER                        "MttyServiceLog"
// 跟踪日志生成器名
#define MTTY_TRACK_LOGGER                          "MttyTrackLog"

/**日志引擎相关默认参数*/
#define DEFAULT_KAFKA_BROKER                        "192.168.31.147"
#define DEFAULT_KAFKA_TOPIC                         "mt-new-click"
#define DEFAULT_KAFKA_KEY                           "click"
#define DEFAULT_KAFKA_MQSIZE_STR                    "10000"

#define DEFAULT_ALIYUN_PRODUCER_ID                  "PID_mtty001"
#define DEFAULT_ALIYUN_TOPIC                        "adlog"
#define DEFAULT_ALIYUN_ACCESS_KEY                   "5jaQzkjjARFVFUrE"
#define DEFAULT_ALIYUN_SECRET_KEY                   "SbFRrY6y1cnSKcdC0QpK1Vkv0QMmTw"

/**ADSelect相关参数*/
// 默认接入节点
#define DEFAULT_ADSELECT_NODE                       "123.56.15.234"
// 默认超时数
#define DEFAULT_ADSELECT_TIMEOUT                    10
// 默认adselect unix sock
#define DEFAULT_ADSELECT_UNIXSOCKFILE               "/tmp/adselect.sock"
// 默认连接端口
#define DEFAULT_ADSELECT_PORT                       9200
// 默认的权限认证
#define DEFAULT_AUTHORIZATION                       "cm9vdDpNdHR5Y29tMTIz"


/**Http 参数相关常量**/
// cookies userId key
#define COOKIES_MTTY_ID                             "m"
// 服务根域名
#define COOKIES_MTTY_DOMAIN                         "mtty.com"
// HTTP GET请求
#define HTTP_REQUEST_GET                            1
// HTTP POST请求
#define HTTP_REQUEST_POST                           0
// HTTP连接最长空闲时间
#define HTTP_IDLE_MAX_SECOND                        5

/**时间相关常量**/
#define DAY_SECOND                                  86440

/**配置类别常量**/
// 服务器相关配置
#define CONFIG_SERVICE                              "ServerConfig"
// 点击模块相关配置
#define CONFIG_CLICK                                "ClickConfig"
// 日志相关的配置键
#define CONFIG_LOG                                  "LogConfig"
// 跟踪日志相关配置
#define CONFIG_TRACK_LOG                            "TrackLogConfig"
// ADSelect组件配置
#define CONFIG_ADSELECT                             "ADSelectConfig"
// DEBUG配置
#define CONFIG_DEBUG                                "DebugConfig"
//  aerospike配置
#define CONFIG_AEROSPIKE                            "AerospikeConfig"

/**配置文件路径常量**/
// 服务配置文件
#define CONFIG_SERVICE_PATH                         "/conf/service.conf"
// 点击模块配置文件
#define CONFIG_CLICK_PATH                           "/conf/click.conf"
// 相关日志配置文件
#define CONFIG_LOG_PATH                             "/conf/log.conf"
// 跟踪模块日志配置文件
#define CONFIG_TRACK_LOG_PATH                       "/conf/track_log.conf"
// ADSelect相关配置文件
#define CONFIG_ADSELECT_PATH                        "/conf/adselect.conf"
// DEBUG相关配置文件
#define CONFIG_DEBUG_PATH                           "/conf/debug.conf"
//  aerospike配置文件
#define CONFIG_AEROSPIKE_PATH                       "/conf/aerospike.conf"

/**核心逻辑模块常量**/
// 点击逻辑服务屏蔽字
#define MASK_CLICK                                  0X00000001
// 曝光逻辑服务屏蔽字
#define MASK_SHOW                                   0X00000002
// 竞价逻辑服务屏蔽字
#define MASK_BID                                    0X00000004
// 曝光统计模块服务屏蔽字
#define MASK_VIEW                                   0X00000008
// 跟踪逻辑模块服务屏蔽字
#define MASK_TRACK                                  0X00000010


/**url 参数常量*/
// 落地URL
#define URL_LANDING_URL                             "url"
// referer
#define URL_REFERER                                 "f"
// ADX广告位ID
#define URL_ADPLACE_ID                              "s"
// 我方广告位ID
#define URL_MTTYADPLACE_ID                          "o"
// 曝光ID
#define URL_EXPOSE_ID                               "r"
// 广告主ID
#define URL_ADOWNER_ID                              "d"
// 推广计划ID
#define URL_ADPLAN_ID                               "t"
// 投放单元ID
#define URL_EXEC_ID                                 "e"
// 创意ID
#define URL_CREATIVE_ID                             "c"
// 平台ID,ADX ID
#define URL_ADX_ID                                  "x"
// 点击ID
#define URL_CLICK_ID                                "h"
// 区域ID
#define URL_AREA_ID                                 "a"
// 点击区域坐标x
#define URL_CLICK_X                                 "sx"
// 点击区域坐标y
#define URL_CLICK_Y                                 "sy"
// ADX 宏
#define URL_ADX_MACRO                               "l"
// 出价价格
#define URL_BID_PRICE                               "b"
// 成交价格
#define URL_EXCHANGE_PRICE                          "p"
// 设备id
#define URL_DEVICE_UID                              "u"
// 计费类型
#define URL_PRICE_TYPE                              "pt"
// 产品包id
#define URL_PRODUCTPACKAGE_ID                       "ep"
// of,曝光类型
#define URL_IMP_OF                                  "of"
// pid,ssp曝光url中的广告位Id
#define URL_SSP_PID                                 "pid"
// adxpid,ssp曝光url中的adx广告位Id
#define URL_SSP_ADX_PID                             "adxpid"
// youku deal标记,PMP结算方式
#define URL_YOUKU_DEAL                              "w"
// url中出价的加密密钥
#define OFFERPRICE_ENCODE_KEY                       "iamlikesupercool"

// 广告主花费系数
#define AD_OWNER_COST_FACTOR                        1.54
// 竞价模块CPM模式出价系数
#define AD_RTB_BIDOFFERPRICE_FACTOR                 0.65

#define ADX_MACRO_SUFFIX                            "http%3A%2F%2Fmtty-cdn.mtty.com%2F1x1.gif"


/** 曝光类型 */
// 广告代理曝光
#define OF_DSP                                      "0"
// 自有媒体曝光
#define OF_SSP                                      "1"
// 移动流量纯展示,参见http://redmine.mtty.com/redmine/issues/48
#define OF_DSP_MOBILE_SHOW                          "2"
// 移动流量日志计费
#define OF_DSP_MOBILE_LOG                           "3"
// 移动SSP,参见http://redmine.mtty.com/redmine/issues/144
#define OF_SSP_MOBILE                               "4"


/** HTML模版路径 */
// 曝光模块返回ADX html模板
#define TEMPLATE_SHOW_ADX_PATH                      "res/html/show_adx.html"
// 曝光模块返回SSP html模块
#define TEMPLATE_SHOW_SSP_PATH                      "res/html/show_ssp.html"
// 光阴adx adm模版
#define TEMPLATE_SHOW_ADX_GUANGYIN_PATH             "res/html/show_adx_guangyin.html"

// SSP返回点击宏中的点击链接
#define SSP_CLICK_URL                               "http://click.mtty.com/c"

/** ADX 广告交换商代码 */
// 淘宝
#define ADX_TANX                                    1
// 优酷
#define ADX_YOUKU                                   5
// 百度
#define ADX_BAIDU                                   6
// 搜狐PC
#define ADX_SOHU_PC                                 8
// 腾讯广点通
#define ADX_TENCENT_GDT                             13
// 光阴网络
#define ADX_GUANGYIN                                14
// 淘宝移动
#define ADX_TANX_MOBILE                             15
// 优酷移动
#define ADX_YOUKU_MOBILE                            17
// 腾讯广点通移动
#define ADX_GDT_MOBILE                              18
// 搜狐移动
#define ADX_SOHU_MOBILE                             19
// 网易新闻移动
#define ADX_NETEASE_MOBILE                          20
// 光阴移动
#define ADX_GUANGYIN_MOBILE                         21
// mtty ssp pc
#define ADX_SSP_PC                                  98
// mtty ssp
#define ADX_SSP_MOBILE                              99

// 淘宝竞价请求模块名
#define BID_QUERY_PATH_TANX                         "/tanxbid"
// 优酷竞价请求模块名
#define BID_QUERY_PATH_YOUKU                        "/youkubid"
// 百度竞价请求模块名
#define BID_QUERY_PATH_BAIDU                        "/besbid"
// 腾讯竞价请求模块名
#define BID_QUERY_PATH_GDT                          "/gdtbid"
// 搜狐竞价请求模块名
#define BID_QUERY_PATH_SOHU                         "/sohubid"
// 网易竞价请求模块名
#define BID_QUERY_PATH_NETEASE                      "/neteasebid"
// 光阴竞价请求模块名
#define BID_QUERY_PATH_GUANGYIN                     "/gybid"

// snippet 中的 曝光URL
#define SNIPPET_SHOW_URL                            "http://show.mtty.com/v"

// snippet 中的 frame
#define SNIPPET_IFRAME                              "<iframe width=\"%d\" height=\"%d\" frameborder=\"0\" scrolling=\"no\" src=\"%s?%s%s\"></iframe><!--<img src=\"%s&\"/>-->"
// snippet 中的 script
#define SNIPPET_SCRIPT                              "<style type=\"text/css\">*{margin:0px; padding:0px;}</style><script type=\"text/javascript\"> document.write(\'<iframe width=\"%d\" height=\"%d\" frameborder=\"0\" scrolling=\"no\" src=\"%s?%s&gc=%s&pid=%s&%s&bdclick=\'+encodeURIComponent(\"%s\") +\'\"></iframe>\');</script>"

/** elasticsearch 相关参数 */
// 投放单ID基数
#define ES_SOLUTION_ID_BASE                         1000000
// 创意ID基数
#define ES_BANNER_ID_BASE                           5000000
// ES中的索引名
#define ES_INDEX_SOLUTIONS                          "mtsolutions"
// ES中的投放类型表
#define ES_DOCUMENT_SOLUTION                        "solution"
// ES中的创意类型表
#define ES_DOCUMENT_BANNER                          "banner"
// ES中的广告位表
#define ES_DOCUMENT_ADPLACE                         "adplace"
// ES中的高级出价器表
#define ES_DOCUMENT_ES_ADPLACE                      "es_adplace"
// ES中 solution,banner和es_adplace的混合表
#define ES_DOCUMENT_SOLBANADPLACE                   "banner,solution,es_adplace,adplace"
// ES中查询格式参数
#define ES_FILTER_FORMAT                            "?pretty&filter_path=hits.total,hits.hits._source"
// ES中的查询结果格式
#define ES_FILTER_FORMAT2                           "?pretty&filter_path=hits.total,hits.hits"

// ES中查询创意的DSL文件路径
#define  ES_QUERY_CREATIVE                          "elasticsearch/dsl/query_banner.dsl"
// ES中按PID查询广告信息的DSL文件路径
#define  ES_QUERY_ADINFO_BY_PID                     "elasticsearch/dsl/query_adinfo_by_pid.dsl"
// ES中按ADXPID查询广告信息的DSL文件路径
#define  ES_QUERY_ADINFO_BY_ADXPID                  "elasticsearch/dsl/query_adinfo_by_adxpid.dsl"
// ES中按PID查询广告位信息的DSL文件路径
#define  ES_QUERY_ADPLACE_BY_PID                    "elasticsearch/dsl/query_adplace_by_pid.dsl"
// ES中按ADXPID查询广告位信息的DSL文件路径
#define  ES_QUERY_ADPLACE_BY_ADXPID                 "elasticsearch/dsl/query_adplace_by_adxpid.dsl"
// ES中按根据广告位条件查询广告信息
#define  ES_QUERY_ADINFO_BY_CONDITION               "elasticsearch/dsl/query_adinfo_by_condition.dsl"
// ES中按根据广告位条件查询广告信息,其中条件包含多个尺寸s
#define  ES_QUERY_ADINFO_BY_CONDITION_MULTISIZE     "elasticsearch/dsl/query_adinfo_by_condition_multisize.dsl"
// ES中按照广告位的简单条件查询广告信息
#define  ES_QUERY_ADINFO_BY_CONDITION_SIMPLE        "elasticsearch/dsl/query_adinfo_by_condition_simple.dsl"

// ES中Banner表的json字段名
#define  ES_BANNER_FILED_JSON                       "json"

// 投放单中的计费方式
#define PRICETYPE_CPD                               0
#define PRICETYPE_CPM                               1
#define PRICETYPE_CPC                               2
#define PRICETYPE_RCPC                              3
#define PRICETYPE_RTB                               4
#define PRICETYPE_RRTB_CPM                          5
#define PRICETYPE_RRTB_CPC                          6

// ADSELECT CACHE存活秒数
#define  ADSELECT_CACHE_EXPIRE_TIME                 10

// URL多长被认为是长URL
#define  URL_LONG_REQUEST_THRESH                    1024
#define  URL_LONG_INPUT_PARAMETER                   1024

// IP 库 数据文件路径
#define IP_DATA_FILE                                "res/17monipdb.dat"
// 地域编码文件
#define AREA_CODE_FILE                              "res/city_area_code.txt"
// 地域编码地位
#define AREACODE_MARGIN                             1000000
// 内容id映射文件
#define CONTENTTYPE_FILE                            "res/contenttype_map.txt"


// 投放单投放成功率的最大权重
#define SOLUTION_SUCCESS_RATE_BASE                  10000
// 默认的产品包id
#define DEFAULT_PRODUCT_PACKAGE_ID                  10000
// 默认的订单id
#define DEFAULT_PRODUCT_ORDER_ID                    10000

//http://redmine.mtty.com/redmine/attachments/download/18/mtWeb定向方式编码表.xlsx
// 投放单浏览器定向编码表(仅PC)
// 不限浏览器类型
#define SOLUTION_BROWSER_ALL                        0
// IE
#define SOLUTION_BROWSER_IE                         1
// EDGE
#define SOLUTION_BROWSER_EDGE                       2
// 360浏览器
#define SOLUTION_BROWSER_360                        3
// Chrome
#define SOLUTION_BROWSER_CHROME                     4
// firefox
#define SOLUTION_BROWSER_FIREFOX                    5
// Safari
#define SOLUTION_BROWSER_SAFARI                     6
// 遨游
#define SOLUTION_BROWSER_AOYOU                      7
// QQ浏览器
#define SOLUTION_BROWSER_QQ                         8
// 猎豹浏览器
#define SOLUTION_BROWSER_LIEBAO                     9
// 其它浏览器
#define SOLUTION_BROWSER_OTHER                      20

// 投放单操作系统定向编码表(仅PC)
// 不限操作系统类型
#define SOLUTION_OS_ALL                             0
// Windows
#define SOLUTION_OS_WINDOWS                         1
// Mac
#define SOLUTION_OS_MAC                             2
// Linux
#define SOLUTION_OS_LINUX                           3
// 其它操作系统
#define SOLUTION_OS_OTHER                           20

// 投放单设备定向编码表(仅移动)
// 不限设备类型
#define SOLUTION_DEVICE_ALL                         0
// iPhone
#define SOLUTION_DEVICE_IPHONE                      1
// Android
#define SOLUTION_DEVICE_ANDROID                     2
// iPad
#define SOLUTION_DEVICE_IPAD                        3
// WindowsPhone
#define SOLUTION_DEVICE_WINDOWSPHONE                4
// Android Pad
#define SOLUTION_DEVICE_ANDROIDPAD                  5
// TV
#define SOLUTION_DEVICE_TV                          6
// 其它设备
#define SOLUTION_DEVICE_OTHER                       20

// 投放单网络环境定向编码表(仅移动)
// 不限网络环境
#define SOLUTION_NETWORK_ALL                        0
// wifi
#define SOLUTION_NETWORK_WIFI                       1
// 未知
#define SOLUTION_NETWORK_UNKNOWN                    2
// 2G
#define SOLUTION_NETWORK_2G                         3
// 3G
#define SOLUTION_NETWORK_3G                         4
// 4G
#define SOLUTION_NETWORK_4G                         5
// 其它
#define SOLUTION_NETWORK_OTHER                      20

// 投放单运营商定向(仅移动)
// 不限运营商
#define SOLUTION_NETWORK_PROVIDER_ALL               0
// 中国移动
#define SOLUTION_NETWORK_PROVIDER_CHINAMOBILE       46000
// 中国联通
#define SOLUTION_NETWORK_PROVIDER_CHINAUNICOM       46001
// 中国电信
#define SOLUTION_NETWORK_PROVIDER_CHINATELECOM      46003


// PC 流量 前端宏
#define  FLOWTYPE_FRONTEND_PC                                0
// 移动流量 前端宏
#define  FLOWTYPE_FRONTEND_MOBILE                            1
// INAPP 流量 前端宏
#define  FLOWTYPE_FRONTEND_INAPP                             2
// 视频流量 前端宏
#define  FLOWTYPE_FRONTEND_VIDEO                             3

// 不限流量 后端宏
#define SOLUTION_FLOWTYPE_ALL                                0
// PC
#define SOLUTION_FLOWTYPE_PC                                 1
// 移动
#define SOLUTION_FLOWTYPE_MOBILE                             2
// 未知流量
#define SOLUTION_FLOWTYPE_UNKNOWN                            20

//普通图片
#define BANNER_TYPE_PIC                                     1
//移动图片
#define BANNER_TYPE_MOBILE                                  2
//贴片视频
#define BANNER_TYPE_VIDEO                                   3
//原生创意
#define BANNER_TYPE_PRIMITIVE                               4

// 加速投放
#define SOLUTION_CONTROLL_TYPE_ACC                           1
// 匀速投放
#define SOLUTION_CONTROLL_TYPE_MODERATE                      0

// SSP URL 参数 设备类型
#define URL_SSP_DEVICE                              "t1"
// SSP URL 参数 平台类型
#define URL_SSP_PLATFORM                            "t2"

#define SSP_DEVICE_UNKNOWN                          "0"
#define SSP_DEVICE_MOBILEPHONE                      "1"
#define SSP_DEVICE_PAD                              "2"

#define SSP_PLATFORM_UNKNOWN                        "0"
#define SSP_PLATFORM_IOS                            "1"
#define SSP_PLATFORM_ANDROID                        "2"
#define SSP_PLATFORM_WINDOWSPHONE                   "3"

#define IP_UNKNOWN                                  999999

#endif //ADCORE_CONSTANTS_H
