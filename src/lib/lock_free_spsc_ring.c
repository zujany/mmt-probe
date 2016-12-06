/*
 * lock_free_spsc_ring.c
 *
 *  Created on: 31 mars 2016
 *      Author: nhnghia
 *
 * An implementation of Lamport queue without lock
 * based on https://github.com/blytkerchan/Lamport
 */
#include <stdlib.h>
#include <stdatomic.h>
#include "lock_free_spsc_ring.h"

void queue_free( lock_free_spsc_ring_t *q ){
	if( q == NULL ) return;
	if( q->_data ) free( q->_data );
	free( q );
}
void queue_init( lock_free_spsc_ring_t *q, uint32_t size ){
	q->_data = malloc( sizeof( uint32_t) * size );
	q->_size = size;
	q->_head = q->_tail = 0;
	q->_cached_head = q->_cached_tail = 0;
}

