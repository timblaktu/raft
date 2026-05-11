#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <stdbool.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "CuTest.h"

#include "linked_list_queue.h"

#include "raft.h"
#include "mock_send_functions.h"

typedef struct msg_t msg_t;

typedef struct
{
    void* outbox;
    void* inbox;
    void* raft;
    /* track messages polled via sender_poll_msg_data for cleanup */
    msg_t** polled;
    int npolled;
} sender_t;

struct msg_t
{
    void* data;
    int len;
    /* what type of message is it? */
    int type;
    /* who sent this? */
    raft_node_t* sender;
};

static sender_t** __senders = NULL;
static int __nsenders = 0;

void senders_new()
{
    __senders = NULL;
    __nsenders = 0;
}

static void __free_msg(msg_t* m)
{
    if (m->type == RAFT_MSG_APPENDENTRIES)
    {
        msg_appendentries_t* ae = m->data;
        if (ae->entries)
            free(ae->entries);
    }
    free(m->data);
    free(m);
}

static void __free_msg_queue(void* queue)
{
    msg_t* m;
    while ((m = llqueue_poll(queue)))
        __free_msg(m);
    llqueue_free(queue);
}

void sender_free(void* s)
{
    sender_t* me = s;
    __free_msg_queue(me->outbox);
    __free_msg_queue(me->inbox);
    int i;
    for (i = 0; i < me->npolled; i++)
        __free_msg(me->polled[i]);
    free(me->polled);
    free(me);
}

void senders_free()
{
    int i;
    for (i = 0; i < __nsenders; i++)
        sender_free(__senders[i]);
    free(__senders);
    __senders = NULL;
    __nsenders = 0;
}

static int __append_msg(
    sender_t* me,
    void* data,
    int type,
    int len,
    raft_node_t* node,
    raft_server_t* raft
    )
{
    msg_t* m = malloc(sizeof(msg_t));
    m->type = type;
    m->len = len;
    m->data = malloc(len);
    m->sender = raft_get_node(raft, raft_get_nodeid(raft));
    memcpy(m->data, data, len);
    llqueue_offer(me->outbox, m);

    /* give to peer */
    sender_t* peer = raft_node_get_udata(node);
    if (peer)
    {
        msg_t* m2 = malloc(sizeof(msg_t));
        m2->type = m->type;
        m2->len = m->len;
        m2->data = malloc(len);
        memcpy(m2->data, m->data, len);
        m2->sender = raft_get_node(peer->raft, raft_get_nodeid(raft));
        /* deep copy entries for AE messages so each msg owns its data */
        if (type == RAFT_MSG_APPENDENTRIES)
        {
            msg_appendentries_t* ae = m2->data;
            if (ae->n_entries > 0 && ae->entries)
            {
                msg_entry_t* ecopy = malloc(sizeof(msg_entry_t) * ae->n_entries);
                memcpy(ecopy, ae->entries, sizeof(msg_entry_t) * ae->n_entries);
                ae->entries = ecopy;
            }
        }
        llqueue_offer(peer->inbox, m2);
    }

    return 1;
}

int sender_requestvote(raft_server_t* raft,
                       void* udata, raft_node_t* node, msg_requestvote_t* msg)
{
    return __append_msg(udata, msg, RAFT_MSG_REQUESTVOTE, sizeof(*msg), node,
                        raft);
}

int sender_requestvote_response(raft_server_t* raft,
                                void* udata, raft_node_t* node,
                                msg_requestvote_response_t* msg)
{
    return __append_msg(udata, msg, RAFT_MSG_REQUESTVOTE_RESPONSE, sizeof(*msg),
                        node, raft);
}

int sender_appendentries(raft_server_t* raft,
                         void* udata, raft_node_t* node, msg_appendentries_t* msg)
{
    if (msg->n_entries > 0)
    {
        msg_entry_t* entries = calloc(1, sizeof(msg_entry_t) * msg->n_entries);
        memcpy(entries, msg->entries, sizeof(msg_entry_t) * msg->n_entries);
        msg->entries = entries;
    }
    else
    {
        msg->entries = NULL;
    }
    return __append_msg(udata, msg, RAFT_MSG_APPENDENTRIES, sizeof(*msg), node,
                        raft);
}

int sender_appendentries_response(raft_server_t* raft,
                                  void* udata, raft_node_t* node,
                                  msg_appendentries_response_t* msg)
{
    return __append_msg(udata, msg, RAFT_MSG_APPENDENTRIES_RESPONSE,
                        sizeof(*msg), node, raft);
}

int sender_entries_response(raft_server_t* raft,
                            void* udata, raft_node_t* node, msg_entry_response_t* msg)
{
    return __append_msg(udata, msg, RAFT_MSG_ENTRY_RESPONSE, sizeof(*msg), node,
                        raft);
}

void* sender_new(void* address)
{
    sender_t* me = malloc(sizeof(sender_t));
    me->outbox = llqueue_new();
    me->inbox = llqueue_new();
    me->polled = NULL;
    me->npolled = 0;
    __senders = realloc(__senders, sizeof(sender_t*) * (++__nsenders));
    __senders[__nsenders - 1] = me;
    return me;
}

void* sender_poll_msg_data(void* s)
{
    sender_t* me = s;
    msg_t* msg = llqueue_poll(me->outbox);
    if (!msg)
        return NULL;
    /* track for later cleanup in sender_free */
    me->polled = realloc(me->polled, sizeof(msg_t*) * (me->npolled + 1));
    me->polled[me->npolled++] = msg;
    return msg->data;
}

void sender_set_raft(void* s, void* r)
{
    sender_t* me = s;
    me->raft = r;
}

int sender_msgs_available(void* s)
{
    sender_t* me = s;

    return 0 < llqueue_count(me->inbox);
}

void sender_poll_msgs(void* s)
{
    sender_t* me = s;
    msg_t* m;

    while ((m = llqueue_poll(me->inbox)))
    {
        switch (m->type)
        {
        case RAFT_MSG_APPENDENTRIES:
        {
            msg_appendentries_response_t response;
            raft_recv_appendentries(me->raft, m->sender, m->data, &response);
            __append_msg(me, &response, RAFT_MSG_APPENDENTRIES_RESPONSE,
                         sizeof(response), m->sender, me->raft);
        }
        break;
        case RAFT_MSG_APPENDENTRIES_RESPONSE:
            raft_recv_appendentries_response(me->raft, m->sender, m->data);
            break;
        case RAFT_MSG_REQUESTVOTE:
        {
            msg_requestvote_response_t response;
            raft_recv_requestvote(me->raft, m->sender, m->data, &response);
            __append_msg(me, &response, RAFT_MSG_REQUESTVOTE_RESPONSE,
                         sizeof(response), m->sender, me->raft);
        }
        break;
        case RAFT_MSG_REQUESTVOTE_RESPONSE:
            raft_recv_requestvote_response(me->raft, m->sender, m->data);
            break;
        case RAFT_MSG_ENTRY:
        {
            msg_entry_response_t response;
            raft_recv_entry(me->raft, m->data, &response);
            __append_msg(me, &response, RAFT_MSG_ENTRY_RESPONSE,
                         sizeof(response), m->sender, me->raft);
        }
        break;

        case RAFT_MSG_ENTRY_RESPONSE:
#if 0
            raft_recv_entry_response(me->raft, m->sender, m->data);
#endif
            break;
        }

        if (m->type == RAFT_MSG_APPENDENTRIES)
        {
            msg_appendentries_t* ae = m->data;
            if (ae->entries)
                free(ae->entries);
        }
        free(m->data);
        free(m);
    }
}
