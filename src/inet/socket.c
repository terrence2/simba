/**
 * @file socket.c
 * @version 0.3.0
 *
 * @section License
 * Copyright (C) 2016, Erik Moqvist
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERSOCKTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * This file is part of the Simba project.
 */

#include "simba.h"

#include "inet.h"

#include "lwip/tcp.h"
#include "lwip/udp.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_common.h"

extern xSemaphoreHandle thrd_idle_sem;

/*
 * All callback functions below are called from the LwIP-thread. For
 * ESP, this is the FreeRTOS LwIP-thread.
 */

/**
 * An UDP packet has been received.
 */
static void on_udp_recv(void *arg_p,
                        struct udp_pcb *pcb_p,
                        struct pbuf *pbuf_p,
                        ip_addr_t *addr_p,
                        uint16_t port)
{
    struct socket_t *socket_p = arg_p;

    if (socket_p->recv.thrd_p != NULL) {
        if (pbuf_p->tot_len < socket_p->recv.size) {
            socket_p->recv.size = pbuf_p->tot_len;
        }

        /* Write the packet to the receive buffer. */
        pbuf_copy_partial(pbuf_p,
                          socket_p->recv.buf_p,
                          socket_p->recv.size,
                          0);

        /* Copy the remote address and port to the receiver. */
        socket_p->recv.remote_addr_p->ip = addr_p->addr;
        socket_p->recv.remote_addr_p->port = port;

        /* Resume the reading thread. */
        thrd_resume_isr(socket_p->recv.thrd_p, 0);
        socket_p->recv.thrd_p = NULL;
        xSemaphoreGive(thrd_idle_sem);
    }

    pbuf_free(pbuf_p);
}

/**
 * TCP data is available.
 */
static err_t on_tcp_recv(void *arg_p,
                         struct tcp_pcb *pcb_p,
                         struct pbuf *pbuf_p,
                         err_t err)
{
    size_t size, pbuf_len;
    struct socket_t *socket_p = arg_p;

    err = ERR_MEM;

    if (socket_p->recv.thrd_p != NULL) {
        pbuf_len = (pbuf_p->tot_len - socket_p->recv.pbuf_offset);

        if (pbuf_len > socket_p->recv.left) {
            size = socket_p->recv.left;
        } else {
            size = pbuf_len;
            err = ERR_OK;
        }

        /* Write the packet to the receive buffer. */
        pbuf_copy_partial(pbuf_p,
                          socket_p->recv.buf_p,
                          size,
                          socket_p->recv.pbuf_offset);
        socket_p->recv.left -= size;
        socket_p->recv.buf_p += size;
        socket_p->recv.pbuf_offset += size;
        tcp_recved(pcb_p, size);

        if (err == ERR_OK) {
            socket_p->recv.pbuf_offset = 0;
            pbuf_free(pbuf_p);
        }

        /* Resume the reading thread when all data has been
           received. */
        if (socket_p->recv.left == 0) {
            thrd_resume_isr(socket_p->recv.thrd_p, 0);
            socket_p->recv.thrd_p = NULL;
            xSemaphoreGive(thrd_idle_sem);
        }
    }

    return (err);
}

/**
 * TCP data has been sent.
 */
static err_t on_tcp_sent(void *arg_p,
                         struct tcp_pcb *pcb_p,
                         uint16_t len)
{
    struct socket_t *socket_p = arg_p;

    if (socket_p->recv.thrd_p != NULL) {
        /* Resume the waiting thread. */
        thrd_resume_isr(socket_p->recv.thrd_p, 0);
        socket_p->recv.thrd_p = NULL;
        xSemaphoreGive(thrd_idle_sem);
    }

    return (0);
}

/**
 * A TCP client is trying to connect.
 */
static err_t on_tcp_accept(void *arg_p,
                           struct tcp_pcb *new_pcb_p,
                           err_t err)
{
    struct socket_t *socket_p = arg_p;

    if (socket_p->recv.thrd_p != NULL) {
        /* Save the new pcb. */
        socket_p->recv.buf_p = new_pcb_p;

        /* Resume the waiting thread. */
        thrd_resume_isr(socket_p->recv.thrd_p, 0);
        socket_p->recv.thrd_p = NULL;
        xSemaphoreGive(thrd_idle_sem);
    }

    return (0);
}

static void init(struct socket_t *self_p,
                 int type,
                 void *pcb_p)
{
    /* Channel functions. */
    self_p->base.read = (thrd_read_fn_t)socket_read;
    self_p->base.write = (thrd_write_fn_t)socket_write;
    self_p->base.size = (thrd_size_fn_t)NULL;

    self_p->type = type;
    self_p->pcb_p = pcb_p;
    self_p->recv.pbuf_offset = 0;
}

int socket_open(struct socket_t *self_p,
                int domain,
                int type,
                int protocol)
{
    void *pcb_p = NULL;

    switch (type) {

    case SOCKET_TYPE_STREAM:
        /* Create and initiate the TCP pcb. */
        pcb_p = tcp_new();
        tcp_arg(pcb_p, self_p);
        tcp_recv(pcb_p, on_tcp_recv);
        tcp_sent(pcb_p, on_tcp_sent);
        tcp_nagle_disable((struct tcp_pcb *)pcb_p);
        break;

    case SOCKET_TYPE_DGRAM:
        /* Create and initiate the UDP pcb. */
        pcb_p = udp_new();
        udp_recv(pcb_p, on_udp_recv, self_p);
        break;

    default:
        return (-1);
    }

    init(self_p, type, pcb_p);

    return (0);
}

int socket_close(struct socket_t *self_p)
{
    switch (self_p->type) {

    case SOCKET_TYPE_STREAM:
        tcp_output(self_p->pcb_p);
        tcp_close(self_p->pcb_p);
        break;

    case SOCKET_TYPE_DGRAM:
        udp_recv(self_p->pcb_p, NULL, NULL);
        udp_remove(self_p->pcb_p);
        break;

    default:
        return (-1);
    }

    return (0);
}

int socket_bind(struct socket_t *self_p,
                const struct socket_addr_t *local_addr_p,
                size_t addrlen)
{
    ip_addr_t ip;

    ip.addr = local_addr_p->ip;

    switch (self_p->type) {

    case SOCKET_TYPE_STREAM:
        return (tcp_bind(self_p->pcb_p, &ip, local_addr_p->port));

    case SOCKET_TYPE_DGRAM:
        return (udp_bind(self_p->pcb_p, &ip, local_addr_p->port));

    default:
        return (-1);
    }
}

int socket_listen(struct socket_t *self_p, int backlog)
{
    switch (self_p->type) {

    case SOCKET_TYPE_STREAM:
        self_p->pcb_p = tcp_listen_with_backlog(self_p->pcb_p, backlog);
        tcp_accept(self_p->pcb_p, on_tcp_accept);
        break;

    default:
        return (-1);
    }

    return (0);
}

int socket_connect(struct socket_t *self_p,
                   const struct socket_addr_t *addr_p,
                   size_t addrlen)
{
    ip_addr_t ip;

    ip.addr = addr_p->ip;

    switch (self_p->type) {

    case SOCKET_TYPE_STREAM:
        return (tcp_connect(self_p->pcb_p, &ip, addr_p->port, NULL));

    case SOCKET_TYPE_DGRAM:
        return (udp_connect(self_p->pcb_p, &ip, addr_p->port));

    default:
        return (-1);
    }

}

int socket_accept(struct socket_t *self_p,
                  struct socket_t *accepted_p,
                  struct socket_addr_t *addr_p,
                  size_t *addrlen_p)
{
    /* The receive callback copies the data to the receive buffer. */
    self_p->recv.thrd_p = thrd_self();

    /* Wait for a client. Resumed from on_tcp_accept(). */
    thrd_suspend(NULL);

    /* Initialize the new socket. */
    init(accepted_p, SOCKET_TYPE_STREAM, self_p->recv.buf_p);
    tcp_arg(accepted_p->pcb_p, accepted_p);
    tcp_recv(accepted_p->pcb_p, on_tcp_recv);
    tcp_sent(accepted_p->pcb_p, on_tcp_sent);
    tcp_nagle_disable((struct tcp_pcb *)accepted_p->pcb_p);

    return (0);
}

ssize_t socket_sendto(struct socket_t *self_p,
                      const void *buf_p,
                      size_t size,
                      int flags,
                      const struct socket_addr_t *remote_addr_p,
                      size_t addrlen)
{
    ssize_t res = size;
    struct pbuf *pbuf_p;
    ip_addr_t ip;

    ip.addr = remote_addr_p->ip;
    pbuf_p = pbuf_alloc(PBUF_TRANSPORT, size, PBUF_RAM);
    memcpy(pbuf_p->payload, buf_p, size);

    if (udp_sendto(self_p->pcb_p,
                   pbuf_p,
                   &ip,
                   remote_addr_p->port) != 0) {
        res = -1;
    }

    pbuf_free(pbuf_p);

    return (res);
}

ssize_t socket_recvfrom(struct socket_t *self_p,
                        void *buf_p,
                        size_t size,
                        int flags,
                        struct socket_addr_t *remote_addr_p,
                        size_t addrlen)
{
    /* The receive callback copies the data to the receive buffer. */
    self_p->recv.buf_p = buf_p;
    self_p->recv.size = size;
    self_p->recv.left = size;
    self_p->recv.remote_addr_p = remote_addr_p;
    self_p->recv.thrd_p = thrd_self();

    /* Wait for data. Resumed from on_udp_recv(). */
    thrd_suspend(NULL);

    return (self_p->recv.size);
}

ssize_t socket_write(struct socket_t *self_p,
                     const void *buf_p,
                     size_t size)
{
    self_p->recv.thrd_p = thrd_self();

    tcp_write(self_p->pcb_p, buf_p, size, 0);
    tcp_output(self_p->pcb_p);

    /* Wait for data. Resumed from on_tcp_sent(). */
    thrd_suspend(NULL);

    return (size);
}

ssize_t socket_read(struct socket_t *self_p,
                    void *buf_p,
                    size_t size)
{
    return (socket_recvfrom(self_p, buf_p, size, 0, NULL, 0));
}
