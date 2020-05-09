
/*
 * Copyright (c) 2006-2008 by Roland Riegel <feedback@roland-riegel.de>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "tcp_config.h"
#include "tcp_queue.h"

#include <string.h>

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
 * TCP data queue implementation (license: GPLv2)
 *
 * \author Roland Riegel
 */

/**
 * \internal
 * A ring buffer.
 *
 * This struct just contains the header. The data area directly follows
 * in memory. Its end is determined from the \a end pointer.
 */
struct tcp_queue_head
{
    /** Points to the ring buffer byte to be read next. */
    uint8_t* pos;
    /** Points behind the end of the data area of the ring buffer. */
    uint8_t* end;
    /** Contains the number of bytes currently contained in the buffer. */
    uint16_t used;
};

/**
 * \internal
 * A TCP data queue.
 *
 * Each queue contains two ring buffers, one for the transmit and
 * one for the receive direction.
 *
 * \note This struct is an opaque type to the outside world.
 */
struct tcp_queue
{
    /** The receive ring header. */
    struct tcp_queue_head rx_head;
    /** The receive buffer. */
    uint8_t rx_buffer[ TCP_RECEIVE_BUFFER_SIZE ];

    /** The transmit ring header. */
    struct tcp_queue_head tx_head;
    /** The transmit buffer. */
    uint8_t tx_buffer[ TCP_TRANSMIT_BUFFER_SIZE ];
};

static struct tcp_queue tcp_queue_queues[ TCP_MAX_CONNECTION_COUNT ];

static uint16_t tcp_queue_put(struct tcp_queue_head* queue_head, const uint8_t* data, uint16_t data_len);
static uint16_t tcp_queue_get(struct tcp_queue_head* queue_head, uint8_t* buffer, uint16_t buffer_len);
static uint16_t tcp_queue_peek(const struct tcp_queue_head* queue_head, uint8_t* buffer, uint16_t offset, uint16_t length);
static uint16_t tcp_queue_skip(struct tcp_queue_head* queue_head, uint16_t len);
static uint16_t tcp_queue_reserve(struct tcp_queue_head* queue_head, uint16_t len);
static uint8_t* tcp_queue_used_buffer(struct tcp_queue_head* queue_head);
static uint8_t* tcp_queue_space_buffer(struct tcp_queue_head* queue_head);
static uint16_t tcp_queue_used(const struct tcp_queue_head* queue_head);
static uint16_t tcp_queue_space(const struct tcp_queue_head* queue_head);

/**
 * \internal
 * Allocates a data queue.
 *
 * \returns The allocated data queue.
 */
struct tcp_queue* tcp_queue_alloc()
{
    struct tcp_queue* queue = &tcp_queue_queues[0];
    for(; queue < &tcp_queue_queues[TCP_MAX_CONNECTION_COUNT]; ++queue)
    {
        if(queue->rx_head.pos == 0 && queue->tx_head.pos == 0)
        {
            queue->rx_head.pos = &queue->rx_buffer[0];
            queue->rx_head.end = queue->rx_head.pos + sizeof(queue->rx_buffer);
            queue->rx_head.used = 0;

            queue->tx_head.pos = &queue->tx_buffer[0];
            queue->tx_head.end = queue->tx_head.pos + sizeof(queue->tx_buffer);
            queue->tx_head.used = 0;

            return queue;
        }
    }

    return 0;
}

/**
 * \internal
 * Deallocates a data queue.
 *
 * \param[in] queue A pointer to the data queue to be deallocated.
 */
void tcp_queue_free(struct tcp_queue* queue)
{
    if(!queue)
        return;

    queue->rx_head.pos = 0;
    queue->tx_head.pos = 0;
}

/**
 * \internal
 * Puts data into the receive buffer of a queue.
 *
 * \param[in] queue A pointer to the data queue.
 * \param[in] data A pointer to the buffer containing the data to be queued.
 * \param[in] data_len The number of bytes to be queued.
 * \returns The number of bytes actually queued.
 */
uint16_t tcp_queue_put_rx(struct tcp_queue* queue, const uint8_t* data, uint16_t data_len)
{
    return tcp_queue_put(&queue->rx_head, data, data_len);
}

/**
 * \internal
 * Reads data from the receive buffer of a queue.
 *
 * \param[in] queue A pointer to the data queue.
 * \param[out] buffer A pointer to the buffer which gets filled with the data.
 * \param[in] buffer_len The maximum number of bytes to read.
 * \returns The number of bytes actually read.
 */
uint16_t tcp_queue_get_rx(struct tcp_queue* queue, uint8_t* buffer, uint16_t buffer_len)
{
    return tcp_queue_get(&queue->rx_head, buffer, buffer_len);
}

/**
 * \internal
 * Reads data from the receive buffer of a queue without actually removing it from the queue.
 *
 * \param[in] queue A pointer to the data queue.
 * \param[out] buffer A pointer to the buffer which gets filled with the data.
 * \param[in] offset Skip the first \a offset bytes before starting to read.
 * \param[in] length The maximum number of bytes to read.
 * \returns The number of bytes actually read.
 */
uint16_t tcp_queue_peek_rx(const struct tcp_queue* queue, uint8_t* buffer, uint16_t offset, uint16_t length)
{
    return tcp_queue_peek(&queue->rx_head, buffer, offset, length);
}

/**
 * \internal
 * Removes data from the receive buffer of a queue without reading it.
 *
 * \param[in] queue A pointer to the data queue.
 * \param[in] len The maximum number of bytes to remove.
 * \returns The number of bytes actually removed.
 */
uint16_t tcp_queue_skip_rx(struct tcp_queue* queue, uint16_t len)
{
    return tcp_queue_skip(&queue->rx_head, len);
}

/**
 * \internal
 * Retrieves the number of bytes available in the receive buffer of a queue.
 *
 * \param[in] queue A pointer to the data queue.
 * \returns The number of bytes occupied of the receive buffer.
 */
uint16_t tcp_queue_used_rx(const struct tcp_queue* queue)
{
    return tcp_queue_used(&queue->rx_head);
}

/**
 * \internal
 * Retrieves the number of free bytes in the receive buffer of a queue.
 *
 * \param[in] queue A pointer to the data queue.
 * \returns The number of free bytes in the receive buffer.
 */
uint16_t tcp_queue_space_rx(const struct tcp_queue* queue)
{
    return tcp_queue_space(&queue->rx_head);
}

/**
 * \internal
 * Puts data into the transmit buffer of a queue.
 *
 * \param[in] queue A pointer to the data queue.
 * \param[in] data A pointer to the buffer containing the data to be queued.
 * \param[in] data_len The number of bytes to be queued.
 * \returns The number of bytes actually queued.
 */
uint16_t tcp_queue_put_tx(struct tcp_queue* queue, const uint8_t* data, uint16_t data_len)
{
    return tcp_queue_put(&queue->tx_head, data, data_len);
}

/**
 * \internal
 * Reads data from the transmit buffer of a queue.
 *
 * \param[in] queue A pointer to the data queue.
 * \param[out] buffer A pointer to the buffer which gets filled with the data.
 * \param[in] buffer_len The maximum number of bytes to read.
 * \returns The number of bytes actually read.
 */
uint16_t tcp_queue_get_tx(struct tcp_queue* queue, uint8_t* buffer, uint16_t buffer_len)
{
    return tcp_queue_get(&queue->tx_head, buffer, buffer_len);
}

/**
 * \internal
 * Reads data from the transmit buffer of a queue without actually removing it.
 *
 * \param[in] queue A pointer to the data queue.
 * \param[out] buffer A pointer to the buffer which gets filled with the data.
 * \param[in] offset Skip the first \a offset bytes before starting to read.
 * \param[in] length The maximum number of bytes to read.
 * \returns The number of bytes actually read.
 */
uint16_t tcp_queue_peek_tx(const struct tcp_queue* queue, uint8_t* buffer, uint16_t offset, uint16_t length)
{
    return tcp_queue_peek(&queue->tx_head, buffer, offset, length);
}

/**
 * \internal
 * Removes data from the transmit buffer of a queue without reading it.
 *
 * \param[in] queue A pointer to the data queue.
 * \param[in] len The maximum number of bytes to remove.
 * \returns The number of bytes actually removed.
 */
uint16_t tcp_queue_skip_tx(struct tcp_queue* queue, uint16_t len)
{
    return tcp_queue_skip(&queue->tx_head, len);
}

/**
 * \internal
 * Declares unused transmit buffer space as valid.
 *
 * This is only useful if tcp_queue_space_buffer_tx() has been used
 * before to retrieve and fill the unused buffer space with valid data.
 *
 * \param[in] queue A pointer to the data queue.
 * \param[in] len The maximum number of bytes to declare as valid.
 * \returns The number of bytes which have been declared as valid.
 */
uint16_t tcp_queue_reserve_tx(struct tcp_queue* queue, uint16_t len)
{
    return tcp_queue_reserve(&queue->tx_head, len);
}

/**
 * \internal
 * Retrieves a pointer to the occupied part of the receive buffer of a queue.
 *
 * The buffer is guaranteed to be linear and contiguous. Its size is
 * retrieved with tcp_queue_used_rx().
 *
 * \note This function can be quite expensive, especially with full ring buffers.
 *
 * \param[in] queue A pointer to the data queue.
 * \returns A pointer to the occupied receive buffer space on success, \c NULL on receive buffer empty.
 */
uint8_t* tcp_queue_used_buffer_rx(struct tcp_queue* queue)
{
    return tcp_queue_used_buffer(&queue->rx_head);
}

/**
 * \internal
 * Retrieves a pointer to the empty part of the transmit buffer of a queue.
 *
 * The buffer is guaranteed to be linear and contiguous. Its size is
 * retrieved with tcp_queue_space_tx(). After writing to the buffer,
 * be sure to call tcp_queue_reserve_tx() to actually add the data to
 * the transmit buffer's content.
 *
 * \attention Make sure the transmit buffer is not modified while the
 * empty buffer space is being used.
 *
 * \param[in] queue A pointer to the data queue.
 * \returns A pointer to the empty transmit buffer space on success, \c NULL on transmit buffer full.
 */
uint8_t* tcp_queue_space_buffer_tx(struct tcp_queue* queue)
{
    return tcp_queue_space_buffer(&queue->tx_head);
}

/**
 * \internal
 * Retrieves the number of bytes available in the transmit buffer of a queue.
 *
 * \param[in] queue A pointer to the data queue.
 * \returns The number of bytes occupied of the transmit buffer.
 */
uint16_t tcp_queue_used_tx(const struct tcp_queue* queue)
{
    return tcp_queue_used(&queue->tx_head);
}

/**
 * \internal
 * Retrieves the number of free bytes in the transmit buffer of a queue.
 *
 * \param[in] queue A pointer to the data queue.
 * \returns The number of free bytes in the transmit buffer.
 */
uint16_t tcp_queue_space_tx(const struct tcp_queue* queue)
{
    return tcp_queue_space(&queue->tx_head);
}

/**
 * \internal
 * Removes all data from the receive and transmit buffers of a queue.
 *
 * \param[in] queue A pointer to the queue to clear.
 */
void tcp_queue_clear(struct tcp_queue* queue)
{
    if(!queue)
        return;

    queue->rx_head.used = 0;
    queue->tx_head.used = 0;
}

/**
 * Puts data into a ring buffer.
 *
 * \param[in] queue_head A pointer to the ring buffer.
 * \param[in] data A pointer to the data to put into the ring buffer.
 * \param[in] data_len The maximum number of bytes to put into the ring buffer.
 * \returns The number of bytes actually put into the ring buffer.
 */
uint16_t tcp_queue_put(struct tcp_queue_head* queue_head, const uint8_t* data, uint16_t data_len)
{
    if(!queue_head || !data || !data_len)
        return 0;

    uint16_t queue_left = tcp_queue_space(queue_head);
    if(data_len > queue_left)
        data_len = queue_left;

    uint8_t* data_pos = queue_head->pos + queue_head->used;
    uint16_t data_left = data_len;
    while(data_left--)
    {
        if(data_pos >= queue_head->end)
            data_pos -= queue_head->end - (uint8_t*) (queue_head + 1);
        else if(data_pos < (uint8_t*) (queue_head + 1))
            data_pos += (uintptr_t) (queue_head + 1);

        *data_pos++ = *data++;
    }

    queue_head->used += data_len;

    return data_len;
}

/**
 * Reads data from a ring buffer.
 *
 * \param[in] queue_head A pointer to the ring buffer.
 * \param[out] buffer A pointer to the buffer into which the data gets written.
 * \param[in] buffer_len The maximum number of bytes to read from the ring buffer.
 * \returns The number of bytes actually read from the ring buffer.
 */
uint16_t tcp_queue_get(struct tcp_queue_head* queue_head, uint8_t* buffer, uint16_t buffer_len)
{
    buffer_len = tcp_queue_peek(queue_head, buffer, 0, buffer_len);
    tcp_queue_skip(queue_head, buffer_len);

    return buffer_len;
}

/**
 * Reads data from a ring buffer without removing it.
 *
 * \param[in] queue_head A pointer to the ring buffer.
 * \param[out] buffer A pointer to the buffer into which the data gets written.
 * \param[in] offset Skip the first \a offset bytes before starting to read.
 * \param[in] length The maximum number of bytes to read.
 * \returns The number of bytes actually read from the ring buffer.
 */
uint16_t tcp_queue_peek(const struct tcp_queue_head* queue_head, uint8_t* buffer, uint16_t offset, uint16_t length)
{
    if(!queue_head || !buffer || !length || queue_head->used <= offset)
        return 0;

    if(queue_head->used < offset + length)
        length = queue_head->used - offset;

    const uint8_t* data_pos = queue_head->pos + offset;
    uint16_t data_left = length;
    while(data_left--)
    {
        if(data_pos >= queue_head->end)
            data_pos -= queue_head->end - (uint8_t*) (queue_head + 1);
        else if(data_pos < (uint8_t*) (queue_head + 1))
            data_pos += (uintptr_t) (queue_head + 1);

        *buffer++ = *data_pos++;
    }

    return length;
}

/**
 * Removes data from a ring buffer without reading it.
 *
 * \param[in] queue_head A pointer to the ring buffer.
 * \param[in] len The maximum number of bytes to remove.
 * \returns The number of bytes actually removed.
 */
uint16_t tcp_queue_skip(struct tcp_queue_head* queue_head, uint16_t len)
{
    if(!queue_head || len < 1)
        return 0;

    if(len > queue_head->used)
    {
        len = queue_head->used;
        queue_head->used = 0;

        return len;
    }

    queue_head->used -= len;
    queue_head->pos += len;
    if(queue_head->pos >= queue_head->end)
        queue_head->pos -= queue_head->end - (uint8_t*) (queue_head + 1);
    else if(queue_head->pos < (uint8_t*) (queue_head + 1))
        queue_head->pos += (uintptr_t) (queue_head + 1);

    return len;
}

/**
 * Adds unoccupied bytes to the valid data area of a ring buffer.
 *
 * Enlarges the amount of data contained in the ring buffer by \a len bytes. The
 * additional data should have been written to the buffer before calling this
 * function. Access to the buffer is provided by tcp_queue_space_buffer().
 *
 * \param[in] queue_head A pointer to the ring buffer.
 * \param[in] len The maximum number of bytes to add.
 * \returns The number of bytes added to the ring buffer.
 */
uint16_t tcp_queue_reserve(struct tcp_queue_head* queue_head, uint16_t len)
{
    if(!queue_head)
        return 0;

    uint16_t space = tcp_queue_space(queue_head);
    if(len > space)
        len = space;

    queue_head->used += len;
    return len;
}

/**
 * Retrieves the occupied part of a ring buffer.
 *
 * The buffer is guaranteed to be linear and contiguous. Its size is
 * retrieved with tcp_queue_used().
 *
 * \note This function can be quite expensive, especially with full ring buffers.
 *
 * \param[in] queue_head A pointer to the ring buffer.
 * \returns A pointer to the occupied buffer space on success, \c NULL on ring buffer empty.
 */
uint8_t* tcp_queue_used_buffer(struct tcp_queue_head* queue_head)
{
    if(!queue_head)
        return 0;

    if(queue_head->used < 1)
        return 0;

    /* rotate data until it is linear and contiguous */
    uint8_t* buffer = (uint8_t*) (queue_head + 1);
    uint16_t space = tcp_queue_space(queue_head);
    if(space >= 32)
    {
        /* Use the ring buffer's unused space to rotate the content.
         * As this doesn't work if the ring buffer is full and is
         * very inefficient for nearly full buffers, we only do it
         * that way if the unused space is larger than 32 bytes.
         */
        while(queue_head->end - queue_head->pos < queue_head->used)
        {
            uint16_t bytes_end = queue_head->end - queue_head->pos;
            uint16_t bytes_begin = queue_head->used - bytes_end;

            if(space > bytes_begin)
                space = bytes_begin;

            memmove(queue_head->pos - space, queue_head->pos, bytes_end);
            memcpy(queue_head->end - space, buffer, space);
            memmove(buffer, buffer + space, bytes_begin - space);

            queue_head->pos -= space;
        }
    }
    else
    {
        /* The ring buffer is full, we must use an extra stack
         * buffer for rotating. This is also much faster for
         * nearly-full ring buffers.
         */
        uint8_t copy_buffer[32];
        uint16_t copy_buffer_used;
        while(queue_head->pos != buffer)
        {
            copy_buffer_used = queue_head->pos - buffer;
            if(copy_buffer_used > sizeof(copy_buffer))
                copy_buffer_used = sizeof(copy_buffer);

            memcpy(copy_buffer, buffer, copy_buffer_used);
            memmove(buffer, buffer + copy_buffer_used, queue_head->end - buffer - copy_buffer_used);
            memcpy(queue_head->end - copy_buffer_used, copy_buffer, copy_buffer_used);

            queue_head->pos -= copy_buffer_used;
        }
    }

    return queue_head->pos;
}

/**
 * Retrieves the empty part of a ring buffer.
 *
 * The buffer is guaranteed to be linear and contiguous. Its size is
 * retrieved with tcp_queue_space(). After writing to the buffer,
 * be sure to call tcp_queue_reserve() to actually add the data to
 * the ring buffer's content.
 *
 * \attention Make sure the ring buffer is not modified while the
 * empty buffer space is being used.
 *
 * \param[in] queue_head A pointer to the ring buffer.
 * \returns A pointer to the empty buffer space on success, \c NULL on ring buffer full.
 */
uint8_t* tcp_queue_space_buffer(struct tcp_queue_head* queue_head)
{
    if(!queue_head)
        return 0;

    uint16_t space = tcp_queue_space(queue_head);
    if(space < 1)
        return 0;

    if(queue_head->used < 1)
        queue_head->pos = (uint8_t*) (queue_head + 1);

    if(queue_head->pos == (uint8_t*) (queue_head + 1))
    {
        return queue_head->pos + queue_head->used;
    }
    else if(queue_head->pos + queue_head->used == queue_head->end)
    {
        return (uint8_t*) (queue_head + 1);
    }
    else if(queue_head->used < queue_head->end - queue_head->pos)
    {
        /* rearrange memory */
        uint8_t* from = queue_head->pos;
        uint8_t* to = (uint8_t*) (queue_head + 1);
        uint16_t used = queue_head->used;

        while(used-- > 0)
            *to++ = *from++;
        queue_head->pos = (uint8_t*) (queue_head + 1);

        return queue_head->pos + queue_head->used;
    }
    else
    {
        return queue_head->pos - space;
    }
}

/**
 * Retrieves the number of bytes available in a ring buffer.
 *
 * \param[in] queue_head A pointer to the ring buffer.
 * \returns The number of bytes occupied in the ring buffer.
 */
uint16_t tcp_queue_used(const struct tcp_queue_head* queue_head)
{
    if(!queue_head)
        return 0;

    return queue_head->used;
}

/**
 * Retrieves the number of free bytes in a ring buffer.
 *
 * \param[in] queue_head A pointer to the ring buffer.
 * \returns The number of free bytes in the ring buffer.
 */
uint16_t tcp_queue_space(const struct tcp_queue_head* queue_head)
{
    if(!queue_head)
        return 0;

    return queue_head->end - (uint8_t*) (queue_head + 1) - queue_head->used;
}

/**
 * @}
 * @}
 * @}
 */

