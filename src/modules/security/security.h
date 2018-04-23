/*
 * security.h
 *
 *  Created on: Mar 17, 2017
 *  Created by: Huu Nghia NGUYEN <huunghia.nguyen@montimage.com>
 */

#ifndef SRC_LIB_SECURITY_SECURITY_H_
#define SRC_LIB_SECURITY_SECURITY_H_

#include <mmt_core.h>
#include <mmt_security.h>
#include "../../configure.h"
#include "../output/output.h"

typedef struct security_context_struct{
	mmt_handler_t *dpi_handler;

	mmt_sec_handler_t *sec_handler;

	const proto_attribute_t **proto_atts;

	uint32_t proto_atts_count;

	const security_conf_t *config;

	output_t *output;
} security_context_t;

/**
 * This function init globally mmt-security
 * It must be called from main thread before any register_security
 * @return
 */
int security_open( );

/**
 * This function closes globally mmt-security
 * It must be called from main thread after all unregister_security
 */
void security_close();

/**
 */
security_context_t* security_worker_alloc_init( const security_conf_t *config,
		mmt_handler_t *dpi_handler, const uint32_t *core_mask,
		bool verbose,
		output_t *output );

/**
 * Stop and free security
 * @param
 * @return number of alerts being generated
 * @note This function must be called from the same thread that calls #register_security
 */
size_t security_worker_release( security_context_t* );

static inline const char *security_get_version(){
	return mmt_sec_get_version_number();
}
#endif /* SRC_LIB_SECURITY_H_ */
