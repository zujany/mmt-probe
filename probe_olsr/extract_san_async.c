#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcap.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/adapters/libuv.h>
#include <libjson/libjson.h>


#include "mmt_core.h"
#include "attribute_json.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#define MAX_FILENAME_SIZE 256
#define TRACE_FILE 1
#define LIVE_INTERFACE 2
#define MTU_BIG (16 * 1024)

static int quiet;

static redisAsyncContext * redis_a = NULL;
static mmt_handler_t * mmt_handler = NULL;
static char filename[MAX_FILENAME_SIZE + 1];
static int type;

static struct attr_id session_source[] = 
{
    {"ip", "dst"},
};

static struct attr_id attr1[] = 
{
};

static struct bus_event olsrevt = {
    "olsr.hello",
    {"olsr", "hello"},
    1,
    0,
    (struct attr_id *) &session_source,
    (struct attr_id *) &attr1,    
};

void * redis_event_loop_fct(void * args);

void dummy_cb (redisAsyncContext *c, void *reply, void *privdata) {
//  printf("zifit\n");
}

void connectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        return;
    }
    printf("Connected...\n");
    (void) redis_event_loop_fct(NULL);
    printf("Connected...\n");
}

void disconnectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        return;
    }
    printf("Disconnected...\n");
    mmt_close_handler( mmt_handler );
}

void usage(const char * prg_name) {
    fprintf(stderr, "%s [<option>]\n", prg_name);
    fprintf(stderr, "Option:\n");
    fprintf(stderr, "\t-t <trace file>: Gives the trace file to analyse.\n");
    fprintf(stderr, "\t-i <interface> : Gives the interface name for live traffic analysis.\n");
    fprintf(stderr, "\t-q             : Be quiet (no output whatsoever, helps profiling).\n");
    fprintf(stderr, "\t-h             : Prints this help.\n");
    exit(1);
}

void parseOptions(int argc, char ** argv, char * filename, int * type) {
    int opt, optcount = 0;
    while ((opt = getopt(argc, argv, "t:i:qh")) != EOF) {
        switch (opt) {
            case 't':
                optcount++;
                if (optcount > 1) {
                    usage(argv[0]);
                }
                strncpy((char *) filename, optarg, MAX_FILENAME_SIZE);
                *type = TRACE_FILE;
                break;
            case 'i':
                optcount++;
                if (optcount > 1) {
                    usage(argv[0]);
                }
                strncpy((char *) filename, optarg, MAX_FILENAME_SIZE);
                *type = LIVE_INTERFACE;
                break;
            case 'q':
                quiet = 1;
                break;
            case 'h':
            default: usage(argv[0]);
        }
    }

    if (filename == NULL || strcmp(filename, "") == 0) {
        if (*type == TRACE_FILE) {
            fprintf(stderr, "Missing trace file name\n");
        }
        if (*type == LIVE_INTERFACE) {
            fprintf(stderr, "Missing network interface name\n");
        }
        usage(argv[0]);
    }
    return;
}

void attr_handler(const ipacket_t * ipacket, attribute_t * attr, void * user_args) {
    struct bus_event * test_event = (struct bus_event *) user_args;
    char buff[20000];
    mmt_attr_sprintf(buff, 19999, attr);
    
    json_t * n = mmt_json_new_node();
    mmt_json_push(n, mmt_json_new_a("v", "1.0"));
    mmt_json_push(n, mmt_json_new_f("ts", ((double) ipacket->p_hdr->ts.tv_sec * 1000) + (int) (ipacket->p_hdr->ts.tv_usec/1000)));
    mmt_json_push(n, mmt_json_new_a("type", "event"));

    json_t * data = mmt_json_new_node();
    mmt_json_set_name(data, "data");
    mmt_json_push(data, mmt_json_new_a("id", test_event->event_id));//TODO: the event name will be described somewhere, for the moment use a static text
    mmt_json_push(data, mmt_attr_json(attr, 0));

    // Now concatenate the instance id string
    int count = 0, inst_id_len = 1024; //TODO: this should be e definition
    int off = 0;
    char inst_id[1024];
    while(count < test_event->session_nb) {
        attribute_t * ext_attr = get_extracted_attribute_by_name(ipacket, test_event->session_src[count].proto, test_event->session_src[count].attr);
        if(ext_attr != NULL) {
            if(count > 0) off += snprintf((char *) &inst_id[off], 2, ":"); 
            off += mmt_attr_sprintf((char *) &inst_id[off], inst_id_len - off, ext_attr);
        }
        count++;
    }
    if(off) {
        mmt_json_push(data, mmt_json_new_a("instance_id", inst_id));
    }

    mmt_json_push(n, data);

    json_t * attributes = mmt_json_new_node();
    mmt_json_set_name(attributes, "attributes");

    int i = 0;
    while(i < test_event->attr_nb) {
        attribute_t * ext_attr = get_extracted_attribute_by_name(ipacket, test_event->attributes[i].proto, test_event->attributes[i].attr);
        if(ext_attr != NULL) {
            mmt_json_push(attributes, mmt_attr_json(ext_attr, 2));
        }
        i++;
    }

    
    mmt_json_push(n, attributes);

    char * format = mmt_json_format(n);
    printf("%s", format);

    // Publish an event 
    redisAsyncCommand(
        redis_a, dummy_cb, NULL,
        "PUBLISH %s %s", test_event->event_id, (char *) format);//TODO: the event (channel) name will be described somewhere, for the moment use a static text
    mmt_format_free(format);
    mmt_json_destroy(n);

}

void extract_attributes (mmt_handler_t * handler, struct bus_event * test_event) {
    int i = 0;
    while(i < test_event->session_nb) {
        register_extraction_attribute_by_name(handler, test_event->session_src[i].proto, test_event->session_src[i].attr);
        printf("\n%i\n", i);
        i++;
    }
    i = 0;
    while(i < test_event->attr_nb) {
        register_extraction_attribute_by_name(handler, test_event->attributes[i].proto, test_event->attributes[i].attr);
        printf("\n%i\n", i);
        i++;
    }
    register_attribute_handler_by_name(handler, test_event->event.proto, test_event->event.attr, attr_handler, NULL, (void *) test_event);
}

void live_capture_callback( u_char *user, const struct pcap_pkthdr *p_pkthdr, const u_char *data )
{
    mmt_handler_t *mmt = (mmt_handler_t*)user;
    struct pkthdr header;
    header.ts = p_pkthdr->ts;
    header.caplen = p_pkthdr->caplen;
    header.len = p_pkthdr->len;
    if (!packet_process( mmt, &header, data )) {
        fprintf(stderr, "Packet data extraction failure.\n");
    }
}

void * redis_event_loop_fct(void * args) {
    char mmt_errbuf[1024];
    pcap_t *pcap;
    const unsigned char *data;
    struct pcap_pkthdr p_pkthdr;
    char errbuf[1024];

    struct pkthdr header;

    init_extraction();

    //Initialize an MMT handler
    mmt_handler = mmt_init_handler(DLT_EN10MB, 0, mmt_errbuf);
    if (!mmt_handler) { /* pcap error ? */
        fprintf(stderr, "MMT handler init failed for the following reason: %s\n", mmt_errbuf);
        exit(EXIT_FAILURE);
    }

    extract_attributes(mmt_handler, &olsrevt);

    if (type == TRACE_FILE) {
        pcap = pcap_open_offline(filename, errbuf); // open offline trace
        if (!pcap) { /* pcap error ? */
            fprintf(stderr, "pcap_open failed for the following reason: %s\n", errbuf);
            exit(EXIT_FAILURE);
        }

        while ((data = pcap_next(pcap, &p_pkthdr))) {
            header.ts = p_pkthdr.ts;
            header.caplen = p_pkthdr.caplen;
            header.len = p_pkthdr.len;
            if (!packet_process(mmt_handler, &header, data)) {
                fprintf(stderr, "Packet data extraction failure.\n");
            }
        }
    } else {
        pcap = pcap_open_live(filename, MTU_BIG, 1, 1000, errbuf);
        if (!pcap) {
            fprintf(stderr, "pcap_open failed for the following reason: %s\n", errbuf);
            exit(EXIT_FAILURE);
        }
        (void)pcap_loop( pcap, -1, &live_capture_callback, (u_char*)mmt_handler );
    }
    printf("Processing over\n");
    mmt_close_handler(mmt_handler);
    close_extraction();
    pcap_close(pcap);
}

int main(int argc, char** argv) {
    pthread_t uv_thread;

    quiet = 0;
    parseOptions(argc, argv, filename, &type);

    signal(SIGPIPE, SIG_IGN);
    uv_loop_t* loop = uv_default_loop();

    redis_a = redisAsyncConnect("127.0.0.1", 6379);
    if (redis_a->err) {
        printf("Error: %s\n", redis_a->errstr);
        // handle error & free redis_a
        exit(EXIT_FAILURE);
    }

    redisLibuvAttach(redis_a, loop);
    redisAsyncSetConnectCallback(redis_a, connectCallback);
    redisAsyncSetDisconnectCallback(redis_a, disconnectCallback);
   /* 
    int rc;
    rc = pthread_create(&uv_thread, NULL, redis_event_loop_fct, (void *) loop);
    if (rc){
        printf("ERROR; return code from pthread_create() is %d\n", rc);
        exit(-1);
    }
    */
    uv_run(loop, UV_RUN_DEFAULT); //creates an event loop
    /* Disconnect after receiving the reply to GET */
    //redisAsyncDisconnect(redis_a);
    return EXIT_SUCCESS;
}
