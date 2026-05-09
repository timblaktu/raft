#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "CuTest.h"

#include "raft.h"
#include "raft_log.h"
#include "raft_private.h"

void TestRaft_is_voting_by_default(CuTest * tc)
{
    raft_node_t *p = raft_node_new((void*)1, 1);
    CuAssertTrue(tc, raft_node_is_voting(p));
}

void TestRaft_node_set_nextIdx(CuTest * tc)
{
    raft_node_t *p = raft_node_new((void*)1, 1);
    raft_node_set_next_idx(p, 3);
    CuAssertTrue(tc, 3 == raft_node_get_next_idx(p));
}

void TestRaft_node_get_next_idx_default_is_1(CuTest * tc)
{
    raft_node_t *p = raft_node_new((void*)1, 1);
    CuAssertTrue(tc, 1 == raft_node_get_next_idx(p));
}

void TestRaft_node_set_nextIdx_clamped_to_1(CuTest * tc)
{
    raft_node_t *p = raft_node_new((void*)1, 1);
    raft_node_set_next_idx(p, 0);
    CuAssertTrue(tc, 1 == raft_node_get_next_idx(p));
    raft_node_set_next_idx(p, -5);
    CuAssertTrue(tc, 1 == raft_node_get_next_idx(p));
}

void TestRaft_node_get_id(CuTest * tc)
{
    raft_node_t *p = raft_node_new((void*)1, 42);
    CuAssertTrue(tc, 42 == raft_node_get_id(p));
}

void TestRaft_node_get_id_zero(CuTest * tc)
{
    raft_node_t *p = raft_node_new((void*)1, 0);
    CuAssertTrue(tc, 0 == raft_node_get_id(p));
}

void TestRaft_node_udata_round_trip(CuTest * tc)
{
    int data1 = 100;
    int data2 = 200;
    raft_node_t *p = raft_node_new(&data1, 1);
    CuAssertTrue(tc, &data1 == raft_node_get_udata(p));
    raft_node_set_udata(p, &data2);
    CuAssertTrue(tc, &data2 == raft_node_get_udata(p));
}

void TestRaft_node_udata_null(CuTest * tc)
{
    raft_node_t *p = raft_node_new(NULL, 1);
    CuAssertTrue(tc, NULL == raft_node_get_udata(p));
}

void TestRaft_node_match_idx_default_is_0(CuTest * tc)
{
    raft_node_t *p = raft_node_new((void*)1, 1);
    CuAssertTrue(tc, 0 == raft_node_get_match_idx(p));
}

void TestRaft_node_set_match_idx(CuTest * tc)
{
    raft_node_t *p = raft_node_new((void*)1, 1);
    raft_node_set_match_idx(p, 5);
    CuAssertTrue(tc, 5 == raft_node_get_match_idx(p));
}

void TestRaft_node_active_by_default(CuTest * tc)
{
    raft_node_t *p = raft_node_new((void*)1, 1);
    CuAssertTrue(tc, 1 == raft_node_is_active(p));
}

void TestRaft_node_set_active(CuTest * tc)
{
    raft_node_t *p = raft_node_new((void*)1, 1);
    raft_node_set_active(p, 0);
    CuAssertTrue(tc, 0 == raft_node_is_active(p));
    raft_node_set_active(p, 1);
    CuAssertTrue(tc, 1 == raft_node_is_active(p));
}

void TestRaft_node_voting_committed_default_is_0(CuTest * tc)
{
    raft_node_t *p = raft_node_new((void*)1, 1);
    CuAssertTrue(tc, 0 == raft_node_is_voting_committed(p));
}

void TestRaft_node_set_voting_committed(CuTest * tc)
{
    raft_node_t *p = raft_node_new((void*)1, 1);
    raft_node_set_voting_committed(p, 1);
    CuAssertTrue(tc, 1 == raft_node_is_voting_committed(p));
    raft_node_set_voting_committed(p, 0);
    CuAssertTrue(tc, 0 == raft_node_is_voting_committed(p));
}

void TestRaft_node_addition_committed_default_is_0(CuTest * tc)
{
    raft_node_t *p = raft_node_new((void*)1, 1);
    CuAssertTrue(tc, 0 == raft_node_is_addition_committed(p));
}

void TestRaft_node_set_addition_committed(CuTest * tc)
{
    raft_node_t *p = raft_node_new((void*)1, 1);
    raft_node_set_addition_committed(p, 1);
    CuAssertTrue(tc, 1 == raft_node_is_addition_committed(p));
    raft_node_set_addition_committed(p, 0);
    CuAssertTrue(tc, 0 == raft_node_is_addition_committed(p));
}

void TestRaft_node_has_sufficient_logs_default_is_0(CuTest * tc)
{
    raft_node_t *p = raft_node_new((void*)1, 1);
    CuAssertTrue(tc, 0 == raft_node_has_sufficient_logs(p));
}

void TestRaft_node_set_has_sufficient_logs(CuTest * tc)
{
    raft_node_t *p = raft_node_new((void*)1, 1);
    raft_node_set_has_sufficient_logs(p);
    CuAssertTrue(tc, 1 == raft_node_has_sufficient_logs(p));
}

void TestRaft_node_set_voting_toggle(CuTest * tc)
{
    raft_node_t *p = raft_node_new((void*)1, 1);
    /* starts voting */
    CuAssertTrue(tc, 1 == raft_node_is_voting(p));
    /* make non-voting */
    raft_node_set_voting(p, 0);
    CuAssertTrue(tc, 0 == raft_node_is_voting(p));
    /* make voting again */
    raft_node_set_voting(p, 1);
    CuAssertTrue(tc, 1 == raft_node_is_voting(p));
}

void TestRaft_node_vote_for_me(CuTest * tc)
{
    raft_node_t *p = raft_node_new((void*)1, 1);
    CuAssertTrue(tc, 0 == raft_node_has_vote_for_me(p));
    raft_node_vote_for_me(p, 1);
    CuAssertTrue(tc, 1 == raft_node_has_vote_for_me(p));
    raft_node_vote_for_me(p, 0);
    CuAssertTrue(tc, 0 == raft_node_has_vote_for_me(p));
}

void TestRaft_node_free(CuTest * tc)
{
    raft_node_t *p = raft_node_new((void*)1, 1);
    /* just ensure no crash */
    raft_node_free(p);
}

void TestRaft_node_flags_independent(CuTest * tc)
{
    /* Setting one flag should not affect others */
    raft_node_t *p = raft_node_new((void*)1, 1);

    /* Set all flags */
    raft_node_vote_for_me(p, 1);
    raft_node_set_has_sufficient_logs(p);
    raft_node_set_active(p, 0);
    raft_node_set_voting_committed(p, 1);
    raft_node_set_addition_committed(p, 1);

    /* Verify all set correctly */
    CuAssertTrue(tc, 1 == raft_node_is_voting(p));
    CuAssertTrue(tc, 1 == raft_node_has_vote_for_me(p));
    CuAssertTrue(tc, 1 == raft_node_has_sufficient_logs(p));
    CuAssertTrue(tc, 0 == raft_node_is_active(p));
    CuAssertTrue(tc, 1 == raft_node_is_voting_committed(p));
    CuAssertTrue(tc, 1 == raft_node_is_addition_committed(p));

    /* Clear one flag and verify others unchanged */
    raft_node_vote_for_me(p, 0);
    CuAssertTrue(tc, 0 == raft_node_has_vote_for_me(p));
    CuAssertTrue(tc, 1 == raft_node_is_voting(p));
    CuAssertTrue(tc, 1 == raft_node_has_sufficient_logs(p));
    CuAssertTrue(tc, 0 == raft_node_is_active(p));
    CuAssertTrue(tc, 1 == raft_node_is_voting_committed(p));
    CuAssertTrue(tc, 1 == raft_node_is_addition_committed(p));
}
