#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "mmt_core.h"
#include "mmt/tcpip/mmt_tcpip_protocols.h"
#include "processing.h"

/**
 * Returns 1 if the given session is a microflow, O otherwise
 * @param expired_session pointer to the session context to check
 * @return 1 if the given session is a microflow, O otherwise
 */
uint32_t is_microflow(const mmt_session_t * expired_session) {
    mmt_probe_context_t * probe_context = get_probe_context_config();

    if (probe_context->microflow_enable==1){
        if ((get_session_packet_count(expired_session) <= probe_context->microf_pthreshold) || (get_session_byte_count(expired_session) <= probe_context->microf_bthreshold)) {
            return 1;
        }
    }
    return 0;
}

uint32_t is_microflow_stats_reportable(microsessions_stats_t * stats) {
    mmt_probe_context_t * probe_context = get_probe_context_config();

    if ((stats->flows_nb > probe_context->microf_report_fthreshold)
            || ((stats->dl_pcount + stats->ul_pcount) > probe_context->microf_report_pthreshold)
            || ((stats->dl_bcount + stats->ul_bcount) > probe_context->microf_report_bthreshold)) {
        return 1;
    }
    return 0;
}

void reset_microflows_stats(microsessions_stats_t * stats) {
    stats->dl_pcount = 0;
    stats->dl_bcount = 0;
    stats->ul_pcount = 0;
    stats->ul_bcount = 0;
    stats->flows_nb = 0;
}

void report_all_protocols_microflows_stats(probe_internal_t * iprobe) {
    int i;
    //FILE * out_file = (probe_context->data_out_file != NULL) ? probe_context->data_out_file : stdout;
    for (i = 0; i < PROTO_MAX_IDENTIFIER; i++) {
        if (iprobe->mf_stats[i].flows_nb) {
            report_microflows_stats(&iprobe->mf_stats[i]);
        }
    }
}

void report_microflows_stats(microsessions_stats_t * stats) {
    mmt_probe_context_t * probe_context = get_probe_context_config();

    //Format id, timestamp, App name, Nb of flows, DL Packet Count, UL Packet Count, DL Byte Count, UL Byte Count
    char message[MAX_MESS + 1];
    snprintf(message, MAX_MESS,
            "%u,%u,\"%s\",%lu.%lu,%u,%u,%u,%u,%u,%u",
            MMT_MICROFLOWS_STATS_FORMAT, probe_context->probe_id_number, probe_context->input_source, stats->end_time.tv_sec, stats->end_time.tv_usec,
            stats->application_id, stats->flows_nb, stats->dl_pcount, stats->ul_pcount, stats->dl_bcount, stats->ul_bcount);

    message[ MAX_MESS ] = '\0'; // correct end of string in case of truncated message
    //send_message_to_file ("microflows.report", message);
    if (probe_context->output_to_file_enable==1)send_message_to_file (message);
    if (probe_context->redis_enable==1)send_message_to_redis ("microflows.report", message);
    /*
     fprintf(out_file, "%i,%lu.%lu,"
         //"%lu.%lu,"
         "%u,%u,%u,%u,%u,%u\n",
         MMT_MICROFLOWS_STATS_FORMAT,
         //stats->start_time.tv_sec, stats->start_time.tv_usec,
         stats->end_time.tv_sec, stats->end_time.tv_usec,
         stats->application_id,
         stats->flows_nb, stats->dl_pcount, stats->ul_pcount, stats->dl_bcount, stats->ul_bcount);
     */
    //Now clean the stats
    reset_microflows_stats(stats);
}

void update_microflows_stats(microsessions_stats_t * stats, const mmt_session_t * expired_session) {
    int keep_direction = 1;
    session_struct_t * temp_session = get_user_session_context(expired_session);
    if (temp_session->ipversion == 4) {
        keep_direction = is_local_net(temp_session->ipclient.ipv4);
        is_local_net(temp_session->ipserver.ipv4);
    }

    if(keep_direction) {
        stats->dl_pcount += get_session_dl_packet_count(expired_session);
        stats->dl_bcount += get_session_dl_byte_count(expired_session);
        stats->ul_pcount += get_session_ul_packet_count(expired_session);
        stats->ul_bcount += get_session_ul_byte_count(expired_session);
    }else {
        stats->dl_pcount += get_session_ul_packet_count(expired_session);
        stats->dl_bcount += get_session_ul_byte_count(expired_session);
        stats->ul_pcount += get_session_dl_packet_count(expired_session);
        stats->ul_bcount += get_session_dl_byte_count(expired_session);
    }
    stats->flows_nb += 1;
    stats->end_time = get_session_last_activity_time(expired_session);
}

