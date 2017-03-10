//
// Created by guoze.lin on 16/3/22.
//

#define _GNU_SOURCE /* for strndup() */
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

/* Typical include path would be <librdkafka/rdkafka.h>, but this program
 * is built from within the librdkafka source tree and thus differs. */
#include "librdkafka/rdkafka.h" /* for Kafka driver */
                                /* Do not include these defines from your program, they will not be
                                 * provided by librdkafka. */
extern "C" {
//#include "/root/mtty/thirdparty/librdkafka/src/rd.h"
#include "librdkafka/src/rd.h"
//#include "﻿/root/mtty/thirdparty/librdkafka/src/rdtime.h"
#include "librdkafka/src/rdtime.h"
}
#include "core/adselectv2/ad_select_client.h"
#include "protocol/log/log.h"
#include "utility/utility.h"
#include <algorithm>
#include <string>
#include <vector>

using namespace adservice::utility::serialize;

adservice::adselectv2::AdSelectClient * adSelectClient;

extern std::string getLogItemString(protocol::log::LogItem & log);

bool inDebugSession = false;
void * debugSession = nullptr;

static std::vector<std::string> filterConds;

static int run = 1;
static int forever = 1;
static int dispintvl = 1000;
static int do_seq = 0;
static int exit_after = 0;
static int exit_eof = 0;
static FILE * stats_fp;
static int dr_disp_div;
static int verbosity = 1;
static int latency_mode = 0;
static int report_offset = 0;
static FILE * latency_fp = NULL;
static int msgcnt = -1;
static int nostatus = 0;

static void stop(int sig)
{
    if (!run)
        exit(0);
    run = 0;
}

static std::vector<std::string> splitFilterConditions(const std::string& condition){
    std::vector<std::string> result;
    const char* p = condition.c_str(),*p0 = p;
    while(*p!='\0'){
        while(*p!=' '&&*p!='\0')
            p++;
        result.push_back(std::string(p0,p));
        while(*p==' ')
            p++;
        p0 = p;
    }
    return result;
}

static long int msgs_wait_cnt = 0;
static rd_ts_t t_end;
static rd_kafka_t * global_rk;

struct avg {
    int64_t val;
    int cnt;
    uint64_t ts_start;
};

static struct {
    rd_ts_t t_start;
    rd_ts_t t_end;
    rd_ts_t t_end_send;
    uint64_t msgs;
    uint64_t msgs_dr_ok;
    uint64_t msgs_dr_err;
    uint64_t bytes_dr_ok;
    uint64_t bytes;
    uint64_t tx;
    uint64_t tx_err;
    uint64_t avg_rtt;
    uint64_t offset;
    rd_ts_t t_fetch_latency;
    rd_ts_t t_last;
    rd_ts_t t_enobufs_last;
    rd_ts_t t_total;
    rd_ts_t latency_last;
    rd_ts_t latency_lo;
    rd_ts_t latency_hi;
    rd_ts_t latency_sum;
    int latency_cnt;
    int64_t last_offset;
} cnt = {};

/* Returns wall clock time in microseconds */
uint64_t wall_clock(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((uint64_t)tv.tv_sec * 1000000LLU) + ((uint64_t)tv.tv_usec);
}

static void err_cb(rd_kafka_t * rk, int err, const char * reason, void * opaque)
{
    printf("%% ERROR CALLBACK: %s: %s: %s\n", rd_kafka_name(rk), rd_kafka_err2str((rd_kafka_resp_err_t)err), reason);
}

static void throttle_cb(rd_kafka_t * rk, const char * broker_name, int32_t broker_id, int throttle_time_ms,
                        void * opaque)
{
    printf("%% THROTTLED %dms by %s (%d)\n", throttle_time_ms, broker_name, broker_id);
}

static void msg_delivered(rd_kafka_t * rk, const rd_kafka_message_t * rkmessage, void * opaque)
{
    static rd_ts_t last;
    rd_ts_t now = rd_clock();
    static int msgs;

    msgs++;

    msgs_wait_cnt--;

    if (rkmessage->err)
        cnt.msgs_dr_err++;
    else {
        cnt.msgs_dr_ok++;
        cnt.bytes_dr_ok += rkmessage->len;
    }

    if ((rkmessage->err && (cnt.msgs_dr_err < 50 || !(cnt.msgs_dr_err % (dispintvl / 1000)))) || !last
        || msgs_wait_cnt < 5
        || !(msgs_wait_cnt % dr_disp_div)
        || (int)(now - last) >= dispintvl * 1000
        || verbosity >= 3) {
        if (rkmessage->err && verbosity >= 2)
            printf("%% Message delivery failed: %s (%li remain)\n", rd_kafka_err2str(rkmessage->err), msgs_wait_cnt);
        else if (verbosity >= 2)
            printf(
                "%% Message delivered (offset %lld): "
                "%li remain\n",
                rkmessage->offset, msgs_wait_cnt);
        if (verbosity >= 3 && do_seq)
            printf(" --> \"%.*s\"\n", (int)rkmessage->len, (const char *)rkmessage->payload);
        last = now;
    }

    if (report_offset)
        cnt.last_offset = rkmessage->offset;

    if (msgs_wait_cnt == 0 && !forever) {
        if (verbosity >= 2)
            printf("All messages delivered!\n");
        t_end = rd_clock();
        run = 0;
    }

    if (exit_after && exit_after <= msgs) {
        printf("%% Hard exit after %i messages, as requested\n", exit_after);
        exit(0);
    }
}

static void msg_consume(rd_kafka_message_t * rkmessage, void * opaque)
{

    if (rkmessage->err) {
        if (rkmessage->err == RD_KAFKA_RESP_ERR__PARTITION_EOF) {
            cnt.offset = rkmessage->offset;

            if (verbosity >= 1 && !nostatus)
                printf(
                    "%% Consumer reached end of "
                    "%s [%d] "
                    "message queue at offset %lld\n",
                    rd_kafka_topic_name(rkmessage->rkt), rkmessage->partition, rkmessage->offset);

            if (exit_eof)
                run = 0;

            return;
        }

        printf(
            "%% Consume error for topic \"%s\" [%d] "
            "offset %lld: %s\n",
            rd_kafka_topic_name(rkmessage->rkt),
            rkmessage->partition,
            rkmessage->offset,
            rd_kafka_message_errstr(rkmessage));

        if (rkmessage->err == RD_KAFKA_RESP_ERR__UNKNOWN_PARTITION
            || rkmessage->err == RD_KAFKA_RESP_ERR__UNKNOWN_TOPIC)
            run = 0;

        cnt.msgs_dr_err++;
        return;
    }

    cnt.offset = rkmessage->offset;
    cnt.msgs++;
    cnt.bytes += rkmessage->len;

    if (verbosity >= 3 || (verbosity >= 2 && !(cnt.msgs % 1000000)))
        printf("@%lld: %.*s: %.*s\n", rkmessage->offset, (int)rkmessage->key_len, (char *)rkmessage->key,
               (int)rkmessage->len, (char *)rkmessage->payload);

    if (latency_mode) {
        int64_t remote_ts, ts;

        if (rkmessage->len > 8 && !memcmp(rkmessage->payload, "LATENCY:", 8)
            && sscanf((const char *)rkmessage->payload, "LATENCY:%lld", &remote_ts) == 1) {
            ts = wall_clock() - remote_ts;
            if (ts > 0 && ts < (1000000 * 60 * 5)) {
                if (ts > cnt.latency_hi)
                    cnt.latency_hi = ts;
                if (!cnt.latency_lo || ts < cnt.latency_lo)
                    cnt.latency_lo = ts;
                cnt.latency_last = ts;
                cnt.latency_cnt++;
                cnt.latency_sum += ts;
                if (latency_fp)
                    fprintf(latency_fp, "%llu\n", ts);
            } else {
                if (verbosity >= 1)
                    printf("Received latency timestamp is too far off: %lldus (message offset %lld): ignored\n", ts,
                           rkmessage->offset);
            }
        } else if (verbosity > 1) {
            // printf("not a LATENCY payload: %.*s\n",(int) rkmessage->len,(char *) rkmessage->payload);
            if (cnt.msgs <= msgcnt) {
                protocol::log::LogItem logItem;
                try {
                    getAvroObject(logItem, (const uint8_t *)rkmessage->payload, rkmessage->len);
                    std::string str = getLogItemString(logItem);
                    if (filterConds.size()>0) {
                        bool filtered = false;
                        for(auto filterCond:filterConds){
                            if (std::string::npos == str.find(filterCond)) {
                                filtered = true;
                                break;
                            }
                        }
                        if(filtered){
                            cnt.msgs --;
                        }else{
                            printf("%s",str.c_str());
                        }
                    } else {
                        printf("%s", str.c_str());
                    }
                } catch (avro::Exception & e) {
                    printf("exception:%s", e.what());
                    printf("received not avro log object,message:%s\n", (char *)rkmessage->payload);
                }
            }
        }
    }

    if (msgcnt != -1 && (int)cnt.msgs >= msgcnt)
        run = 0;
}

/**
 * Find and extract single value from a two-level search.
 * First find 'field1', then find 'field2' and extract its value.
 * Returns 0 on miss else the value.
 */
static uint64_t json_parse_fields(const char * json, const char ** end, const char * field1, const char * field2)
{
    const char * t = json;
    const char * t2;
    int len1 = strlen(field1);
    int len2 = strlen(field2);

    while ((t2 = strstr(t, field1))) {
        uint64_t v;

        t = t2;
        t += len1;

        /* Find field */
        if (!(t2 = strstr(t, field2)))
            continue;
        t2 += len2;

        while (isspace((int)*t2))
            t2++;

        v = strtoull(t2, (char **)&t, 10);
        if (t2 == t)
            continue;

        *end = t;
        return v;
    }

    *end = t + strlen(t);
    return 0;
}

/**
 * Parse various values from rdkafka stats
 */
static void json_parse_stats(const char * json)
{
    const char * t;
    const int max_avgs = 100; /* max number of brokers to scan for rtt */
    uint64_t avg_rtt[max_avgs + 1];
    int avg_rtt_i = 0;

    /* Store totals at end of array */
    avg_rtt[max_avgs] = 0;

    /* Extract all broker RTTs */
    t = json;
    while (avg_rtt_i < max_avgs && *t) {
        avg_rtt[avg_rtt_i] = json_parse_fields(t, &t, "\"rtt\":", "\"avg\":");

        /* Skip low RTT values, means no messages are passing */
        if (avg_rtt[avg_rtt_i] < 100 /*0.1ms*/)
            continue;

        avg_rtt[max_avgs] += avg_rtt[avg_rtt_i];
        avg_rtt_i++;
    }

    if (avg_rtt_i > 0)
        avg_rtt[max_avgs] /= avg_rtt_i;

    cnt.avg_rtt = avg_rtt[max_avgs];
}

static int stats_cb(rd_kafka_t * rk, char * json, size_t json_len, void * opaque)
{

    /* Extract values for our own stats */
    json_parse_stats(json);

    if (stats_fp)
        fprintf(stats_fp, "%s\n", json);
    return 0;
}

#define _OTYPE_TAB 0x1     /* tabular format */
#define _OTYPE_SUMMARY 0x2 /* summary format */
#define _OTYPE_FORCE 0x4   /* force output regardless of interval timing */
static void print_stats(rd_kafka_t * rk, int mode, int otype, const char * compression)
{
    rd_ts_t now = rd_clock();
    rd_ts_t t_total;
    static int rows_written = 0;
    int print_header;
    char extra[512];
    int extra_of = 0;
    *extra = '\0';

#define EXTRA_PRINTF(fmt...)                                                       \
    do {                                                                           \
        if (extra_of < sizeof(extra))                                              \
            extra_of += snprintf(extra + extra_of, sizeof(extra) - extra_of, fmt); \
    } while (0)

    if (!(otype & _OTYPE_FORCE) && (((otype & _OTYPE_SUMMARY) && verbosity == 0) || cnt.t_last + dispintvl > now))
        return;

    print_header = !rows_written || (verbosity > 0 && !(rows_written % 20));

    if (cnt.t_end_send)
        t_total = cnt.t_end_send - cnt.t_start;
    else if (cnt.t_end)
        t_total = cnt.t_end - cnt.t_start;
    else
        t_total = now - cnt.t_start;

    if (mode == 'P') {

        if (otype & _OTYPE_TAB) {
#define ROW_START() \
    do {            \
    } while (0)
#define COL_HDR(NAME) printf("| %10.10s ", (NAME))
#define COL_PR64(NAME, VAL) printf("| %10llu ", (VAL))
#define COL_PRF(NAME, VAL) printf("| %10.2f ", (VAL))
#define ROW_END()       \
    do {                \
        printf("\n");   \
        rows_written++; \
    } while (0)

            if (print_header) {
                /* First time, print header */
                ROW_START();
                COL_HDR("elapsed");
                COL_HDR("msgs");
                COL_HDR("bytes");
                COL_HDR("rtt");
                COL_HDR("dr");
                COL_HDR("dr_m/s");
                COL_HDR("dr_MB/s");
                COL_HDR("dr_err");
                COL_HDR("tx_err");
                COL_HDR("outq");
                if (report_offset)
                    COL_HDR("offset");
                ROW_END();
            }

            ROW_START();
            COL_PR64("elapsed", t_total / 1000);
            COL_PR64("msgs", cnt.msgs);
            COL_PR64("bytes", cnt.bytes);
            COL_PR64("rtt", cnt.avg_rtt / 1000);
            COL_PR64("dr", cnt.msgs_dr_ok);
            COL_PR64("dr_m/s", ((cnt.msgs_dr_ok * 1000000) / t_total));
            COL_PRF("dr_MB/s", (float)((cnt.bytes_dr_ok) / (float)t_total));
            COL_PR64("dr_err", cnt.msgs_dr_err);
            COL_PR64("tx_err", cnt.tx_err);
            COL_PR64("outq", rk ? (uint64_t)rd_kafka_outq_len(rk) : 0);
            if (report_offset)
                COL_PR64("offset", (uint64_t)cnt.last_offset);
            ROW_END();
        }

        if (otype & _OTYPE_SUMMARY) {
            printf(
                "%% %llu messages produced "
                "(%llu bytes), "
                "%llu delivered "
                "(offset %lld, %llu failed) "
                "in %llums: %llu msgs/s and "
                "%.02f Mb/s, "
                "%llu produce failures, %i in queue, "
                "%s compression\n",
                cnt.msgs, cnt.bytes, cnt.msgs_dr_ok, cnt.last_offset, cnt.msgs_dr_err, t_total / 1000,
                ((cnt.msgs_dr_ok * 1000000) / t_total), (float)((cnt.bytes_dr_ok) / (float)t_total), cnt.tx_err,
                rk ? rd_kafka_outq_len(rk) : 0, compression);
        }

    } else {
        float latency_avg = 0.0f;

        if (latency_mode && cnt.latency_cnt)
            latency_avg = (double)cnt.latency_sum / (float)cnt.latency_cnt;

        if (otype & _OTYPE_TAB) {
            if (print_header) {
                /* First time, print header */
                ROW_START();
                COL_HDR("elapsed");
                COL_HDR("msgs");
                COL_HDR("bytes");
                COL_HDR("rtt");
                COL_HDR("m/s");
                COL_HDR("MB/s");
                COL_HDR("rx_err");
                COL_HDR("offset");
                if (latency_mode) {
                    COL_HDR("lat_curr");
                    COL_HDR("lat_avg");
                    COL_HDR("lat_lo");
                    COL_HDR("lat_hi");
                }
                ROW_END();
            }

            ROW_START();
            COL_PR64("elapsed", t_total / 1000);
            COL_PR64("msgs", cnt.msgs);
            COL_PR64("bytes", cnt.bytes);
            COL_PR64("rtt", cnt.avg_rtt / 1000);
            COL_PR64("m/s", ((cnt.msgs * 1000000) / t_total));
            COL_PRF("MB/s", (float)((cnt.bytes) / (float)t_total));
            COL_PR64("rx_err", cnt.msgs_dr_err);
            COL_PR64("offset", cnt.offset);
            if (latency_mode) {
                COL_PRF("lat_curr", cnt.latency_last / 1000.0f);
                COL_PRF("lat_avg", latency_avg / 1000.0f);
                COL_PRF("lat_lo", cnt.latency_lo / 1000.0f);
                COL_PRF("lat_hi", cnt.latency_hi / 1000.0f);
            }
            ROW_END();
        }

        if (otype & _OTYPE_SUMMARY) {
            if (latency_avg >= 1.0f)
                extra_of += snprintf(extra + extra_of,
                                     sizeof(extra) - extra_of,
                                     ", latency "
                                     "curr/avg/lo/hi "
                                     "%.2f/%.2f/%.2f/%.2fms",
                                     cnt.latency_last / 1000.0f,
                                     latency_avg / 1000.0f,
                                     cnt.latency_lo / 1000.0f,
                                     cnt.latency_hi / 1000.0f);
            printf(
                "%% %llu messages (%llu bytes) "
                "consumed in %llums: %llu msgs/s "
                "(%.02f Mb/s)"
                "%s\n",
                cnt.msgs, cnt.bytes, t_total / 1000, ((cnt.msgs * 1000000) / t_total),
                (float)((cnt.bytes) / (float)t_total), extra);
        }
    }

    cnt.t_last = now;
}

static void sig_usr1(int sig)
{
    rd_kafka_dump(stdout, global_rk);
}

int main(int argc, char ** argv)
{
    const char * brokers = "localhost";
    char mode = 'C';
    char * topic = NULL;
    const char * key = NULL;
    int partition = RD_KAFKA_PARTITION_UA; /* random */
    int opt;
    int sendflags = 0;
    const char * debug = NULL;
    rd_ts_t now;
    char errstr[512];
    uint64_t seq = 0;
    int seed = time(NULL);
    rd_kafka_t * rk;
    rd_kafka_topic_t * rkt;
    rd_kafka_conf_t * conf;
    rd_kafka_topic_conf_t * topic_conf;
    const char * compression = "no";
    int64_t start_offset = 0;
    int batch_size = 0;
    int idle = 0;
    char * stats_intvlstr = NULL;
    char tmp[128];
    char * tmp2;
    int otype = _OTYPE_SUMMARY;
    double dtmp;
    int rate_sleep = 0;
    latency_mode = 1;

    /* Kafka configuration */
    conf = rd_kafka_conf_new();
    rd_kafka_conf_set_error_cb(conf, err_cb);
    rd_kafka_conf_set_throttle_cb(conf, throttle_cb);
    rd_kafka_conf_set_dr_msg_cb(conf, msg_delivered);

    /* Quick termination */
    snprintf(tmp, sizeof(tmp), "%i", SIGIO);
    rd_kafka_conf_set(conf, "internal.termination.signal", tmp, NULL, 0);

    /* Consumer config */
    /* Tell rdkafka to (try to) maintain 1M messages
     * in its internal receive buffers. This is to avoid
     * application -> rdkafka -> broker  per-message ping-pong
     * latency.
     * The larger the local queue, the higher the performance.
     * Try other values with: ... -X queued.min.messages=1000
     */
    rd_kafka_conf_set(conf, "queued.min.messages", "1000000", NULL, 0);

    /* Kafka topic configuration */
    topic_conf = rd_kafka_topic_conf_new();

    while ((opt = getopt(argc, argv, "Ct:p:b:k:s:o:B:v:f:q::")) != -1) {
        switch (opt) {
        case 'C':
            mode = opt;
            break;
        case 't':
            topic = optarg;
            break;
        case 'p':
            partition = atoi(optarg);
            break;
        case 'b':
            brokers = optarg;
            break;
        case 'k':
            key = optarg;
            break;
        case 's':
            msgcnt = atoi(optarg);
            if (msgcnt < 0) {
                msgcnt = -1;
            }
            break;
        case 'o':
            if (!strcmp(optarg, "end"))
                start_offset = RD_KAFKA_OFFSET_END;
            else if (!strcmp(optarg, "beginning"))
                start_offset = RD_KAFKA_OFFSET_BEGINNING;
            else if (!strcmp(optarg, "stored"))
                start_offset = RD_KAFKA_OFFSET_STORED;
            else {
                start_offset = strtoll(optarg, NULL, 10);

                if (start_offset < 0)
                    start_offset = RD_KAFKA_OFFSET_TAIL(-start_offset);
            }

            break;
        case 'B':
            batch_size = atoi(optarg);
            break;
        case 'v':
            verbosity = atoi(optarg);
            break;
        case 'f':
            filterConds = splitFilterConditions(optarg);
            break;
        case 'q':
            nostatus = 1;
            break;
        default:
            fprintf(stderr, "Unknown option: %c\n", opt);
            goto usage;
        }
    }

    if (!topic || optind != argc) {
        if (optind < argc)
            fprintf(stderr, "Unknown argument: %s\n", argv[optind]);
    usage:
        fprintf(stderr,
                "Usage: %s [-C] -t <topic> "
                "[-p <partition>] [-b <broker,broker..>] [options..]\n"
                "\n"
                "librdkafka version %s (0x%08x)\n"
                "\n"
                " Options:\n"
                "  -C       Consumer mode\n"
                "  -t <topic>   Topic to fetch / produce\n"
                "  -p <num>     Partition (defaults to random)\n"
                "  -b <brokers> Broker address list (host[:port],..)\n"
                "  -o           set consumer start offset \n"
                "  -s <num>     set the number of messages to receive\n"
                "  -B <num>     using batch fetch mode\n"
                "  -v <num>     specify verbosity level 1 to 3\n"
                "  -f <condition> filter a condition\n"
                "  -q    do not print status info\n"
                "\n"
                " In Consumer mode:\n"
                "  consumes messages and prints thruput\n"
                "  If -B <..> is supplied the batch consumer\n"
                "  mode is used, else the callback mode is used.\n"
                "\n",
                argv[0], rd_kafka_version_str(), rd_kafka_version(), RD_KAFKA_DEBUG_CONTEXTS);
        exit(1);
    }

    dispintvl *= 1000;
    if (verbosity > 1)
        printf("%% Using random seed %i, verbosity level %i\n", seed, verbosity);
    srand(seed);
    signal(SIGINT, stop);
    signal(SIGUSR1, sig_usr1);
    /* Always enable stats (for RTT extraction), and if user supplied
    * the -T <intvl> option we let her take part of the stats aswell. */
    rd_kafka_conf_set_stats_cb(conf, stats_cb);
    if (!stats_intvlstr) {
        /* if no user-desired stats, adjust stats interval
        * to the display interval. */
        snprintf(tmp, sizeof(tmp), "%i", dispintvl / 1000);
    }
    if (rd_kafka_conf_set(conf, "statistics.interval.ms", stats_intvlstr ? stats_intvlstr : tmp, errstr, sizeof(errstr))
        != RD_KAFKA_CONF_OK) {
        fprintf(stderr, "%% %s\n", errstr);
        exit(1);
    }

    if (mode == 'C') {
        rd_kafka_message_t ** rkmessages = NULL;

        /* Create Kafka handle */
        if (!(rk = rd_kafka_new(RD_KAFKA_CONSUMER, conf, errstr, sizeof(errstr)))) {
            fprintf(stderr, "%% Failed to create Kafka consumer: %s\n", errstr);
            exit(1);
        }
        global_rk = rk;

        /* Add broker(s) */
        if (rd_kafka_brokers_add(rk, brokers) < 1) {
            fprintf(stderr, "%% No valid brokers specified\n");
            exit(1);
        }

        /* Create topic to consume from */
        rkt = rd_kafka_topic_new(rk, topic, topic_conf);

        /* Batch consumer */
        if (batch_size)
            rkmessages = (rd_kafka_message_t **)malloc(sizeof(*rkmessages) * batch_size);

        /* Start consuming */
        if (rd_kafka_consume_start(rkt, partition, start_offset) == -1) {
            fprintf(stderr, "%% Failed to start consuming: %s\n", rd_kafka_err2str(rd_kafka_errno2err(errno)));
            exit(1);
        }

        cnt.t_start = rd_clock();
        while (run && (msgcnt == -1 || msgcnt > (int)cnt.msgs)) {
            /* Consume messages.
             * A message may either be a real message, or
             * an error signaling (if rkmessage->err is set).
             */
            uint64_t fetch_latency;
            int r;

            fetch_latency = rd_clock();

            if (batch_size) {
                int i;

                /* Batch fetch mode */
                r = rd_kafka_consume_batch(rkt, partition, 1000, rkmessages, batch_size);
                if (r != -1) {
                    for (i = 0; i < r; i++) {
                        msg_consume(rkmessages[i], NULL);
                        rd_kafka_message_destroy(rkmessages[i]);
                    }
                }
            } else {
                /* Callback mode */
                r = rd_kafka_consume_callback(rkt, partition, 1000 /*timeout*/, msg_consume, NULL);
            }

            cnt.t_fetch_latency += rd_clock() - fetch_latency;
            if (r == -1)
                fprintf(stderr, "%% Error: %s\n", rd_kafka_err2str(rd_kafka_errno2err(errno)));
            // print_stats(rk, mode, otype, compression);
            /* Poll to handle stats callbacks */
            rd_kafka_poll(rk, 0);
        }
        cnt.t_end = rd_clock();

        /* Stop consuming */
        rd_kafka_consume_stop(rkt, partition);

        /* Destroy topic */
        rd_kafka_topic_destroy(rkt);

        if (batch_size)
            free(rkmessages);

        /* Destroy the handle */
        rd_kafka_destroy(rk);

        global_rk = rk = NULL;
    }

    print_stats(NULL, mode, otype | _OTYPE_FORCE, compression);

    if (cnt.t_fetch_latency && cnt.msgs)
        printf("%% Average application fetch latency: %lluus\n", cnt.t_fetch_latency / cnt.msgs);

    if (latency_fp)
        fclose(latency_fp);

    if (stats_fp) {
        pclose(stats_fp);
        stats_fp = NULL;
    }

    /* Let background threads clean up and terminate cleanly. */
    rd_kafka_wait_destroyed(2000);

    return 0;
}
