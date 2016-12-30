
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef TCP_QUEUE_H
#define TCP_QUEUE_H

#include <stdbool.h>
#include <stdint.h>

/**
 * \addtogroup net
 *
 * @{
 */
/**
 * \addtogroup net_stack
 *
 * @{
 */
/**
 * \addtogroup net_stack_tcp
 *
 * @{
 */
/**
 * \file
 * TCP data queue header (license: GPLv2)
 *
 * \author Roland Riegel
 */

struct tcp_queue;

struct tcp_queue* tcp_queue_alloc();
void tcp_queue_free(struct tcp_queue* queue);

uint16_t tcp_queue_put_rx(struct tcp_queue* queue, const uint8_t* data, uint16_t data_len);
uint16_t tcp_queue_get_rx(struct tcp_queue* queue, uint8_t* buffer, uint16_t buffer_len);
uint16_t tcp_queue_peek_rx(const struct tcp_queue* queue, uint8_t* buffer, uint16_t offset, uint16_t length);
uint16_t tcp_queue_skip_rx(struct tcp_queue* queue, uint16_t len);

uint16_t tcp_queue_used_rx(const struct tcp_queue* queue);
uint16_t tcp_queue_space_rx(const struct tcp_queue* queue);
uint8_t* tcp_queue_used_buffer_rx(struct tcp_queue* queue);

uint16_t tcp_queue_put_tx(struct tcp_queue* queue, const uint8_t* data, uint16_t data_len);
uint16_t tcp_queue_get_tx(struct tcp_queue* queue, uint8_t* buffer, uint16_t buffer_len);
uint16_t tcp_queue_peek_tx(const struct tcp_queue* queue, uint8_t* buffer, uint16_t offset, uint16_t length);
uint16_t tcp_queue_skip_tx(struct tcp_queue* queue, uint16_t len);

uint16_t tcp_queue_space_tx(const struct tcp_queue* queue);

uint8_t* tcp_queue_space_buffer_tx(struct tcp_queue* queue);

uint16_t tcp_queue_reserve_tx(struct tcp_queue* queue, uint16_t len);

uint16_t tcp_queue_used_tx(const struct tcp_queue* queue);

/**
 * @}
 * @}
 * @}
 */

#endif

