#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "CuTest.h"

#include "linked_list_queue.h"

#include "raft.h"
#include "raft_log.h"
#include "raft_private.h"

static int __logentry_get_node_id(
    raft_server_t* raft,
    void *udata,
    raft_entry_t *ety,
    raft_index_t ety_idx
    )
{
    return 0;
}

static int __log_offer(
    raft_server_t* raft,
    void *user_data,
    raft_entry_t *entry,
    raft_index_t entry_idx
    )
{
    CuAssertIntEquals((CuTest*)raft, 1, entry_idx);
    return 0;
}

static int __log_pop(
    raft_server_t* raft,
    void *user_data,
    raft_entry_t *entry,
    raft_index_t entry_idx
    )
{
    raft_entry_t* copy = malloc(sizeof(*entry));
    memcpy(copy, entry, sizeof(*entry));
    llqueue_offer(user_data, copy);
    return 0;
}

static int __log_pop_failing(
    raft_server_t* raft,
    void *user_data,
    raft_entry_t *entry,
    raft_index_t entry_idx
    )
{
    return -1;
}

raft_cbs_t funcs = {
    .log_pop = __log_pop,
    .log_get_node_id = __logentry_get_node_id
};

void* __set_up()
{
    void* queue = llqueue_new();
    void *r = raft_new();
    raft_set_callbacks(r, &funcs, queue);
    return r;
}

void TestLog_new_is_empty(CuTest * tc)
{
    void *l;

    l = log_new();
    CuAssertTrue(tc, 0 == log_count(l));
}

void TestLog_append_is_not_empty(CuTest * tc)
{
    void *l;
    raft_entry_t e;

    void *r = raft_new();

    memset(&e, 0, sizeof(raft_entry_t));

    e.id = 1;

    l = log_new();
    raft_cbs_t funcs = {
        .log_offer = __log_offer
    };
    log_set_callbacks(l, &funcs, r);
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e));
    CuAssertIntEquals(tc, 1, log_count(l));
}

void TestLog_get_at_idx(CuTest * tc)
{
    void *l;
    raft_entry_t e1, e2, e3;

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));

    l = log_new();
    e1.id = 1;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    e2.id = 2;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));
    e3.id = 3;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e3));
    CuAssertIntEquals(tc, 3, log_count(l));
    CuAssertIntEquals(tc, e1.id, log_get_at_idx(l, 1)->id);
    CuAssertIntEquals(tc, e2.id, log_get_at_idx(l, 2)->id);
    CuAssertIntEquals(tc, e3.id, log_get_at_idx(l, 3)->id);
}

void TestLog_get_at_idx_returns_null_where_out_of_bounds(CuTest * tc)
{
    void *l;
    raft_entry_t e1;

    memset(&e1, 0, sizeof(raft_entry_t));

    l = log_new();
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 0));
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 1));

    e1.id = 1;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 2));
}

void TestLog_delete(CuTest * tc)
{
    void *l;
    raft_entry_t e1, e2, e3;

    void* queue = llqueue_new();
    void *r = raft_new();
    raft_cbs_t funcs = {
        .log_pop = __log_pop,
        .log_get_node_id = __logentry_get_node_id
    };
    raft_set_callbacks(r, &funcs, queue);

    l = log_new();
    log_set_callbacks(l, &funcs, r);

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));

    e1.id = 1;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    e2.id = 2;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));
    e3.id = 3;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e3));
    CuAssertIntEquals(tc, 3, log_count(l));
    CuAssertIntEquals(tc, 3, log_get_current_idx(l));

    log_delete(l, 3);
    CuAssertIntEquals(tc, 2, log_count(l));
    CuAssertIntEquals(tc, e3.id, ((raft_entry_t*)llqueue_poll(queue))->id);
    CuAssertIntEquals(tc, 2, log_count(l));
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 3));

    log_delete(l, 2);
    CuAssertIntEquals(tc, 1, log_count(l));
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 2));

    log_delete(l, 1);
    CuAssertIntEquals(tc, 0, log_count(l));
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 1));
}

void TestLog_delete_onwards(CuTest * tc)
{
    void* queue = llqueue_new();
    void *r = raft_new();
    raft_cbs_t funcs = {
        .log_pop = __log_pop,
        .log_get_node_id = __logentry_get_node_id
    };
    raft_set_callbacks(r, &funcs, queue);

    void *l;
    raft_entry_t e1, e2, e3;

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));

    l = log_new();
    log_set_callbacks(l, &funcs, r);
    e1.id = 1;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    e2.id = 2;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));
    e3.id = 3;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e3));
    CuAssertIntEquals(tc, 3, log_count(l));

    /* even 3 gets deleted */
    log_delete(l, 2);
    CuAssertIntEquals(tc, 1, log_count(l));
    CuAssertIntEquals(tc, e1.id, log_get_at_idx(l, 1)->id);
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 2));
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 3));
}

void TestLog_delete_handles_log_pop_failure(CuTest * tc)
{
    void *l;
    raft_entry_t e1, e2, e3;

    void* queue = llqueue_new();
    void *r = raft_new();
    raft_cbs_t funcs = {
        .log_pop = __log_pop_failing,
        .log_get_node_id = __logentry_get_node_id
    };
    raft_set_callbacks(r, &funcs, queue);

    l = log_new();
    log_set_callbacks(l, &funcs, r);

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));

    e1.id = 1;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    e2.id = 2;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));
    e3.id = 3;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e3));
    CuAssertIntEquals(tc, 3, log_count(l));
    CuAssertIntEquals(tc, 3, log_get_current_idx(l));

    CuAssertIntEquals(tc, -1, log_delete(l, 3));
    CuAssertIntEquals(tc, 3, log_count(l));
    CuAssertIntEquals(tc, 3, log_count(l));
    CuAssertIntEquals(tc, e3.id, ((raft_entry_t*)log_peektail(l))->id);
 }

void TestLog_delete_fails_for_idx_zero(CuTest * tc)
{
    void* queue = llqueue_new();
    void *r = raft_new();
    raft_cbs_t funcs = {
        .log_pop = __log_pop,
        .log_get_node_id = __logentry_get_node_id
    };
    raft_set_callbacks(r, &funcs, queue);

    void *l;
    raft_entry_t e1, e2, e3, e4;

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));
    memset(&e4, 0, sizeof(raft_entry_t));

    e1.id = 1;
    e2.id = 2;
    e3.id = 3;
    e4.id = 4;

    l = log_alloc(1);
    log_set_callbacks(l, &funcs, r);
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e3));
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e4));
    CuAssertIntEquals(tc, log_delete(l, 0), -1);
}

void TestLog_poll(CuTest * tc)
{
    void* queue = llqueue_new();
    void *r = raft_new();
    raft_cbs_t funcs = {
        .log_pop = __log_pop,
        .log_get_node_id = __logentry_get_node_id
    };
    raft_set_callbacks(r, &funcs, queue);

    void *l;
    raft_entry_t e1, e2, e3;

    l = log_new();
    log_set_callbacks(l, &funcs, r);

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));

    e1.id = 1;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    CuAssertIntEquals(tc, 1, log_get_current_idx(l));

    e2.id = 2;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));
    CuAssertIntEquals(tc, 2, log_get_current_idx(l));

    e3.id = 3;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e3));
    CuAssertIntEquals(tc, 3, log_count(l));
    CuAssertIntEquals(tc, 3, log_get_current_idx(l));

    raft_entry_t *ety;

    /* remove 1st */
    ety = NULL;
    CuAssertIntEquals(tc, log_poll(l, (void*)&ety), 0);
    CuAssertTrue(tc, NULL != ety);
    CuAssertIntEquals(tc, 2, log_count(l));
    CuAssertIntEquals(tc, ety->id, 1);
    CuAssertIntEquals(tc, 1, log_get_base(l));
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 1));
    CuAssertTrue(tc, NULL != log_get_at_idx(l, 2));
    CuAssertTrue(tc, NULL != log_get_at_idx(l, 3));
    CuAssertIntEquals(tc, 3, log_get_current_idx(l));

    /* remove 2nd */
    ety = NULL;
    CuAssertIntEquals(tc, log_poll(l, (void*)&ety), 0);
    CuAssertTrue(tc, NULL != ety);
    CuAssertIntEquals(tc, 1, log_count(l));
    CuAssertIntEquals(tc, ety->id, 2);
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 1));
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 2));
    CuAssertTrue(tc, NULL != log_get_at_idx(l, 3));
    CuAssertIntEquals(tc, 3, log_get_current_idx(l));

    /* remove 3rd */
    ety = NULL;
    CuAssertIntEquals(tc, log_poll(l, (void*)&ety), 0);
    CuAssertTrue(tc, NULL != ety);
    CuAssertIntEquals(tc, 0, log_count(l));
    CuAssertIntEquals(tc, ety->id, 3);
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 1));
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 2));
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 3));
    CuAssertIntEquals(tc, 3, log_get_current_idx(l));
}

void TestLog_peektail(CuTest * tc)
{
    void *l;
    raft_entry_t e1, e2, e3;

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));

    l = log_new();
    e1.id = 1;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    e2.id = 2;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));
    e3.id = 3;
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e3));
    CuAssertIntEquals(tc, 3, log_count(l));
    CuAssertIntEquals(tc, e3.id, log_peektail(l)->id);
}

#if 0
// TODO: duplicate testing not implemented yet
void T_estlog_cant_append_duplicates(CuTest * tc)
{
    void *l;
    raft_entry_t e;

    e.id = 1;

    l = log_new();
    CuAssertTrue(tc, 1 == log_append_entry(l, &e));
    CuAssertTrue(tc, 1 == log_count(l));
}
#endif

void TestLog_load_from_snapshot(CuTest * tc)
{
    void *l;
    raft_entry_t e1, e2, e3;

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));

    l = log_new();
    CuAssertIntEquals(tc, 0, log_get_current_idx(l));
    CuAssertIntEquals(tc, 0, log_load_from_snapshot(l, 10, 5));
    CuAssertIntEquals(tc, 10, log_get_current_idx(l));
    CuAssertIntEquals(tc, 0, log_count(l));
}

void TestLog_load_from_snapshot_clears_log(CuTest * tc)
{
    void *l;
    raft_entry_t e1, e2, e3;

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));

    l = log_new();

    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));
    CuAssertIntEquals(tc, 2, log_count(l));
    CuAssertIntEquals(tc, 2, log_get_current_idx(l));

    CuAssertIntEquals(tc, 0, log_load_from_snapshot(l, 10, 5));
    CuAssertIntEquals(tc, 0, log_count(l));
    CuAssertIntEquals(tc, 10, log_get_current_idx(l));
}

void TestLog_front_pushes_across_boundary(CuTest * tc)
{
    void* r = __set_up();

    void *l;
    raft_entry_t e1, e2, e3;

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));

    e1.id = 1;
    e2.id = 2;
    e3.id = 3;

    l = log_alloc(1);
    log_set_callbacks(l, &funcs, r);

    raft_entry_t* ety;

    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    CuAssertIntEquals(tc, log_poll(l, (void*)&ety), 0);
    CuAssertIntEquals(tc, ety->id, 1);
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));
    CuAssertIntEquals(tc, log_poll(l, (void*)&ety), 0);
    CuAssertIntEquals(tc, ety->id, 2);
}

void TestLog_front_and_back_pushed_across_boundary_with_enlargement_required(CuTest * tc)
{
    void *l;
    raft_entry_t e1, e2, e3, e4;

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));
    memset(&e4, 0, sizeof(raft_entry_t));

    e1.id = 1;
    e2.id = 2;
    e3.id = 3;
    e4.id = 4;

    l = log_alloc(1);

    raft_entry_t* ety;

    /* append */
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));

    /* poll */
    CuAssertIntEquals(tc, log_poll(l, (void*)&ety), 0);
    CuAssertIntEquals(tc, ety->id, 1);

    /* append */
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));

    /* poll */
    CuAssertIntEquals(tc, log_poll(l, (void*)&ety), 0);
    CuAssertIntEquals(tc, ety->id, 2);

    /* append append */
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e3));
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e4));

    /* poll */
    CuAssertIntEquals(tc, log_poll(l, (void*)&ety), 0);
    CuAssertIntEquals(tc, ety->id, 3);
}

void TestLog_delete_after_polling(CuTest * tc)
{
    void *l;
    raft_entry_t e1, e2, e3, e4;

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));
    memset(&e4, 0, sizeof(raft_entry_t));

    e1.id = 1;
    e2.id = 2;
    e3.id = 3;
    e4.id = 4;

    l = log_alloc(1);

    raft_entry_t* ety;

    /* append */
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    CuAssertIntEquals(tc, 1, log_count(l));

    /* poll */
    CuAssertIntEquals(tc, log_poll(l, (void*)&ety), 0);
    CuAssertIntEquals(tc, ety->id, 1);
    CuAssertIntEquals(tc, 0, log_count(l));

    /* append */
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));
    CuAssertIntEquals(tc, 1, log_count(l));

    /* poll */
    CuAssertIntEquals(tc, log_delete(l, 1), 0);
    CuAssertIntEquals(tc, 0, log_count(l));
}

void TestLog_delete_after_polling_from_double_append(CuTest * tc)
{
    void* queue = llqueue_new();
    void *r = raft_new();
    raft_cbs_t funcs = {
        .log_pop = __log_pop,
        .log_get_node_id = __logentry_get_node_id
    };
    raft_set_callbacks(r, &funcs, queue);

    void *l;
    raft_entry_t e1, e2, e3, e4;

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));
    memset(&e4, 0, sizeof(raft_entry_t));

    e1.id = 1;
    e2.id = 2;
    e3.id = 3;
    e4.id = 4;

    l = log_alloc(1);
    log_set_callbacks(l, &funcs, r);

    raft_entry_t* ety;

    /* append append */
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));
    CuAssertIntEquals(tc, 2, log_count(l));

    /* poll */
    CuAssertIntEquals(tc, log_poll(l, (void*)&ety), 0);
    CuAssertIntEquals(tc, ety->id, 1);
    CuAssertIntEquals(tc, 1, log_count(l));

    /* append */
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e3));
    CuAssertIntEquals(tc, 2, log_count(l));

    /* poll */
    CuAssertIntEquals(tc, log_delete(l, 1), 0);
    CuAssertIntEquals(tc, 0, log_count(l));
}

void TestLog_get_from_idx_with_base_off_by_one(CuTest * tc)
{
    void* queue = llqueue_new();
    void *r = raft_new();
    raft_cbs_t funcs = {
        .log_pop = __log_pop,
        .log_get_node_id = __logentry_get_node_id
    };
    raft_set_callbacks(r, &funcs, queue);

    void *l;
    raft_entry_t e1, e2;

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));

    e1.id = 1;
    e2.id = 2;

    l = log_alloc(1);
    log_set_callbacks(l, &funcs, r);

    raft_entry_t* ety;

    /* append append */
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));
    CuAssertIntEquals(tc, 2, log_count(l));

    /* poll */
    CuAssertIntEquals(tc, log_poll(l, (void*)&ety), 0);
    CuAssertIntEquals(tc, ety->id, 1);
    CuAssertIntEquals(tc, 1, log_count(l));

    /* get off-by-one index */
    int n_etys;
    CuAssertPtrEquals(tc, log_get_from_idx(l, 1, &n_etys), NULL);
    CuAssertIntEquals(tc, n_etys, 0);

    /* now get the correct index */
    ety = log_get_from_idx(l, 2, &n_etys);
    CuAssertPtrNotNull(tc, ety);
    CuAssertIntEquals(tc, n_etys, 1);
    CuAssertIntEquals(tc, ety->id, 2);
}

void TestLog_delete_with_idx_below_base(CuTest * tc)
{
    void* queue = llqueue_new();
    void *r = raft_new();
    raft_cbs_t funcs = {
        .log_pop = __log_pop,
        .log_get_node_id = __logentry_get_node_id
    };
    raft_set_callbacks(r, &funcs, queue);

    void *l;
    raft_entry_t e1, e2, e3;

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));

    e1.id = 1;
    e2.id = 2;
    e3.id = 3;

    l = log_new();
    log_set_callbacks(l, &funcs, r);
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e3));

    /* poll first entry so base=1 */
    raft_entry_t *ety;
    CuAssertIntEquals(tc, 0, log_poll(l, (void*)&ety));
    CuAssertIntEquals(tc, 1, log_get_base(l));
    CuAssertIntEquals(tc, 2, log_count(l));

    /* delete with idx=1 which is at/below base — should delete all remaining */
    CuAssertIntEquals(tc, 0, log_delete(l, 1));
    CuAssertIntEquals(tc, 0, log_count(l));
}

void TestLog_delete_with_idx_beyond_current(CuTest * tc)
{
    void* queue = llqueue_new();
    void *r = raft_new();
    raft_cbs_t funcs = {
        .log_pop = __log_pop,
        .log_get_node_id = __logentry_get_node_id
    };
    raft_set_callbacks(r, &funcs, queue);

    void *l;
    raft_entry_t e1, e2;

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));

    e1.id = 1;
    e2.id = 2;

    l = log_new();
    log_set_callbacks(l, &funcs, r);
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));
    CuAssertIntEquals(tc, 2, log_count(l));

    /* delete with idx beyond current entries — no-op */
    CuAssertIntEquals(tc, 0, log_delete(l, 10));
    CuAssertIntEquals(tc, 2, log_count(l));
}

void TestLog_poll_on_empty_log(CuTest * tc)
{
    void *l;
    raft_entry_t *ety = NULL;

    l = log_new();
    CuAssertIntEquals(tc, 0, log_count(l));

    /* polling empty log returns -1 */
    CuAssertIntEquals(tc, -1, log_poll(l, (void*)&ety));
    CuAssertIntEquals(tc, 0, log_count(l));
}

void TestLog_poll_all_entries_then_poll_empty(CuTest * tc)
{
    void *l;
    raft_entry_t e1, e2;

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));

    e1.id = 1;
    e2.id = 2;

    l = log_new();
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));
    CuAssertIntEquals(tc, 2, log_count(l));

    raft_entry_t *ety;

    CuAssertIntEquals(tc, 0, log_poll(l, (void*)&ety));
    CuAssertIntEquals(tc, 1, ety->id);
    CuAssertIntEquals(tc, 0, log_poll(l, (void*)&ety));
    CuAssertIntEquals(tc, 2, ety->id);
    CuAssertIntEquals(tc, 0, log_count(l));

    /* now poll on empty — should fail */
    CuAssertIntEquals(tc, -1, log_poll(l, (void*)&ety));
    CuAssertIntEquals(tc, 0, log_count(l));
    CuAssertIntEquals(tc, 2, log_get_current_idx(l));
}

void TestLog_get_at_idx_after_polling(CuTest * tc)
{
    void *l;
    raft_entry_t e1, e2, e3;

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));

    e1.id = 1;
    e2.id = 2;
    e3.id = 3;

    l = log_new();
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e3));

    raft_entry_t *ety;
    CuAssertIntEquals(tc, 0, log_poll(l, (void*)&ety));
    CuAssertIntEquals(tc, 1, log_get_base(l));

    /* idx 1 is now below base — should return NULL */
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 1));
    /* idx 2 and 3 still accessible */
    CuAssertIntEquals(tc, 2, log_get_at_idx(l, 2)->id);
    CuAssertIntEquals(tc, 3, log_get_at_idx(l, 3)->id);
    /* idx 4 beyond range */
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 4));
}

void TestLog_circular_buffer_wraparound(CuTest * tc)
{
    void *l;

    /* alloc with small capacity to force wraparound */
    l = log_alloc(3);

    raft_entry_t entries[6];
    int i;
    for (i = 0; i < 6; i++)
    {
        memset(&entries[i], 0, sizeof(raft_entry_t));
        entries[i].id = i + 1;
    }

    /* fill to capacity */
    CuAssertIntEquals(tc, 0, log_append_entry(l, &entries[0]));
    CuAssertIntEquals(tc, 0, log_append_entry(l, &entries[1]));
    CuAssertIntEquals(tc, 0, log_append_entry(l, &entries[2]));
    CuAssertIntEquals(tc, 3, log_count(l));

    /* poll two to advance front */
    raft_entry_t *ety;
    CuAssertIntEquals(tc, 0, log_poll(l, (void*)&ety));
    CuAssertIntEquals(tc, 1, ety->id);
    CuAssertIntEquals(tc, 0, log_poll(l, (void*)&ety));
    CuAssertIntEquals(tc, 2, ety->id);
    CuAssertIntEquals(tc, 1, log_count(l));
    CuAssertIntEquals(tc, 2, log_get_base(l));

    /* append two more — these wrap around in the circular buffer */
    CuAssertIntEquals(tc, 0, log_append_entry(l, &entries[3]));
    CuAssertIntEquals(tc, 0, log_append_entry(l, &entries[4]));
    CuAssertIntEquals(tc, 3, log_count(l));

    /* verify all remaining entries are correct */
    CuAssertIntEquals(tc, 3, log_get_at_idx(l, 3)->id);
    CuAssertIntEquals(tc, 4, log_get_at_idx(l, 4)->id);
    CuAssertIntEquals(tc, 5, log_get_at_idx(l, 5)->id);

    /* polled entries should not be accessible */
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 1));
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 2));
}

void TestLog_count_after_mixed_poll_append(CuTest * tc)
{
    void *l;
    raft_entry_t e1, e2, e3, e4;

    memset(&e1, 0, sizeof(raft_entry_t));
    memset(&e2, 0, sizeof(raft_entry_t));
    memset(&e3, 0, sizeof(raft_entry_t));
    memset(&e4, 0, sizeof(raft_entry_t));

    e1.id = 1;
    e2.id = 2;
    e3.id = 3;
    e4.id = 4;

    l = log_new();
    CuAssertIntEquals(tc, 0, log_count(l));

    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    CuAssertIntEquals(tc, 1, log_count(l));

    CuAssertIntEquals(tc, 0, log_append_entry(l, &e2));
    CuAssertIntEquals(tc, 2, log_count(l));

    raft_entry_t *ety;
    CuAssertIntEquals(tc, 0, log_poll(l, (void*)&ety));
    CuAssertIntEquals(tc, 1, log_count(l));

    CuAssertIntEquals(tc, 0, log_append_entry(l, &e3));
    CuAssertIntEquals(tc, 2, log_count(l));

    CuAssertIntEquals(tc, 0, log_poll(l, (void*)&ety));
    CuAssertIntEquals(tc, 1, log_count(l));

    CuAssertIntEquals(tc, 0, log_poll(l, (void*)&ety));
    CuAssertIntEquals(tc, 0, log_count(l));

    CuAssertIntEquals(tc, 0, log_append_entry(l, &e4));
    CuAssertIntEquals(tc, 1, log_count(l));
    CuAssertIntEquals(tc, 4, log_get_current_idx(l));
}

void TestLog_new_from_base(CuTest * tc)
{
    void *l;
    raft_entry_t e1;

    memset(&e1, 0, sizeof(raft_entry_t));
    e1.id = 1;

    l = log_new();

    /* load from snapshot sets base without adding entries */
    CuAssertIntEquals(tc, 0, log_load_from_snapshot(l, 5, 2));
    CuAssertIntEquals(tc, 0, log_count(l));
    CuAssertIntEquals(tc, 5, log_get_current_idx(l));
    CuAssertIntEquals(tc, 5, log_get_base(l));

    /* appending after base works — entry gets idx 6 */
    CuAssertIntEquals(tc, 0, log_append_entry(l, &e1));
    CuAssertIntEquals(tc, 1, log_count(l));
    CuAssertIntEquals(tc, 6, log_get_current_idx(l));
    CuAssertIntEquals(tc, 1, log_get_at_idx(l, 6)->id);

    /* indices below base return NULL */
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 1));
    CuAssertTrue(tc, NULL == log_get_at_idx(l, 5));
}

void TestLog_capacity_growth_multiple_reallocs(CuTest * tc)
{
    void *l;

    /* start with capacity 2 — will need multiple reallocs */
    l = log_alloc(2);

    raft_entry_t entries[20];
    int i;
    for (i = 0; i < 20; i++)
    {
        memset(&entries[i], 0, sizeof(raft_entry_t));
        entries[i].id = i + 1;
    }

    /* append 20 entries — triggers realloc at 2, 4, 8, 16 */
    for (i = 0; i < 20; i++)
    {
        CuAssertIntEquals(tc, 0, log_append_entry(l, &entries[i]));
        CuAssertIntEquals(tc, i + 1, log_count(l));
    }

    /* verify all entries are still accessible and correct */
    for (i = 0; i < 20; i++)
    {
        raft_entry_t *ety = log_get_at_idx(l, i + 1);
        CuAssertPtrNotNull(tc, ety);
        CuAssertIntEquals(tc, i + 1, ety->id);
    }

    CuAssertIntEquals(tc, 20, log_count(l));
    CuAssertIntEquals(tc, 20, log_get_current_idx(l));
}

void TestLog_peektail_on_empty_log(CuTest * tc)
{
    void *l;

    l = log_new();
    CuAssertTrue(tc, NULL == log_peektail(l));
}
