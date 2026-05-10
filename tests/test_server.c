
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
#include "mock_send_functions.h"

// TODO: leader doesn't timeout and cause election

static int __raft_persist_term(
    raft_server_t* raft,
    void *udata,
    raft_term_t term,
    int vote
    )
{
    return 0;
}

static int __raft_persist_vote(
    raft_server_t* raft,
    void *udata,
    int vote
    )
{
    return 0;
}

int __raft_applylog(
    raft_server_t* raft,
    void *udata,
    raft_entry_t *ety,
    raft_index_t idx
    )
{
    return 0;
}

int __raft_applylog_shutdown(
    raft_server_t* raft,
    void *udata,
    raft_entry_t *ety,
    raft_index_t idx
    )
{
    return RAFT_ERR_SHUTDOWN;
}

int __raft_send_requestvote(raft_server_t* raft,
                            void* udata,
                            raft_node_t* node,
                            msg_requestvote_t* msg)
{
    return 0;
}

static int __raft_send_appendentries(raft_server_t* raft,
                              void* udata,
                              raft_node_t* node,
                              msg_appendentries_t* msg)
{
    return 0;
}

static int __raft_log_get_node_id(raft_server_t* raft,
        void *udata,
        raft_entry_t *entry,
        raft_index_t entry_idx)
{
    return atoi(entry->data.buf);
}

static int __raft_log_offer(raft_server_t* raft,
        void* udata,
        raft_entry_t *entry,
        raft_index_t entry_idx)
{
    return 0;
}

static int __raft_node_has_sufficient_logs(
    raft_server_t* raft,
    void *user_data,
    raft_node_t* node)
{
    int *flag = (int*)user_data;
    *flag += 1;
    return 0;
}

raft_cbs_t generic_funcs = {
    .persist_term = __raft_persist_term,
    .persist_vote = __raft_persist_vote,
};

static int max_election_timeout(int election_timeout)
{
	return 2 * election_timeout;
}

void TestRaft_server_voted_for_records_who_we_voted_for(CuTest * tc)
{
    void *r = raft_new();
    raft_set_callbacks(r, &generic_funcs, NULL);
    raft_add_node(r, NULL, 2, 0);
    raft_vote(r, raft_get_node(r, 2));
    CuAssertTrue(tc, 2 == raft_get_voted_for(r));
}

void TestRaft_server_get_my_node(CuTest * tc)
{
    void *r = raft_new();
    raft_node_t* me = raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    CuAssertTrue(tc, me == raft_get_my_node(r));
}

void TestRaft_server_idx_starts_at_1(CuTest * tc)
{
    void *r = raft_new();
    CuAssertTrue(tc, 0 == raft_get_current_idx(r));

    raft_entry_t ety = {};
    ety.data.buf = "aaa";
    ety.data.len = 3;
    ety.id = 1;
    ety.term = 1;
    raft_append_entry(r, &ety);
    CuAssertTrue(tc, 1 == raft_get_current_idx(r));
}

void TestRaft_server_currentterm_defaults_to_0(CuTest * tc)
{
    void *r = raft_new();
    CuAssertTrue(tc, 0 == raft_get_current_term(r));
}

void TestRaft_server_set_currentterm_sets_term(CuTest * tc)
{
    void *r = raft_new();
    raft_set_callbacks(r, &generic_funcs, NULL);
    raft_set_current_term(r, 5);
    CuAssertTrue(tc, 5 == raft_get_current_term(r));
}

void TestRaft_server_voting_results_in_voting(CuTest * tc)
{
    void *r = raft_new();
    raft_set_callbacks(r, &generic_funcs, NULL);
    raft_add_node(r, NULL, 1, 0);
    raft_add_node(r, NULL, 9, 0);

    raft_vote(r, raft_get_node(r, 1));
    CuAssertTrue(tc, 1 == raft_get_voted_for(r));
    raft_vote(r, raft_get_node(r, 9));
    CuAssertTrue(tc, 9 == raft_get_voted_for(r));
}

void TestRaft_server_add_node_makes_non_voting_node_voting(CuTest * tc)
{
    void *r = raft_new();
    void* n1 = raft_add_non_voting_node(r, NULL, 9, 0);

    CuAssertTrue(tc, !raft_node_is_voting(n1));
    raft_add_node(r, NULL, 9, 0);
    CuAssertTrue(tc, raft_node_is_voting(n1));
    CuAssertIntEquals(tc, 1, raft_get_num_nodes(r));
}

void TestRaft_server_add_node_with_already_existing_id_is_not_allowed(CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 9, 0);
    raft_add_node(r, NULL, 11, 0);

    CuAssertTrue(tc, NULL == raft_add_node(r, NULL, 9, 0));
    CuAssertTrue(tc, NULL == raft_add_node(r, NULL, 11, 0));
}

void TestRaft_server_add_non_voting_node_with_already_existing_id_is_not_allowed(CuTest * tc)
{
    void *r = raft_new();
    raft_add_non_voting_node(r, NULL, 9, 0);
    raft_add_non_voting_node(r, NULL, 11, 0);

    CuAssertTrue(tc, NULL == raft_add_non_voting_node(r, NULL, 9, 0));
    CuAssertTrue(tc, NULL == raft_add_non_voting_node(r, NULL, 11, 0));
}

void TestRaft_server_add_non_voting_node_with_already_existing_voting_id_is_not_allowed(CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 9, 0);
    raft_add_node(r, NULL, 11, 0);

    CuAssertTrue(tc, NULL == raft_add_non_voting_node(r, NULL, 9, 0));
    CuAssertTrue(tc, NULL == raft_add_non_voting_node(r, NULL, 11, 0));
}

void TestRaft_server_remove_node(CuTest * tc)
{
    void *r = raft_new();
    void* n1 = raft_add_node(r, NULL, 1, 0);
    void* n2 = raft_add_node(r, NULL, 9, 0);

    raft_remove_node(r, n1);
    CuAssertTrue(tc, NULL == raft_get_node(r, 1));
    CuAssertTrue(tc, NULL != raft_get_node(r, 9));
    raft_remove_node(r, n2);
    CuAssertTrue(tc, NULL == raft_get_node(r, 9));
}

void TestRaft_election_start_increments_term(CuTest * tc)
{
    void *r = raft_new();
    raft_set_callbacks(r, &generic_funcs, NULL);
    raft_set_current_term(r, 1);
    raft_election_start(r);
    CuAssertTrue(tc, 2 == raft_get_current_term(r));
}

void TestRaft_set_state(CuTest * tc)
{
    void *r = raft_new();
    raft_set_state(r, RAFT_STATE_LEADER);
    CuAssertTrue(tc, RAFT_STATE_LEADER == raft_get_state(r));
}

void TestRaft_server_starts_as_follower(CuTest * tc)
{
    void *r = raft_new();
    CuAssertTrue(tc, RAFT_STATE_FOLLOWER == raft_get_state(r));
}

void TestRaft_server_starts_with_election_timeout_of_1000ms(CuTest * tc)
{
    void *r = raft_new();
    CuAssertTrue(tc, 1000 == raft_get_election_timeout(r));
}

void TestRaft_server_starts_with_request_timeout_of_200ms(CuTest * tc)
{
    void *r = raft_new();
    CuAssertTrue(tc, 200 == raft_get_request_timeout(r));
}

void TestRaft_server_entry_append_increases_logidx(CuTest* tc)
{
    raft_entry_t ety = {};
    char *str = "aaa";

    ety.data.buf = str;
    ety.data.len = 3;
    ety.id = 1;
    ety.term = 1;

    void *r = raft_new();
    CuAssertTrue(tc, 0 == raft_get_current_idx(r));
    raft_append_entry(r, &ety);
    CuAssertTrue(tc, 1 == raft_get_current_idx(r));
}

void TestRaft_server_append_entry_means_entry_gets_current_term(CuTest* tc)
{
    raft_entry_t ety = {};
    char *str = "aaa";

    ety.data.buf = str;
    ety.data.len = 3;
    ety.id = 1;
    ety.term = 1;

    void *r = raft_new();
    CuAssertTrue(tc, 0 == raft_get_current_idx(r));
    raft_append_entry(r, &ety);
    CuAssertTrue(tc, 1 == raft_get_current_idx(r));
}

void TestRaft_server_append_entry_is_retrievable(CuTest * tc)
{
    void *r = raft_new();
    raft_set_callbacks(r, &generic_funcs, NULL);
    raft_set_state(r, RAFT_STATE_CANDIDATE);

    raft_set_current_term(r, 5);
    raft_entry_t ety = {};
    ety.term = 1;
    ety.id = 100;
    ety.data.len = 4;
    ety.data.buf = (unsigned char*)"aaa";
    raft_append_entry(r, &ety);

    raft_entry_t* kept =  raft_get_entry_from_idx(r, 1);
    CuAssertTrue(tc, NULL != kept->data.buf);
    CuAssertIntEquals(tc, ety.data.len, kept->data.len);
    CuAssertTrue(tc, kept->data.buf == ety.data.buf);
}

static int __raft_logentry_offer(
    raft_server_t* raft,
    void *udata,
    raft_entry_t *ety,
    raft_index_t ety_idx
    )
{
    CuAssertIntEquals(udata, ety_idx, 1);
    ety->data.buf = udata;
    return 0;
}

void TestRaft_server_append_entry_user_can_set_data_buf(CuTest * tc)
{
    raft_cbs_t funcs = {
        .log_offer = __raft_logentry_offer,
        .persist_term = __raft_persist_term,
    };
    char *buf = "aaa";

    void *r = raft_new();
    raft_set_state(r, RAFT_STATE_CANDIDATE);
    raft_set_callbacks(r, &funcs, tc);
    raft_set_current_term(r, 5);
    raft_entry_t ety = {};
    ety.term = 1;
    ety.id = 100;
    ety.data.len = 4;
    ety.data.buf = buf;
    raft_append_entry(r, &ety);
    /* User's input entry is intact. */
    CuAssertTrue(tc, ety.data.buf == buf);
    raft_entry_t* kept =  raft_get_entry_from_idx(r, 1);
    CuAssertTrue(tc, NULL != kept->data.buf);
    /* Data buf is the one set by log_offer. */
    CuAssertTrue(tc, kept->data.buf == tc);
}

#if 0
/* TODO: no support for duplicate detection yet */
void
T_estRaft_server_append_entry_not_sucessful_if_entry_with_id_already_appended(
    CuTest* tc)
{
    void *r;
    raft_entry_t ety;
    char *str = "aaa";

    ety.data.buf = str;
    ety.data.len = 3;
    ety.id = 1;
    ety.term = 1;

    r = raft_new();
    CuAssertTrue(tc, 1 == raft_get_current_idx(r));
    raft_append_entry(r, &ety);
    raft_append_entry(r, &ety);
    CuAssertTrue(tc, 2 == raft_get_current_idx(r));

    /* different ID so we can be successful */
    ety.id = 2;
    raft_append_entry(r, &ety);
    CuAssertTrue(tc, 3 == raft_get_current_idx(r));
}
#endif

void TestRaft_server_entry_is_retrieveable_using_idx(CuTest* tc)
{
    raft_entry_t e1 = {};
    raft_entry_t e2 = {};
    raft_entry_t *ety_appended;
    char *str = "aaa";
    char *str2 = "bbb";

    void *r = raft_new();

    e1.term = 1;
    e1.id = 1;
    e1.data.buf = str;
    e1.data.len = 3;
    raft_append_entry(r, &e1);

    /* different ID so we can be successful */
    e2.term = 1;
    e2.id = 2;
    e2.data.buf = str2;
    e2.data.len = 3;
    raft_append_entry(r, &e2);

    CuAssertTrue(tc, NULL != (ety_appended = raft_get_entry_from_idx(r, 2)));
    CuAssertTrue(tc, !strncmp(ety_appended->data.buf, str2, 3));
}

void TestRaft_server_wont_apply_entry_if_we_dont_have_entry_to_apply(CuTest* tc)
{
    void *r = raft_new();
    raft_set_commit_idx(r, 0);
    raft_set_last_applied_idx(r, 0);

    raft_apply_entry(r);
    CuAssertTrue(tc, 0 == raft_get_last_applied_idx(r));
    CuAssertTrue(tc, 0 == raft_get_commit_idx(r));
}

void TestRaft_server_wont_apply_entry_if_there_isnt_a_majority(CuTest* tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);
    raft_set_commit_idx(r, 0);
    raft_set_last_applied_idx(r, 0);

    raft_apply_entry(r);
    CuAssertTrue(tc, 0 == raft_get_last_applied_idx(r));
    CuAssertTrue(tc, 0 == raft_get_commit_idx(r));

    char *str = "aaa";
    raft_entry_t ety = {};
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = str;
    ety.data.len = 3;
    raft_append_entry(r, &ety);
    raft_apply_entry(r);
    /* Not allowed to be applied because we haven't confirmed a majority yet */
    CuAssertTrue(tc, 0 == raft_get_last_applied_idx(r));
    CuAssertTrue(tc, 0 == raft_get_commit_idx(r));
}

/* If commitidx > lastApplied: increment lastApplied, apply log[lastApplied]
 * to state machine (�5.3) */
void TestRaft_server_increment_lastApplied_when_lastApplied_lt_commitidx(
    CuTest* tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .applylog = __raft_applylog,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    /* must be follower */
    raft_set_state(r, RAFT_STATE_FOLLOWER);
    raft_set_current_term(r, 1);
    raft_set_last_applied_idx(r, 0);

    /* need at least one entry */
    raft_entry_t ety = {};
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = "aaa";
    ety.data.len = 3;
    raft_append_entry(r, &ety);

    raft_set_commit_idx(r, 1);

    /* let time lapse */
    raft_periodic(r, 1);
    CuAssertIntEquals(tc, 1, raft_get_last_applied_idx(r));
}

void TestRaft_user_applylog_error_propogates_to_periodic(
    CuTest* tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .applylog = __raft_applylog_shutdown,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    /* must be follower */
    raft_set_state(r, RAFT_STATE_FOLLOWER);
    raft_set_current_term(r, 1);
    raft_set_last_applied_idx(r, 0);

    /* need at least one entry */
    raft_entry_t ety = {};
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = "aaa";
    ety.data.len = 3;
    raft_append_entry(r, &ety);

    raft_set_commit_idx(r, 1);

    /* let time lapse */
    CuAssertIntEquals(tc, RAFT_ERR_SHUTDOWN, raft_periodic(r, 1));
    CuAssertIntEquals(tc, 1, raft_get_last_applied_idx(r));
}

void TestRaft_server_apply_entry_increments_last_applied_idx(CuTest* tc)
{
    raft_cbs_t funcs = {
        .applylog = __raft_applylog,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);
    raft_set_last_applied_idx(r, 0);

    raft_entry_t ety = {};
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = "aaa";
    ety.data.len = 3;
    raft_append_entry(r, &ety);
    raft_set_commit_idx(r, 1);
    raft_apply_entry(r);
    CuAssertTrue(tc, 1 == raft_get_last_applied_idx(r));
}

void TestRaft_server_periodic_elapses_election_timeout(CuTest * tc)
{
    void *r = raft_new();
    /* we don't want to set the timeout to zero */
    raft_set_election_timeout(r, 1000);
    CuAssertTrue(tc, 0 == raft_get_timeout_elapsed(r));

    raft_periodic(r, 0);
    CuAssertTrue(tc, 0 == raft_get_timeout_elapsed(r));

    raft_periodic(r, 100);
    CuAssertTrue(tc, 100 == raft_get_timeout_elapsed(r));
}

void TestRaft_server_election_timeout_does_not_promote_us_to_leader_if_there_is_are_more_than_1_nodes(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
        .send_requestvote = __raft_send_requestvote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_election_timeout(r, 1000);

    /* clock over (ie. 1000 + 1), causing new election */
    raft_periodic(r, 1001);

    CuAssertTrue(tc, 0 == raft_is_leader(r));
}

void TestRaft_server_election_timeout_does_not_promote_us_to_leader_if_we_are_not_voting_node(CuTest * tc)
{
    void *r = raft_new();
    raft_add_non_voting_node(r, NULL, 1, 1);
    raft_set_election_timeout(r, 1000);

    /* clock over (ie. 1000 + 1), causing new election */
    raft_periodic(r, 1001);

    CuAssertTrue(tc, 0 == raft_is_leader(r));
    CuAssertTrue(tc, 0 == raft_get_current_term(r));
}

void TestRaft_server_election_timeout_does_not_start_election_if_there_are_no_voting_nodes(CuTest * tc)
{
    void *r = raft_new();
    raft_add_non_voting_node(r, NULL, 1, 1);
    raft_add_non_voting_node(r, NULL, 2, 0);
    raft_set_election_timeout(r, 1000);

    /* clock over (ie. 1000 + 1), causing new election */
    raft_periodic(r, 1001);

    CuAssertTrue(tc, 0 == raft_get_current_term(r));
}

void TestRaft_server_election_timeout_does_promote_us_to_leader_if_there_is_only_1_node(CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_set_election_timeout(r, 1000);

    /* clock over (ie. 1000 + 1), causing new election */
    raft_periodic(r, 1001);

    CuAssertTrue(tc, 1 == raft_is_leader(r));
}

void TestRaft_server_election_timeout_does_promote_us_to_leader_if_there_is_only_1_voting_node(CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries = __raft_send_appendentries,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_non_voting_node(r, NULL, 2, 0);
    raft_set_election_timeout(r, 1000);

    /* clock over (ie. 1000 + 1), causing new election */
    raft_periodic(r, 1001);

    CuAssertTrue(tc, 1 == raft_is_leader(r));
}

void TestRaft_server_recv_entry_auto_commits_if_we_are_the_only_node(CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_set_election_timeout(r, 1000);
    raft_become_leader(r);
    CuAssertTrue(tc, 0 == raft_get_commit_idx(r));

    /* entry message */
    msg_entry_t ety = {};
    ety.id = 1;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");

    /* receive entry */
    msg_entry_response_t cr;
    raft_recv_entry(r, &ety, &cr);
    CuAssertTrue(tc, 1 == raft_get_log_count(r));
    CuAssertTrue(tc, 1 == raft_get_commit_idx(r));
}

void TestRaft_server_recv_entry_fails_if_there_is_already_a_voting_change(CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_set_election_timeout(r, 1000);
    raft_become_leader(r);
    CuAssertTrue(tc, 0 == raft_get_commit_idx(r));

    /* entry message */
    msg_entry_t ety = {};
    ety.type = RAFT_LOGTYPE_ADD_NODE;
    ety.id = 1;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");

    /* receive entry */
    msg_entry_response_t cr;
    CuAssertTrue(tc, 0 == raft_recv_entry(r, &ety, &cr));
    CuAssertTrue(tc, 1 == raft_get_log_count(r));

    ety.id = 2;
    CuAssertTrue(tc, RAFT_ERR_ONE_VOTING_CHANGE_ONLY == raft_recv_entry(r, &ety, &cr));
    CuAssertTrue(tc, 1 == raft_get_commit_idx(r));
}

void TestRaft_server_cfg_sets_num_nodes(CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    CuAssertTrue(tc, 2 == raft_get_num_nodes(r));
}

void TestRaft_server_cant_get_node_we_dont_have(CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    CuAssertTrue(tc, NULL == raft_get_node(r, 0));
    CuAssertTrue(tc, NULL != raft_get_node(r, 1));
    CuAssertTrue(tc, NULL != raft_get_node(r, 2));
    CuAssertTrue(tc, NULL == raft_get_node(r, 3));
}

/* If term > currentTerm, set currentTerm to term (step down if candidate or
 * leader) */
void TestRaft_votes_are_majority_is_true(
    CuTest * tc
    )
{
    /* 1 of 3 = lose */
    CuAssertTrue(tc, 0 == raft_votes_is_majority(3, 1));

    /* 2 of 3 = win */
    CuAssertTrue(tc, 1 == raft_votes_is_majority(3, 2));

    /* 2 of 5 = lose */
    CuAssertTrue(tc, 0 == raft_votes_is_majority(5, 2));

    /* 3 of 5 = win */
    CuAssertTrue(tc, 1 == raft_votes_is_majority(5, 3));

    /* 2 of 1?? This is an error */
    CuAssertTrue(tc, 0 == raft_votes_is_majority(1, 2));
}

void TestRaft_server_recv_requestvote_response_dont_increase_votes_for_me_when_not_granted(
    CuTest * tc
    )
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);
    CuAssertTrue(tc, 0 == raft_get_nvotes_for_me(r));

    msg_requestvote_response_t rvr;
    memset(&rvr, 0, sizeof(msg_requestvote_response_t));
    rvr.term = 1;
    rvr.vote_granted = 0;
    int e = raft_recv_requestvote_response(r, raft_get_node(r, 2), &rvr);
    CuAssertIntEquals(tc, 0, e);
    CuAssertTrue(tc, 0 == raft_get_nvotes_for_me(r));
}

void TestRaft_server_recv_requestvote_response_dont_increase_votes_for_me_when_term_is_not_equal(
    CuTest * tc
    )
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 3);
    CuAssertTrue(tc, 0 == raft_get_nvotes_for_me(r));

    msg_requestvote_response_t rvr;
    memset(&rvr, 0, sizeof(msg_requestvote_response_t));
    rvr.term = 2;
    rvr.vote_granted = 1;
    raft_recv_requestvote_response(r, raft_get_node(r, 2), &rvr);
    CuAssertTrue(tc, 0 == raft_get_nvotes_for_me(r));
}

void TestRaft_server_recv_requestvote_response_increase_votes_for_me(
    CuTest * tc
    )
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
        .send_requestvote = __raft_send_requestvote,
        .send_appendentries = __raft_send_appendentries,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);
    CuAssertTrue(tc, 0 == raft_get_nvotes_for_me(r));
    CuAssertIntEquals(tc, 1, raft_get_current_term(r));

    raft_become_candidate(r);
    CuAssertIntEquals(tc, 2, raft_get_current_term(r));
    CuAssertTrue(tc, 1 == raft_get_nvotes_for_me(r));

    msg_requestvote_response_t rvr;
    memset(&rvr, 0, sizeof(msg_requestvote_response_t));
    rvr.term = 2;
    rvr.vote_granted = 1;
    int e = raft_recv_requestvote_response(r, raft_get_node(r, 2), &rvr);
    CuAssertIntEquals(tc, 0, e);
    CuAssertTrue(tc, 2 == raft_get_nvotes_for_me(r));
}

void TestRaft_server_recv_requestvote_response_must_be_candidate_to_receive(
    CuTest * tc
    )
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
        .send_appendentries = __raft_send_appendentries,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);
    CuAssertTrue(tc, 0 == raft_get_nvotes_for_me(r));

    raft_become_leader(r);

    msg_requestvote_response_t rvr;
    memset(&rvr, 0, sizeof(msg_requestvote_response_t));
    rvr.term = 1;
    rvr.vote_granted = 1;
    raft_recv_requestvote_response(r, raft_get_node(r, 2), &rvr);
    CuAssertTrue(tc, 0 == raft_get_nvotes_for_me(r));
}

/* Reply false if term < currentTerm (�5.1) */
void TestRaft_server_recv_requestvote_reply_false_if_term_less_than_current_term(
    CuTest * tc
    )
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_requestvote_response_t rvr;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 2);

    /* term is less than current term */
    msg_requestvote_t rv;
    memset(&rv, 0, sizeof(msg_requestvote_t));
    rv.term = 1;
    int e = raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);
    CuAssertIntEquals(tc, 0, e);
    CuAssertIntEquals(tc, 0, rvr.vote_granted);
}

void TestRaft_leader_recv_requestvote_does_not_step_down(
    CuTest * tc
    )
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
        .send_appendentries = __raft_send_appendentries,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_requestvote_response_t rvr;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);
    raft_vote(r, raft_get_node(r, 1));
    raft_become_leader(r);
    CuAssertIntEquals(tc, 1, raft_is_leader(r));

    /* term is less than current term */
    msg_requestvote_t rv;
    memset(&rv, 0, sizeof(msg_requestvote_t));
    rv.term = 1;
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);
    CuAssertIntEquals(tc, 1, raft_get_current_leader(r));
}

/* Reply true if term >= currentTerm (�5.1) */
void TestRaft_server_recv_requestvote_reply_true_if_term_greater_than_or_equal_to_current_term(
    CuTest * tc
    )
{
    msg_requestvote_t rv;
    msg_requestvote_response_t rvr;

    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);

    /* term is less than current term */
    memset(&rv, 0, sizeof(msg_requestvote_t));
    rv.term = 2;
    rv.last_log_idx = 1;
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);

    CuAssertTrue(tc, 1 == rvr.vote_granted);
}

void TestRaft_server_recv_requestvote_reset_timeout(
    CuTest * tc
    )
{
    msg_requestvote_t rv;
    msg_requestvote_response_t rvr;

    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);

    raft_set_election_timeout(r, 1000);
    raft_periodic(r, 900);

    memset(&rv, 0, sizeof(msg_requestvote_t));
    rv.term = 2;
    rv.last_log_idx = 1;
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);
    CuAssertTrue(tc, 1 == rvr.vote_granted);
    CuAssertIntEquals(tc, 0, raft_get_timeout_elapsed(r));
}

void TestRaft_server_recv_requestvote_candidate_step_down_if_term_is_higher_than_current_term(
    CuTest * tc
    )
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
        .send_requestvote = __raft_send_requestvote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_become_candidate(r);
    raft_set_current_term(r, 1);
    CuAssertIntEquals(tc, 1, raft_get_voted_for(r));

    /* current term is less than term */
    msg_requestvote_t rv;
    memset(&rv, 0, sizeof(msg_requestvote_t));
    rv.candidate_id = 2;
    rv.term = 2;
    rv.last_log_idx = 1;
    msg_requestvote_response_t rvr;
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);
    CuAssertIntEquals(tc, 1, raft_is_follower(r));
    CuAssertIntEquals(tc, 2, raft_get_current_term(r));
    CuAssertIntEquals(tc, 2, raft_get_voted_for(r));
}

void TestRaft_server_recv_requestvote_depends_on_candidate_id(
    CuTest * tc
    )
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
        .send_requestvote = __raft_send_requestvote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_become_candidate(r);
    raft_set_current_term(r, 1);
    CuAssertIntEquals(tc, 1, raft_get_voted_for(r));

    /* current term is less than term */
    msg_requestvote_t rv;
    memset(&rv, 0, sizeof(msg_requestvote_t));
    rv.candidate_id = 3;
    rv.term = 2;
    rv.last_log_idx = 1;
    msg_requestvote_response_t rvr;
    raft_recv_requestvote(r, NULL, &rv, &rvr);
    CuAssertIntEquals(tc, 1, raft_is_follower(r));
    CuAssertIntEquals(tc, 2, raft_get_current_term(r));
    CuAssertIntEquals(tc, 3, raft_get_voted_for(r));
}

/* If votedFor is null or candidateId, and candidate's log is at
 * least as up-to-date as local log, grant vote (�5.2, �5.4) */
void TestRaft_server_recv_requestvote_dont_grant_vote_if_we_didnt_vote_for_this_candidate(
    CuTest * tc
    )
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 0, 0);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);

    /* vote for self */
    raft_vote_for_nodeid(r, 1);

    msg_requestvote_t rv = {};
    rv.term = 1;
    rv.candidate_id = 1;
    rv.last_log_idx = 1;
    rv.last_log_term = 1;
    msg_requestvote_response_t rvr;
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);
    CuAssertTrue(tc, 0 == rvr.vote_granted);

    /* vote for ID 0 */
    raft_vote_for_nodeid(r, 0);
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);
    CuAssertTrue(tc, 0 == rvr.vote_granted);
}

/* If requestvote is received within the minimum election timeout of
 * hearing from a current leader, it does not update its term or grant its
 * vote (�6).
 */
void TestRaft_server_recv_requestvote_ignore_if_master_is_fresh(CuTest * tc)
{
    raft_cbs_t funcs = { 0
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);
    raft_set_election_timeout(r, 1000);

    msg_appendentries_t ae = { 0 };
    msg_appendentries_response_t aer;
    ae.term = 1;

    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertTrue(tc, 1 == aer.success);

    msg_requestvote_t rv = { 
        .term = 2,
        .candidate_id = 3,
        .last_log_idx = 0,
        .last_log_term = 1
    };
    msg_requestvote_response_t rvr;
    raft_recv_requestvote(r, raft_get_node(r, 3), &rv, &rvr);
    CuAssertTrue(tc, 1 != rvr.vote_granted);

    /* After election timeout passed, the same requestvote should be accepted */
    raft_periodic(r, 1001);
    raft_recv_requestvote(r, raft_get_node(r, 3), &rv, &rvr);
    CuAssertTrue(tc, 1 == rvr.vote_granted);
}

void TestRaft_follower_becomes_follower_is_follower(CuTest * tc)
{
    void *r = raft_new();
    raft_become_follower(r);
    CuAssertTrue(tc, raft_is_follower(r));
}

void TestRaft_follower_becomes_follower_does_not_clear_voted_for(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);

    raft_vote(r, raft_get_node(r, 1));
    CuAssertIntEquals(tc, 1, raft_get_voted_for(r));
    raft_become_follower(r);
    CuAssertIntEquals(tc, 1, raft_get_voted_for(r));
}

/* 5.1 */
void TestRaft_follower_recv_appendentries_reply_false_if_term_less_than_currentterm(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    /* no leader known at this point */
    CuAssertTrue(tc, -1 == raft_get_current_leader(r));

    /* term is low */
    msg_appendentries_t ae;
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;

    /*  higher current term */
    raft_set_current_term(r, 5);
    msg_appendentries_response_t aer;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    CuAssertTrue(tc, 0 == aer.success);
    /* rejected appendentries doesn't change the current leader. */
    CuAssertTrue(tc, -1 == raft_get_current_leader(r));
}

void TestRaft_follower_recv_appendentries_does_not_need_node(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    msg_appendentries_t ae = {};
    ae.term = 1;
    msg_appendentries_response_t aer;
    raft_recv_appendentries(r, NULL, &ae, &aer);
    CuAssertTrue(tc, 1 == aer.success);
}

/* TODO: check if test case is needed */
void TestRaft_follower_recv_appendentries_updates_currentterm_if_term_gt_currentterm(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /*  older currentterm */
    raft_set_current_term(r, 1);
    CuAssertTrue(tc, -1 == raft_get_current_leader(r));

    /*  newer term for appendentry */
    memset(&ae, 0, sizeof(msg_appendentries_t));
    /* no prev log idx */
    ae.prev_log_idx = 0;
    ae.term = 2;

    /*  appendentry has newer term, so we change our currentterm */
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertTrue(tc, 1 == aer.success);
    CuAssertTrue(tc, 2 == aer.term);
    /* term has been updated */
    CuAssertTrue(tc, 2 == raft_get_current_term(r));
    /* and leader has been updated */
    CuAssertIntEquals(tc, 2, raft_get_current_leader(r));
}

void TestRaft_follower_recv_appendentries_does_not_log_if_no_entries_are_specified(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_FOLLOWER);

    /*  log size s */
    CuAssertTrue(tc, 0 == raft_get_log_count(r));

    /* receive an appendentry with commit */
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_term = 1;
    ae.prev_log_idx = 4;
    ae.leader_commit = 5;
    ae.n_entries = 0;

    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertTrue(tc, 0 == raft_get_log_count(r));
}

void TestRaft_follower_recv_appendentries_increases_log(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_appendentries_t ae;
    msg_entry_t ety = {};
    msg_appendentries_response_t aer;
    char *str = "aaa";

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_FOLLOWER);

    /*  log size s */
    CuAssertTrue(tc, 0 == raft_get_log_count(r));

    /* receive an appendentry with commit */
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 3;
    ae.prev_log_term = 1;
    /* first appendentries msg */
    ae.prev_log_idx = 0;
    ae.leader_commit = 5;
    /* include one entry */
    memset(&ety, 0, sizeof(msg_entry_t));
    ety.data.buf = str;
    ety.data.len = 3;
    ety.id = 1;
    /* check that old terms are passed onto the log */
    ety.term = 2;
    ae.entries = &ety;
    ae.n_entries = 1;

    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertTrue(tc, 1 == aer.success);
    CuAssertTrue(tc, 1 == raft_get_log_count(r));
    raft_entry_t* log = raft_get_entry_from_idx(r, 1);
    CuAssertTrue(tc, 2 == log->term);
}

/*  5.3 */
void TestRaft_follower_recv_appendentries_reply_false_if_doesnt_have_log_at_prev_log_idx_which_matches_prev_log_term(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_entry_t ety = {};
    char *str = "aaa";

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* term is different from appendentries */
    raft_set_current_term(r, 2);
    // TODO at log manually?

    /* log idx that server doesn't have */
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 2;
    ae.prev_log_idx = 1;
    /* prev_log_term is less than current term (ie. 2) */
    ae.prev_log_term = 1;
    /* include one entry */
    memset(&ety, 0, sizeof(msg_entry_t));
    ety.data.buf = str;
    ety.data.len = 3;
    ety.id = 1;
    ae.entries = &ety;
    ae.n_entries = 1;

    /* trigger reply */
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    /* reply is false */
    CuAssertTrue(tc, 0 == aer.success);
}

static raft_entry_t* __create_mock_entries_for_conflict_tests(
        CuTest * tc,
        raft_server_t* r,
        char** strs)
{
    raft_entry_t ety = {};
    raft_entry_t *ety_appended;

    /* increase log size */
    char *str1 = strs[0];
    ety.data.buf = str1;
    ety.data.len = 3;
    ety.id = 1;
    ety.term = 1;
    raft_append_entry(r, &ety);
    CuAssertTrue(tc, 1 == raft_get_log_count(r));

    /* this log will be overwritten by a later appendentries */
    char *str2 = strs[1];
    ety.data.buf = str2;
    ety.data.len = 3;
    ety.id = 2;
    ety.term = 1;
    raft_append_entry(r, &ety);
    CuAssertTrue(tc, 2 == raft_get_log_count(r));
    CuAssertTrue(tc, NULL != (ety_appended = raft_get_entry_from_idx(r, 2)));
    CuAssertTrue(tc, !strncmp(ety_appended->data.buf, str2, 3));

    /* this log will be overwritten by a later appendentries */
    char *str3 = strs[2];
    ety.data.buf = str3;
    ety.data.len = 3;
    ety.id = 3;
    ety.term = 1;
    raft_append_entry(r, &ety);
    CuAssertTrue(tc, 3 == raft_get_log_count(r));
    CuAssertTrue(tc, NULL != (ety_appended = raft_get_entry_from_idx(r, 3)));
    CuAssertTrue(tc, !strncmp(ety_appended->data.buf, str3, 3));

    return ety_appended;
}

/* 5.3 */
void TestRaft_follower_recv_appendentries_delete_entries_if_conflict_with_new_entries_via_prev_log_idx(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_current_term(r, 1);

    char* strs[] = {"111", "222", "333"};
    raft_entry_t *ety_appended = __create_mock_entries_for_conflict_tests(tc, r, strs);

    /* pass a appendentry that is newer  */
    msg_entry_t mety = {};

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 2;
    /* entries from 2 onwards will be overwritten by this appendentries message */
    ae.prev_log_idx = 1;
    ae.prev_log_term = 1;
    /* include one entry */
    memset(&mety, 0, sizeof(msg_entry_t));
    char *str4 = "444";
    mety.data.buf = str4;
    mety.data.len = 3;
    mety.id = 4;
    ae.entries = &mety;
    ae.n_entries = 1;

    /* str4 has overwritten the last 2 entries */
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertTrue(tc, 1 == aer.success);
    CuAssertTrue(tc, 2 == raft_get_log_count(r));
    /* str1 is still there */
    CuAssertTrue(tc, NULL != (ety_appended = raft_get_entry_from_idx(r, 1)));
    CuAssertTrue(tc, !strncmp(ety_appended->data.buf, strs[0], 3));
    /* str4 has overwritten the last 2 entries */
    CuAssertTrue(tc, NULL != (ety_appended = raft_get_entry_from_idx(r, 2)));
    CuAssertTrue(tc, !strncmp(ety_appended->data.buf, str4, 3));
}

void TestRaft_follower_recv_appendentries_delete_entries_if_conflict_with_new_entries_via_prev_log_idx_at_idx_0(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_current_term(r, 1);

    char* strs[] = {"111", "222", "333"};
    raft_entry_t *ety_appended = __create_mock_entries_for_conflict_tests(tc, r, strs);

    /* pass a appendentry that is newer  */
    msg_entry_t mety = {};

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 2;
    /* ALL append entries will be overwritten by this appendentries message */
    ae.prev_log_idx = 0;
    ae.prev_log_term = 0;
    /* include one entry */
    memset(&mety, 0, sizeof(msg_entry_t));
    char *str4 = "444";
    mety.data.buf = str4;
    mety.data.len = 3;
    mety.id = 4;
    ae.entries = &mety;
    ae.n_entries = 1;

    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertTrue(tc, 1 == aer.success);
    CuAssertTrue(tc, 1 == raft_get_log_count(r));
    /* str1 is gone */
    CuAssertTrue(tc, NULL != (ety_appended = raft_get_entry_from_idx(r, 1)));
    CuAssertTrue(tc, !strncmp(ety_appended->data.buf, str4, 3));
}

void TestRaft_follower_recv_appendentries_delete_entries_if_conflict_with_new_entries_greater_than_prev_log_idx(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_current_term(r, 1);

    char* strs[] = {"111", "222", "333"};
    raft_entry_t *ety_appended;
   
    __create_mock_entries_for_conflict_tests(tc, r, strs);
    CuAssertIntEquals(tc, 3, raft_get_log_count(r));

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 2;
    ae.prev_log_idx = 1;
    ae.prev_log_term = 1;
    msg_entry_t e[1];
    memset(&e, 0, sizeof(msg_entry_t) * 1);
    e[0].id = 1;
    ae.entries = e;
    ae.n_entries = 1;

    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertTrue(tc, 1 == aer.success);
    CuAssertIntEquals(tc, 2, raft_get_log_count(r));
    CuAssertTrue(tc, NULL != (ety_appended = raft_get_entry_from_idx(r, 1)));
    CuAssertTrue(tc, !strncmp(ety_appended->data.buf, strs[0], 3));
}

// TODO: add TestRaft_follower_recv_appendentries_delete_entries_if_term_is_different

void TestRaft_follower_recv_appendentries_add_new_entries_not_already_in_log(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_idx = 0;
    ae.prev_log_term = 1;
    /* include entries */
    msg_entry_t e[2];
    memset(&e, 0, sizeof(msg_entry_t) * 2);
    e[0].id = 1;
    e[1].id = 2;
    ae.entries = e;
    ae.n_entries = 2;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    CuAssertTrue(tc, 1 == aer.success);
    CuAssertTrue(tc, 2 == raft_get_log_count(r));
}

void TestRaft_follower_recv_appendentries_does_not_add_dupe_entries_already_in_log(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_idx = 0;
    ae.prev_log_term = 1;
    /* include 1 entry */
    msg_entry_t e[2];
    memset(&e, 0, sizeof(msg_entry_t) * 2);
    e[0].id = 1;
    ae.entries = e;
    ae.n_entries = 1;
    memset(&aer, 0, sizeof(aer));
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    memset(&aer, 0, sizeof(aer));
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    /* still successful even when no raft_append_entry() happened! */
    CuAssertTrue(tc, 1 == aer.success);
    CuAssertIntEquals(tc, 1, raft_get_log_count(r));

    /* lets get the server to append 2 now! */
    e[1].id = 2;
    ae.n_entries = 2;
    memset(&aer, 0, sizeof(aer));
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertTrue(tc, 1 == aer.success);
    CuAssertIntEquals(tc, 2, raft_get_log_count(r));
}

typedef enum {
    __RAFT_NO_ERR = 0,
    __RAFT_LOG_OFFER_ERR,
    __RAFT_LOG_POP_ERR
} __raft_error_type_e;

typedef struct {
    __raft_error_type_e type;
    raft_index_t idx;
} __raft_error_t;

static int __raft_log_offer_error(
    raft_server_t* raft,
    void *user_data,
    raft_entry_t *entry,
    raft_index_t entry_idx)
{
    __raft_error_t *error = user_data;

    if (__RAFT_LOG_OFFER_ERR == error->type && entry_idx == error->idx)
        return RAFT_ERR_NOMEM;
    return 0;
}

static int __raft_log_pop_error(
    raft_server_t* raft,
    void *user_data,
    raft_entry_t *entry,
    raft_index_t entry_idx)
{
    __raft_error_t *error = user_data;

    if (__RAFT_LOG_POP_ERR == error->type && entry_idx == error->idx)
        return RAFT_ERR_NOMEM;
    return 0;
}

void TestRaft_follower_recv_appendentries_partial_failures(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .log_offer = __raft_log_offer_error,
        .log_pop = __raft_log_pop_error
    };

    void *r = raft_new();
    __raft_error_t error = {};
    raft_set_callbacks(r, &funcs, &error);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);

    /* Append entry 1 and 2 of term 1. */
    raft_entry_t ety = {};
    ety.data.buf = "1aa";
    ety.data.len = 3;
    ety.id = 1;
    ety.term = 1;
    raft_append_entry(r, &ety);
    ety.data.buf = "1bb";
    ety.data.len = 3;
    ety.id = 2;
    ety.term = 1;
    raft_append_entry(r, &ety);
    CuAssertIntEquals(tc, 2, raft_get_current_idx(r));

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    /* To be received: entry 2 and 3 of term 2. */
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 2;
    ae.prev_log_idx = 1;
    ae.prev_log_term = 1;
    msg_entry_t e[2];
    memset(&e, 0, sizeof(msg_entry_t) * 2);
    e[0].term = 2;
    e[0].id = 2;
    e[1].term = 2;
    e[1].id = 3;
    ae.entries = e;
    ae.n_entries = 2;

    /* Ask log_pop to fail at entry 2. */
    error.type = __RAFT_LOG_POP_ERR;
    error.idx = 2;
    memset(&aer, 0, sizeof(aer));
    int err = raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertIntEquals(tc, RAFT_ERR_NOMEM, err);
    CuAssertIntEquals(tc, 1, aer.success);
    CuAssertIntEquals(tc, 1, aer.current_idx);
    CuAssertIntEquals(tc, 2, raft_get_current_idx(r));
    raft_entry_t *tmp = raft_get_entry_from_idx(r, 2);
    CuAssertTrue(tc, NULL != tmp);
    CuAssertIntEquals(tc, 1, tmp->term);

    /* Ask log_offer to fail at entry 3. */
    error.type = __RAFT_LOG_OFFER_ERR;
    error.idx = 3;
    memset(&aer, 0, sizeof(aer));
    err = raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertIntEquals(tc, RAFT_ERR_NOMEM, err);
    CuAssertIntEquals(tc, 1, aer.success);
    CuAssertIntEquals(tc, 2, aer.current_idx);
    CuAssertIntEquals(tc, 2, raft_get_current_idx(r));
    tmp = raft_get_entry_from_idx(r, 2);
    CuAssertTrue(tc, NULL != tmp);
    CuAssertIntEquals(tc, 2, tmp->term);

    /* No more errors. */
    memset(&error, 0, sizeof(error));
    memset(&aer, 0, sizeof(aer));
    err = raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertIntEquals(tc, 0, err);
    CuAssertIntEquals(tc, 1, aer.success);
    CuAssertIntEquals(tc, 3, aer.current_idx);
    CuAssertIntEquals(tc, 3, raft_get_current_idx(r));
}

/* If leaderCommit > commitidx, set commitidx =
 *  min(leaderCommit, last log idx) */
void TestRaft_follower_recv_appendentries_set_commitidx_to_prevLogIdx(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_idx = 0;
    ae.prev_log_term = 1;
    /* include entries */
    msg_entry_t e[4];
    memset(&e, 0, sizeof(msg_entry_t) * 4);
    e[0].term = 1;
    e[0].id = 1;
    e[1].term = 1;
    e[1].id = 2;
    e[2].term = 1;
    e[2].id = 3;
    e[3].term = 1;
    e[3].id = 4;
    ae.entries = e;
    ae.n_entries = 4;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    /* receive an appendentry with commit */
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_term = 1;
    ae.prev_log_idx = 4;
    ae.leader_commit = 5;
    /* receipt of appendentries changes commit idx */
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    CuAssertTrue(tc, 1 == aer.success);
    /* set to 4 because commitIDX is lower */
    CuAssertIntEquals(tc, 4, raft_get_commit_idx(r));
}

void TestRaft_follower_recv_appendentries_set_commitidx_to_LeaderCommit(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_idx = 0;
    ae.prev_log_term = 1;
    /* include entries */
    msg_entry_t e[4];
    memset(&e, 0, sizeof(msg_entry_t) * 4);
    e[0].term = 1;
    e[0].id = 1;
    e[1].term = 1;
    e[1].id = 2;
    e[2].term = 1;
    e[2].id = 3;
    e[3].term = 1;
    e[3].id = 4;
    ae.entries = e;
    ae.n_entries = 4;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    /* receive an appendentry with commit */
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_term = 1;
    ae.prev_log_idx = 3;
    ae.leader_commit = 3;
    /* receipt of appendentries changes commit idx */
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    CuAssertTrue(tc, 1 == aer.success);
    /* set to 3 because leaderCommit is lower */
    CuAssertIntEquals(tc, 3, raft_get_commit_idx(r));
}

void TestRaft_follower_recv_appendentries_failure_includes_current_idx(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);

    raft_entry_t ety = {};
    ety.data.buf = "aaa";
    ety.data.len = 3;
    ety.id = 1;
    ety.term = 1;
    raft_append_entry(r, &ety);

    /* receive an appendentry with commit */
    msg_appendentries_t ae;
    memset(&ae, 0, sizeof(msg_appendentries_t));
    /* lower term means failure */
    ae.term = 0;
    ae.prev_log_term = 0;
    ae.prev_log_idx = 0;
    ae.leader_commit = 0;
    msg_appendentries_response_t aer;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    CuAssertTrue(tc, 0 == aer.success);
    CuAssertIntEquals(tc, 1, aer.current_idx);

    /* try again with a higher current_idx */
    memset(&aer, 0, sizeof(aer));
    ety.id = 2;
    raft_append_entry(r, &ety);
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertTrue(tc, 0 == aer.success);
    CuAssertIntEquals(tc, 2, aer.current_idx);
}

void TestRaft_follower_becomes_candidate_when_election_timeout_occurs(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
        .send_requestvote = __raft_send_requestvote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    /*  1 second election timeout */
    raft_set_election_timeout(r, 1000);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /*  max election timeout have passed */
    raft_periodic(r, max_election_timeout(1000) + 1);

    /* is a candidate now */
    CuAssertTrue(tc, 1 == raft_is_candidate(r));
}

/* Candidate 5.2 */
void TestRaft_follower_dont_grant_vote_if_candidate_has_a_less_complete_log(
    CuTest * tc)
{
    msg_requestvote_t rv;
    msg_requestvote_response_t rvr;

    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /*  request vote */
    /*  vote indicates candidate's log is not complete compared to follower */
    memset(&rv, 0, sizeof(msg_requestvote_t));
    rv.term = 1;
    rv.candidate_id = 1;
    rv.last_log_idx = 1;
    rv.last_log_term = 1;

    raft_set_current_term(r, 1);

    /* server's idx are more up-to-date */
    raft_entry_t ety = {};
    ety.term = 1;
    ety.id = 100;
    ety.data.len = 4;
    ety.data.buf = (unsigned char*)"aaa";
    raft_append_entry(r, &ety);
    ety.id = 101;
    ety.term = 2;
    raft_append_entry(r, &ety);

    /* vote not granted */
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);
    CuAssertTrue(tc, 0 == rvr.vote_granted);

    /* approve vote, because last_log_term is higher */
    raft_set_current_term(r, 2);
    memset(&rv, 0, sizeof(msg_requestvote_t));
    rv.term = 2;
    rv.candidate_id = 1;
    rv.last_log_idx = 1;
    rv.last_log_term = 3;
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);
    CuAssertIntEquals(tc, 1, rvr.vote_granted);
}

void TestRaft_follower_recv_appendentries_heartbeat_does_not_overwrite_logs(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_idx = 0;
    ae.prev_log_term = 1;
    /* include entries */
    msg_entry_t e[4];
    memset(&e, 0, sizeof(msg_entry_t) * 4);
    e[0].term = 1;
    e[0].id = 1;
    ae.entries = e;
    ae.n_entries = 1;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    /* The server sends a follow up AE
     * NOTE: the server has received a response from the last AE so
     * prev_log_idx has been incremented */
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_idx = 1;
    ae.prev_log_term = 1;
    /* include entries */
    memset(&e, 0, sizeof(msg_entry_t) * 4);
    e[0].term = 1;
    e[0].id = 2;
    e[1].term = 1;
    e[1].id = 3;
    e[2].term = 1;
    e[2].id = 4;
    e[3].term = 1;
    e[3].id = 5;
    ae.entries = e;
    ae.n_entries = 4;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    /* receive a heartbeat
     * NOTE: the leader hasn't received the response to the last AE so it can
     * only assume prev_Log_idx is still 1 */
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_term = 1;
    ae.prev_log_idx = 1;
    /* receipt of appendentries changes commit idx */
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    CuAssertTrue(tc, 1 == aer.success);
    CuAssertIntEquals(tc, 5, raft_get_current_idx(r));
}

void TestRaft_follower_recv_appendentries_does_not_deleted_commited_entries(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_idx = 0;
    ae.prev_log_term = 1;
    /* include entries */
    msg_entry_t e[5];
    memset(&e, 0, sizeof(msg_entry_t) * 4);
    e[0].term = 1;
    e[0].id = 1;
    ae.entries = e;
    ae.n_entries = 1;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    /* Follow up AE. Node responded with success */
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_idx = 1;
    ae.prev_log_term = 1;
    /* include entries */
    memset(&e, 0, sizeof(msg_entry_t) * 4);
    e[0].term = 1;
    e[0].id = 2;
    e[1].term = 1;
    e[1].id = 3;
    e[2].term = 1;
    e[2].id = 4;
    e[3].term = 1;
    e[3].id = 5;
    ae.entries = e;
    ae.n_entries = 4;
    ae.leader_commit = 4;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    /* The server sends a follow up AE */
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_idx = 1;
    ae.prev_log_term = 1;
    /* include entries */
    memset(&e, 0, sizeof(msg_entry_t) * 5);
    e[0].term = 1;
    e[0].id = 2;
    e[1].term = 1;
    e[1].id = 3;
    e[2].term = 1;
    e[2].id = 4;
    e[3].term = 1;
    e[3].id = 5;
    e[4].term = 1;
    e[4].id = 6;
    ae.entries = e;
    ae.n_entries = 5;
    ae.leader_commit = 4;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    CuAssertTrue(tc, 1 == aer.success);
    CuAssertIntEquals(tc, 6, raft_get_current_idx(r));
    CuAssertIntEquals(tc, 4, raft_get_commit_idx(r));

    /* The server sends a follow up AE.
     * This appendentry forces the node to check if it's going to delete
     * commited logs */
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_idx = 3;
    ae.prev_log_term = 1;
    /* include entries */
    memset(&e, 0, sizeof(msg_entry_t) * 5);
    e[0].id = 1;
    e[0].id = 5;
    e[1].term = 1;
    e[1].id = 6;
    e[2].term = 1;
    e[2].id = 7;
    ae.entries = e;
    ae.n_entries = 3;
    ae.leader_commit = 4;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    CuAssertTrue(tc, 1 == aer.success);
    CuAssertIntEquals(tc, 6, raft_get_current_idx(r));
}

void TestRaft_candidate_becomes_candidate_is_candidate(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_become_candidate(r);
    CuAssertTrue(tc, raft_is_candidate(r));
}

/* Candidate 5.2 */
void TestRaft_follower_becoming_candidate_increments_current_term(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    CuAssertTrue(tc, 0 == raft_get_current_term(r));
    raft_become_candidate(r);
    CuAssertTrue(tc, 1 == raft_get_current_term(r));
}

/* Candidate 5.2 */
void TestRaft_follower_becoming_candidate_votes_for_self(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    CuAssertTrue(tc, -1 == raft_get_voted_for(r));
    raft_become_candidate(r);
    CuAssertTrue(tc, raft_get_nodeid(r) == raft_get_voted_for(r));
    CuAssertTrue(tc, 1 == raft_get_nvotes_for_me(r));
}

/* Candidate 5.2 */
void TestRaft_follower_becoming_candidate_resets_election_timeout(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_set_election_timeout(r, 1000);
    CuAssertTrue(tc, 0 == raft_get_timeout_elapsed(r));

    raft_periodic(r, 900);
    CuAssertTrue(tc, 900 == raft_get_timeout_elapsed(r));

    raft_become_candidate(r);
    /* time is selected randomly */
    CuAssertTrue(tc, raft_get_timeout_elapsed(r) < 1000);
}

void TestRaft_follower_recv_appendentries_resets_election_timeout(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_set_election_timeout(r, 1000);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 1);

    raft_periodic(r, 900);

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    raft_recv_appendentries(r, raft_get_node(r, 1), &ae, &aer);
    CuAssertTrue(tc, 0 == raft_get_timeout_elapsed(r));
}

/* Candidate 5.2 */
void TestRaft_follower_becoming_candidate_requests_votes_from_other_servers(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
        .send_requestvote = sender_requestvote,
    };
    msg_requestvote_t* rv;

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_set_callbacks(r, &funcs, sender);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);

    /* set term so we can check it gets included in the outbound message */
    raft_set_current_term(r, 2);

    /* becoming candidate triggers vote requests */
    raft_become_candidate(r);

    /* 2 nodes = 2 vote requests */
    rv = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL != rv);
    CuAssertTrue(tc, 2 != rv->term);
    CuAssertTrue(tc, 3 == rv->term);
    /*  TODO: there should be more items */
    rv = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL != rv);
    CuAssertTrue(tc, 3 == rv->term);
}

/* Candidate 5.2 */
void TestRaft_candidate_election_timeout_and_no_leader_results_in_new_election(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
        .send_requestvote = __raft_send_requestvote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_requestvote_response_t vr;
    memset(&vr, 0, sizeof(msg_requestvote_response_t));
    vr.term = 0;
    vr.vote_granted = 1;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_election_timeout(r, 1000);

    /* server wants to be leader, so becomes candidate */
    raft_become_candidate(r);
    CuAssertTrue(tc, 1 == raft_get_current_term(r));

    /* clock over (ie. max election timeout + 1), causing new election */
    raft_periodic(r, max_election_timeout(1000) + 1);
    CuAssertTrue(tc, 2 == raft_get_current_term(r));

    /*  receiving this vote gives the server majority */
//    raft_recv_requestvote_response(r,1,&vr);
//    CuAssertTrue(tc, 1 == raft_is_leader(r));
}

/* Candidate 5.2 */
void TestRaft_candidate_receives_majority_of_votes_becomes_leader(CuTest * tc)
{
    msg_requestvote_response_t vr;

    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
        .send_requestvote = __raft_send_requestvote,
        .send_appendentries = __raft_send_appendentries,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);
    raft_add_node(r, NULL, 4, 0);
    raft_add_node(r, NULL, 5, 0);
    CuAssertTrue(tc, 5 == raft_get_num_nodes(r));

    /* vote for self */
    raft_become_candidate(r);
    CuAssertTrue(tc, 1 == raft_get_current_term(r));
    CuAssertTrue(tc, 1 == raft_get_nvotes_for_me(r));

    /* a vote for us */
    memset(&vr, 0, sizeof(msg_requestvote_response_t));
    vr.term = 1;
    vr.vote_granted = 1;
    /* get one vote */
    raft_recv_requestvote_response(r, raft_get_node(r, 2), &vr);
    CuAssertTrue(tc, 2 == raft_get_nvotes_for_me(r));
    CuAssertTrue(tc, 0 == raft_is_leader(r));

    /* get another vote
     * now has majority (ie. 3/5 votes) */
    raft_recv_requestvote_response(r, raft_get_node(r, 3), &vr);
    CuAssertTrue(tc, 1 == raft_is_leader(r));
}

/* Candidate 5.2 */
void TestRaft_candidate_will_not_respond_to_voterequest_if_it_has_already_voted(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_requestvote_t rv;
    msg_requestvote_response_t rvr;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_vote(r, raft_get_node(r, 1));

    memset(&rv, 0, sizeof(msg_requestvote_t));
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);

    /* we've vote already, so won't respond with a vote granted... */
    CuAssertTrue(tc, 0 == rvr.vote_granted);
}

/* Candidate 5.2 */
void TestRaft_candidate_requestvote_includes_logidx(CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_requestvote = sender_requestvote,
        .log              = NULL,
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
    };

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_state(r, RAFT_STATE_CANDIDATE);

    raft_set_callbacks(r, &funcs, sender);
    raft_set_current_term(r, 5);
    /* 3 entries */
    raft_entry_t ety = {};
    ety.term = 1;
    ety.id = 100;
    ety.data.len = 4;
    ety.data.buf = (unsigned char*)"aaa";
    raft_append_entry(r, &ety);
    ety.id = 101;
    raft_append_entry(r, &ety);
    ety.id = 102;
    ety.term = 3;
    raft_append_entry(r, &ety);
    raft_send_requestvote(r, raft_get_node(r, 2));

    msg_requestvote_t* rv = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL != rv);
    CuAssertIntEquals(tc, 3, rv->last_log_idx);
    CuAssertIntEquals(tc, 5, rv->term);
    CuAssertIntEquals(tc, 3, rv->last_log_term);
    CuAssertIntEquals(tc, 1, rv->candidate_id);
}

void TestRaft_candidate_recv_requestvote_response_becomes_follower_if_current_term_is_less_than_term(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_current_term(r, 1);
    raft_set_state(r, RAFT_STATE_CANDIDATE);
    raft_vote(r, 0);
    CuAssertTrue(tc, 0 == raft_is_follower(r));
    CuAssertTrue(tc, -1 == raft_get_current_leader(r));
    CuAssertTrue(tc, 1 == raft_get_current_term(r));

    msg_requestvote_response_t rvr;
    memset(&rvr, 0, sizeof(msg_requestvote_response_t));
    rvr.term = 2;
    rvr.vote_granted = 0;
    raft_recv_requestvote_response(r, raft_get_node(r, 2), &rvr);
    CuAssertTrue(tc, 1 == raft_is_follower(r));
    CuAssertTrue(tc, 2 == raft_get_current_term(r));
    CuAssertTrue(tc, -1 == raft_get_voted_for(r));
}

/* Candidate 5.2 */
void TestRaft_candidate_recv_appendentries_frm_leader_results_in_follower(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_CANDIDATE);
    raft_vote(r, 0);
    CuAssertTrue(tc, 0 == raft_is_follower(r));
    CuAssertTrue(tc, -1 == raft_get_current_leader(r));
    CuAssertTrue(tc, 0 == raft_get_current_term(r));

    /* receive recent appendentries */
    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertTrue(tc, 1 == raft_is_follower(r));
    /* after accepting a leader, it's available as the last known leader */
    CuAssertTrue(tc, 2 == raft_get_current_leader(r));
    CuAssertTrue(tc, 1 == raft_get_current_term(r));
    CuAssertTrue(tc, -1 == raft_get_voted_for(r));
}

/* Candidate 5.2 */
void TestRaft_candidate_recv_appendentries_from_same_term_results_in_step_down(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
        .send_requestvote = __raft_send_requestvote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_current_term(r, 1);
    raft_become_candidate(r);
    CuAssertTrue(tc, 0 == raft_is_follower(r));
    CuAssertIntEquals(tc, 1, raft_get_voted_for(r));

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 2;
    ae.prev_log_idx = 1;
    ae.prev_log_term = 1;

    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertTrue(tc, 0 == raft_is_candidate(r));

    /* The election algorithm requires that votedFor always contains the node
     * voted for in the current term (if any), which is why it is persisted.
     * By resetting that to -1 we have the following problem:
     *
     *  Node self, other1 and other2 becomes candidates
     *  Node other1 wins election
     *  Node self gets appendentries
     *  Node self resets votedFor
     *  Node self gets requestvote from other2
     *  Node self votes for Other2
    */
    CuAssertIntEquals(tc, 1, raft_get_voted_for(r));
}

void TestRaft_leader_becomes_leader_is_leader(CuTest * tc)
{
    void *r = raft_new();
    raft_become_leader(r);
    CuAssertTrue(tc, raft_is_leader(r));
}

void TestRaft_leader_becomes_leader_does_not_clear_voted_for(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_vote(r, raft_get_node(r, 1));
    CuAssertTrue(tc, 1 == raft_get_voted_for(r));
    raft_become_leader(r);
    CuAssertTrue(tc, 1 == raft_get_voted_for(r));
}

void TestRaft_leader_when_becomes_leader_all_nodes_have_nextidx_equal_to_lastlog_idx_plus_1(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries = __raft_send_appendentries,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);

    /* candidate to leader */
    raft_set_state(r, RAFT_STATE_CANDIDATE);
    raft_become_leader(r);

    int i;
    for (i = 2; i <= 3; i++)
    {
        raft_node_t* p = raft_get_node(r, i);
        CuAssertTrue(tc, raft_get_current_idx(r) + 1 ==
                     raft_node_get_next_idx(p));
    }
}

/* 5.2 */
void TestRaft_leader_when_it_becomes_a_leader_sends_empty_appendentries(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries = sender_appendentries,
        .log                = NULL
    };

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_set_callbacks(r, &funcs, sender);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);

    /* candidate to leader */
    raft_set_state(r, RAFT_STATE_CANDIDATE);
    raft_become_leader(r);

    /* receive appendentries messages for both nodes */
    msg_appendentries_t* ae = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL != ae);
    ae = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL != ae);
}

/* 5.2
 * Note: commit means it's been appended to the log, not applied to the FSM */
void TestRaft_leader_responds_to_entry_msg_when_entry_is_committed(CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries = __raft_send_appendentries,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_entry_response_t cr;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* I am the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    CuAssertTrue(tc, 0 == raft_get_log_count(r));

    /* entry message */
    msg_entry_t ety = {};
    ety.id = 1;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");

    /* receive entry */
    raft_recv_entry(r, &ety, &cr);
    CuAssertTrue(tc, 1 == raft_get_log_count(r));

    /* trigger response through commit */
    raft_apply_entry(r);
}

void TestRaft_non_leader_recv_entry_msg_fails(CuTest * tc)
{
    msg_entry_response_t cr;

    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_FOLLOWER);

    /* entry message */
    msg_entry_t ety = {};
    ety.id = 1;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");

    /* receive entry */
    int e = raft_recv_entry(r, &ety, &cr);
    CuAssertTrue(tc, RAFT_ERR_NOT_LEADER == e);
}

/* 5.3 */
void TestRaft_leader_sends_appendentries_with_NextIdx_when_PrevIdx_gt_NextIdx(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries = sender_appendentries,
        .log                = NULL
    };

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_set_callbacks(r, &funcs, sender);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* i'm leader */
    raft_set_state(r, RAFT_STATE_LEADER);

    raft_node_t* p = raft_get_node(r, 2);
    raft_node_set_next_idx(p, 4);

    /* receive appendentries messages */
    raft_send_appendentries(r, p);
    msg_appendentries_t* ae = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL != ae);
}

void TestRaft_leader_sends_appendentries_with_leader_commit(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries = sender_appendentries,
        .log                = NULL
    };

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_set_callbacks(r, &funcs, sender);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* i'm leader */
    raft_set_state(r, RAFT_STATE_LEADER);

    int i;

    for (i=0; i<10; i++)
    {
        raft_entry_t ety = {};
        ety.term = 1;
        ety.id = 1;
        ety.data.buf = "aaa";
        ety.data.len = 3;
        raft_append_entry(r, &ety);
    }

    raft_set_commit_idx(r, 10);

    /* receive appendentries messages */
    raft_send_appendentries(r, raft_get_node(r, 2));
    msg_appendentries_t*  ae = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL != ae);
    CuAssertTrue(tc, ae->leader_commit == 10);
}

void TestRaft_leader_sends_appendentries_with_prevLogIdx(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries = sender_appendentries,
        .log                = NULL
    };

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_set_callbacks(r, &funcs, sender);
    raft_add_node(r, NULL, 1, 1); /* me */
    raft_add_node(r, NULL, 2, 0);

    /* i'm leader */
    raft_set_state(r, RAFT_STATE_LEADER);

    /* receive appendentries messages */
    raft_send_appendentries(r, raft_get_node(r, 2));
    msg_appendentries_t*  ae = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL != ae);
    CuAssertTrue(tc, ae->prev_log_idx == 0);

    raft_node_t* n = raft_get_node(r, 2);

    /* add 1 entry */
    /* receive appendentries messages */
    raft_entry_t ety = {};
    ety.term = 2;
    ety.id = 100;
    ety.data.len = 4;
    ety.data.buf = (unsigned char*)"aaa";
    raft_append_entry(r, &ety);
    raft_node_set_next_idx(n, 1);
    raft_send_appendentries(r, raft_get_node(r, 2));
    ae = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL != ae);
    CuAssertTrue(tc, ae->prev_log_idx == 0);
    CuAssertTrue(tc, ae->n_entries == 1);
    CuAssertTrue(tc, ae->entries[0].id == 100);
    CuAssertTrue(tc, ae->entries[0].term == 2);

    /* set next_idx */
    /* receive appendentries messages */
    raft_node_set_next_idx(n, 2);
    raft_send_appendentries(r, raft_get_node(r, 2));
    ae = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL != ae);
    CuAssertTrue(tc, ae->prev_log_idx == 1);
}

void TestRaft_leader_sends_appendentries_when_node_has_next_idx_of_0(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries = sender_appendentries,
        .log                = NULL
    };

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_set_callbacks(r, &funcs, sender);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* i'm leader */
    raft_set_state(r, RAFT_STATE_LEADER);

    /* receive appendentries messages */
    raft_send_appendentries(r, raft_get_node(r, 2));
    msg_appendentries_t*  ae = sender_poll_msg_data(sender);

    /* add an entry */
    /* receive appendentries messages */
    raft_node_t* n = raft_get_node(r, 2);
    raft_node_set_next_idx(n, 1);
    raft_entry_t ety = {};
    ety.term = 1;
    ety.id = 100;
    ety.data.len = 4;
    ety.data.buf = (unsigned char*)"aaa";
    raft_append_entry(r, &ety);
    raft_send_appendentries(r, n);
    ae = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL != ae);
    CuAssertTrue(tc, ae->prev_log_idx == 0);
}

/* 5.3 */
void TestRaft_leader_retries_appendentries_with_decremented_NextIdx_log_inconsistency(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries = sender_appendentries,
        .log                = NULL
    };

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_set_callbacks(r, &funcs, sender);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* i'm leader */
    raft_set_state(r, RAFT_STATE_LEADER);

    /* receive appendentries messages */
    raft_send_appendentries(r, raft_get_node(r, 2));
    msg_appendentries_t* ae = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL != ae);
}

/*
 * If there exists an N such that N > commitidx, a majority
 * of matchidx[i] = N, and log[N].term == currentTerm:
 * set commitidx = N (�5.2, �5.4).  */
void TestRaft_leader_append_entry_to_log_increases_idxno(CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries = __raft_send_appendentries,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_entry_response_t cr;
    msg_entry_t ety = {};
    ety.id = 1;
    ety.data.buf = "entry";
    ety.data.len = strlen("entry");

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_state(r, RAFT_STATE_LEADER);
    CuAssertTrue(tc, 0 == raft_get_log_count(r));

    raft_recv_entry(r, &ety, &cr);
    CuAssertTrue(tc, 1 == raft_get_log_count(r));
}

#if 0
// TODO no support for duplicates
void T_estRaft_leader_doesnt_append_entry_if_unique_id_is_duplicate(CuTest * tc)
{
    void *r;

    /* 2 nodes */
    raft_node_configuration_t cfg[] = {
        { (void*)1 },
        { (void*)2 },
        { NULL     }
    };

    msg_entry_t ety;
    ety.id = 1;
    ety.data = "entry";
    ety.data.len = strlen("entry");

    r = raft_new();
    raft_set_configuration(r, cfg, 0);

    raft_set_state(r, RAFT_STATE_LEADER);
    CuAssertTrue(tc, 0 == raft_get_log_count(r));

    raft_recv_entry(r, 1, &ety);
    CuAssertTrue(tc, 1 == raft_get_log_count(r));

    raft_recv_entry(r, 1, &ety);
    CuAssertTrue(tc, 1 == raft_get_log_count(r));
}
#endif

void TestRaft_leader_recv_appendentries_response_increase_commit_idx_when_majority_have_entry_and_atleast_one_newer_entry(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .applylog = __raft_applylog,
        .persist_term = __raft_persist_term,
        .send_appendentries          = sender_appendentries,
        .log                         = NULL
    };
    msg_appendentries_response_t aer;

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);
    raft_add_node(r, NULL, 4, 0);
    raft_add_node(r, NULL, 5, 0);
    raft_set_callbacks(r, &funcs, sender);

    /* I'm the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);
    raft_set_commit_idx(r, 0);
    /* the last applied idx will became 1, and then 2 */
    raft_set_last_applied_idx(r, 0);

    /* append entries - we need two */
    raft_entry_t ety = {};
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = "aaaa";
    ety.data.len = 4;
    raft_append_entry(r, &ety);
    ety.id = 2;
    raft_append_entry(r, &ety);
    ety.id = 3;
    raft_append_entry(r, &ety);

    memset(&aer, 0, sizeof(msg_appendentries_response_t));

    /* FIRST entry log application */
    /* send appendentries -
     * server will be waiting for response */
    raft_send_appendentries(r, raft_get_node(r, 2));
    raft_send_appendentries(r, raft_get_node(r, 3));
    /* receive mock success responses */
    aer.term = 1;
    aer.success = 1;
    aer.current_idx = 1;
    aer.first_idx = 1;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    CuAssertIntEquals(tc, 0, raft_get_commit_idx(r));
    raft_recv_appendentries_response(r, raft_get_node(r, 3), &aer);
    /* leader will now have majority followers who have appended this log */
    CuAssertIntEquals(tc, 1, raft_get_commit_idx(r));
    raft_periodic(r, 1);
    CuAssertIntEquals(tc, 1, raft_get_last_applied_idx(r));

    /* SECOND entry log application */
    /* send appendentries -
     * server will be waiting for response */
    raft_send_appendentries(r, raft_get_node(r, 2));
    raft_send_appendentries(r, raft_get_node(r, 3));
    /* receive mock success responses */
    aer.term = 1;
    aer.success = 1;
    aer.current_idx = 2;
    aer.first_idx = 2;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    CuAssertIntEquals(tc, 1, raft_get_commit_idx(r));
    raft_recv_appendentries_response(r, raft_get_node(r, 3), &aer);
    /* leader will now have majority followers who have appended this log */
    CuAssertIntEquals(tc, 2, raft_get_commit_idx(r));
    raft_periodic(r, 1);
    CuAssertIntEquals(tc, 2, raft_get_last_applied_idx(r));
}

void TestRaft_leader_recv_appendentries_response_set_has_sufficient_logs_for_node(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .applylog = __raft_applylog,
        .persist_term = __raft_persist_term,
        .node_has_sufficient_logs = __raft_node_has_sufficient_logs,
        .log                         = NULL
    };
    msg_appendentries_response_t aer;

    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);
    raft_add_node(r, NULL, 4, 0);
    raft_node_t* node = raft_add_node(r, NULL, 5, 0);

    int has_sufficient_logs_flag = 0;
    raft_set_callbacks(r, &funcs, &has_sufficient_logs_flag);

    /* I'm the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);
    raft_set_commit_idx(r, 0);
    /* the last applied idx will became 1, and then 2 */
    raft_set_last_applied_idx(r, 0);

    /* append entries - we need two */
    raft_entry_t ety = {};
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = "aaaa";
    ety.data.len = 4;
    raft_append_entry(r, &ety);
    ety.id = 2;
    raft_append_entry(r, &ety);
    ety.id = 3;
    raft_append_entry(r, &ety);

    memset(&aer, 0, sizeof(msg_appendentries_response_t));

    raft_send_appendentries(r, raft_get_node(r, 5));
    aer.term = 1;
    aer.success = 1;
    aer.current_idx = 2;
    aer.first_idx = 1;

    raft_node_set_voting(node, 0);
    raft_recv_appendentries_response(r, node, &aer);
    CuAssertIntEquals(tc, 1, has_sufficient_logs_flag);

    raft_recv_appendentries_response(r, node, &aer);
    CuAssertIntEquals(tc, 1, has_sufficient_logs_flag);
}

void TestRaft_leader_recv_appendentries_response_increase_commit_idx_using_voting_nodes_majority(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .applylog = __raft_applylog,
        .persist_term = __raft_persist_term,
        .send_appendentries          = sender_appendentries,
        .log                         = NULL
    };
    msg_appendentries_response_t aer;

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);
    raft_add_non_voting_node(r, NULL, 4, 0);
    raft_add_non_voting_node(r, NULL, 5, 0);
    raft_set_callbacks(r, &funcs, sender);

    /* I'm the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);
    raft_set_commit_idx(r, 0);
    /* the last applied idx will became 1, and then 2 */
    raft_set_last_applied_idx(r, 0);

    /* append entries - we need two */
    raft_entry_t ety = {};
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = "aaaa";
    ety.data.len = 4;
    raft_append_entry(r, &ety);

    memset(&aer, 0, sizeof(msg_appendentries_response_t));

    /* FIRST entry log application */
    /* send appendentries -
     * server will be waiting for response */
    raft_send_appendentries(r, raft_get_node(r, 2));
    /* receive mock success responses */
    aer.term = 1;
    aer.success = 1;
    aer.current_idx = 1;
    aer.first_idx = 1;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    CuAssertIntEquals(tc, 1, raft_get_commit_idx(r));
    /* leader will now have majority followers who have appended this log */
    raft_periodic(r, 1);
    CuAssertIntEquals(tc, 1, raft_get_last_applied_idx(r));
}

void TestRaft_leader_recv_appendentries_response_duplicate_does_not_decrement_match_idx(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .send_appendentries          = sender_appendentries,
        .log                         = NULL
    };
    msg_appendentries_response_t aer;

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);
    raft_set_callbacks(r, &funcs, sender);

    /* I'm the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);
    raft_set_commit_idx(r, 0);
    /* the last applied idx will became 1, and then 2 */
    raft_set_last_applied_idx(r, 0);

    /* append entries - we need two */
    raft_entry_t ety = {};
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = "aaaa";
    ety.data.len = 4;
    raft_append_entry(r, &ety);
    ety.id = 2;
    raft_append_entry(r, &ety);
    ety.id = 3;
    raft_append_entry(r, &ety);

    memset(&aer, 0, sizeof(msg_appendentries_response_t));

    /* receive msg 1 */
    aer.term = 1;
    aer.success = 1;
    aer.current_idx = 1;
    aer.first_idx = 1;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    CuAssertIntEquals(tc, 1, raft_node_get_match_idx(raft_get_node(r, 2)));

    /* receive msg 2 */
    aer.term = 1;
    aer.success = 1;
    aer.current_idx = 2;
    aer.first_idx = 2;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    CuAssertIntEquals(tc, 2, raft_node_get_match_idx(raft_get_node(r, 2)));

    /* receive msg 1 - because of duplication ie. unreliable network */
    aer.term = 1;
    aer.success = 1;
    aer.current_idx = 1;
    aer.first_idx = 1;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    CuAssertIntEquals(tc, 2, raft_node_get_match_idx(raft_get_node(r, 2)));
}

void TestRaft_leader_recv_appendentries_response_do_not_increase_commit_idx_because_of_old_terms_with_majority(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .applylog = __raft_applylog,
        .persist_term = __raft_persist_term,
        .send_appendentries          = sender_appendentries,
        .log                         = NULL
    };
    msg_appendentries_response_t aer;

    void *sender = sender_new(NULL);
    void *r = raft_new();

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);
    raft_add_node(r, NULL, 4, 0);
    raft_add_node(r, NULL, 5, 0);
    raft_set_callbacks(r, &funcs, sender);

    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 2);
    raft_set_commit_idx(r, 0);
    raft_set_last_applied_idx(r, 0);

    /* append entries - we need two */
    raft_entry_t ety = {};
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = "aaaa";
    ety.data.len = 4;
    raft_append_entry(r, &ety);
    ety.id = 2;
    raft_append_entry(r, &ety);
    ety.term = 2;
    ety.id = 3;
    raft_append_entry(r, &ety);

    memset(&aer, 0, sizeof(msg_appendentries_response_t));

    /* FIRST entry log application */
    /* send appendentries -
     * server will be waiting for response */
    raft_send_appendentries(r, raft_get_node(r, 2));
    raft_send_appendentries(r, raft_get_node(r, 3));
    /* receive mock success responses */
    aer.term = 1;
    aer.success = 1;
    aer.current_idx = 1;
    aer.first_idx = 1;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    CuAssertIntEquals(tc, 0, raft_get_commit_idx(r));
    raft_recv_appendentries_response(r, raft_get_node(r, 3), &aer);
    CuAssertIntEquals(tc, 0, raft_get_commit_idx(r));
    raft_periodic(r, 1);
    CuAssertIntEquals(tc, 0, raft_get_last_applied_idx(r));

    /* SECOND entry log application */
    /* send appendentries -
     * server will be waiting for response */
    raft_send_appendentries(r, raft_get_node(r, 2));
    raft_send_appendentries(r, raft_get_node(r, 3));
    /* receive mock success responses */
    aer.term = 1;
    aer.success = 1;
    aer.current_idx = 2;
    aer.first_idx = 2;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    CuAssertIntEquals(tc, 0, raft_get_commit_idx(r));
    raft_recv_appendentries_response(r, raft_get_node(r, 3), &aer);
    CuAssertIntEquals(tc, 0, raft_get_commit_idx(r));
    raft_periodic(r, 1);
    CuAssertIntEquals(tc, 0, raft_get_last_applied_idx(r));

    /* THIRD entry log application */
    raft_send_appendentries(r, raft_get_node(r, 2));
    raft_send_appendentries(r, raft_get_node(r, 3));
    /* receive mock success responses
     * let's say that the nodes have majority within leader's current term */
    aer.term = 2;
    aer.success = 1;
    aer.current_idx = 3;
    aer.first_idx = 3;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    CuAssertIntEquals(tc, 0, raft_get_commit_idx(r));
    raft_recv_appendentries_response(r, raft_get_node(r, 3), &aer);
    CuAssertIntEquals(tc, 3, raft_get_commit_idx(r));
    raft_periodic(r, 1);
    CuAssertIntEquals(tc, 3, raft_get_last_applied_idx(r));
}

void TestRaft_leader_recv_appendentries_response_jumps_to_lower_next_idx(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .send_appendentries          = sender_appendentries,
        .log                         = NULL
    };
    msg_appendentries_response_t aer;

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_callbacks(r, &funcs, sender);

    raft_set_current_term(r, 2);
    raft_set_commit_idx(r, 0);

    /* append entries */
    raft_entry_t ety = {};
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = "aaaa";
    ety.data.len = 4;
    raft_append_entry(r, &ety);
    ety.term = 2;
    ety.id = 2;
    raft_append_entry(r, &ety);
    ety.term = 3;
    ety.id = 3;
    raft_append_entry(r, &ety);
    ety.term = 4;
    ety.id = 4;
    raft_append_entry(r, &ety);

    msg_appendentries_t* ae;

    /* become leader sets next_idx to current_idx */
    raft_become_leader(r);
    raft_node_t* node = raft_get_node(r, 2);
    CuAssertIntEquals(tc, 5, raft_node_get_next_idx(node));
    CuAssertTrue(tc, NULL != (ae = sender_poll_msg_data(sender)));

    /* FIRST entry log application */
    /* send appendentries -
     * server will be waiting for response */
    raft_send_appendentries(r, raft_get_node(r, 2));
    CuAssertTrue(tc, NULL != (ae = sender_poll_msg_data(sender)));
    CuAssertIntEquals(tc, 4, ae->prev_log_term);
    CuAssertIntEquals(tc, 4, ae->prev_log_idx);

    /* receive mock success responses */
    memset(&aer, 0, sizeof(msg_appendentries_response_t));
    aer.term = 2;
    aer.success = 0;
    aer.current_idx = 1;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    CuAssertIntEquals(tc, 2, raft_node_get_next_idx(node));

    /* see if new appendentries have appropriate values */
    CuAssertTrue(tc, NULL != (ae = sender_poll_msg_data(sender)));
    CuAssertIntEquals(tc, 1, ae->prev_log_term);
    CuAssertIntEquals(tc, 1, ae->prev_log_idx);

    CuAssertTrue(tc, NULL == sender_poll_msg_data(sender));
}

void TestRaft_leader_recv_appendentries_response_decrements_to_lower_next_idx(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .send_appendentries          = sender_appendentries,
        .log                         = NULL
    };
    msg_appendentries_response_t aer;

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_callbacks(r, &funcs, sender);

    raft_set_current_term(r, 2);
    raft_set_commit_idx(r, 0);

    /* append entries */
    raft_entry_t ety = {};
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = "aaaa";
    ety.data.len = 4;
    raft_append_entry(r, &ety);
    ety.term = 2;
    ety.id = 2;
    raft_append_entry(r, &ety);
    ety.term = 3;
    ety.id = 3;
    raft_append_entry(r, &ety);
    ety.term = 4;
    ety.id = 4;
    raft_append_entry(r, &ety);

    msg_appendentries_t* ae;

    /* become leader sets next_idx to current_idx */
    raft_become_leader(r);
    raft_node_t* node = raft_get_node(r, 2);
    CuAssertIntEquals(tc, 5, raft_node_get_next_idx(node));
    CuAssertTrue(tc, NULL != (ae = sender_poll_msg_data(sender)));

    /* FIRST entry log application */
    /* send appendentries -
     * server will be waiting for response */
    raft_send_appendentries(r, raft_get_node(r, 2));
    CuAssertTrue(tc, NULL != (ae = sender_poll_msg_data(sender)));
    CuAssertIntEquals(tc, 4, ae->prev_log_term);
    CuAssertIntEquals(tc, 4, ae->prev_log_idx);

    /* receive mock success responses */
    memset(&aer, 0, sizeof(msg_appendentries_response_t));
    aer.term = 2;
    aer.success = 0;
    aer.current_idx = 4;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    CuAssertIntEquals(tc, 4, raft_node_get_next_idx(node));

    /* see if new appendentries have appropriate values */
    CuAssertTrue(tc, NULL != (ae = sender_poll_msg_data(sender)));
    CuAssertIntEquals(tc, 3, ae->prev_log_term);
    CuAssertIntEquals(tc, 3, ae->prev_log_idx);

    /* receive mock success responses */
    memset(&aer, 0, sizeof(msg_appendentries_response_t));
    aer.term = 2;
    aer.success = 0;
    aer.current_idx = 4;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    CuAssertIntEquals(tc, 3, raft_node_get_next_idx(node));

    /* see if new appendentries have appropriate values */
    CuAssertTrue(tc, NULL != (ae = sender_poll_msg_data(sender)));
    CuAssertIntEquals(tc, 2, ae->prev_log_term);
    CuAssertIntEquals(tc, 2, ae->prev_log_idx);

    CuAssertTrue(tc, NULL == sender_poll_msg_data(sender));
}

void TestRaft_leader_recv_appendentries_response_retry_only_if_leader(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .send_appendentries          = sender_appendentries,
    };

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);
    raft_set_callbacks(r, &funcs, sender);

    /* I'm the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);
    raft_set_commit_idx(r, 0);
    /* the last applied idx will became 1, and then 2 */
    raft_set_last_applied_idx(r, 0);

    /* append entries - we need two */
    raft_entry_t ety = {};
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = "aaaa";
    ety.data.len = 4;
    raft_append_entry(r, &ety);

    raft_send_appendentries(r, raft_get_node(r, 2));
    raft_send_appendentries(r, raft_get_node(r, 3));

    CuAssertTrue(tc, NULL != sender_poll_msg_data(sender));
    CuAssertTrue(tc, NULL != sender_poll_msg_data(sender));

    raft_become_follower(r);

    /* receive mock success responses */
    msg_appendentries_response_t aer;
    memset(&aer, 0, sizeof(msg_appendentries_response_t));
    aer.term = 1;
    aer.success = 1;
    aer.current_idx = 1;
    aer.first_idx = 1;
    CuAssertTrue(tc, RAFT_ERR_NOT_LEADER == raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer));
    CuAssertTrue(tc, NULL == sender_poll_msg_data(sender));
}

void TestRaft_leader_recv_appendentries_response_without_node_fails(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);

    /* I'm the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);

    /* receive mock success responses */
    msg_appendentries_response_t aer;
    memset(&aer, 0, sizeof(msg_appendentries_response_t));
    aer.term = 1;
    aer.success = 1;
    aer.current_idx = 0;
    aer.first_idx = 0;
    CuAssertIntEquals(tc, -1, raft_recv_appendentries_response(r, NULL, &aer));
}

void TestRaft_leader_recv_entry_resets_election_timeout(
    CuTest * tc)
{
    void *r = raft_new();
    raft_set_election_timeout(r, 1000);
    raft_set_state(r, RAFT_STATE_LEADER);

    raft_periodic(r, 900);

    /* entry message */
    msg_entry_t mety = {};
    mety.id = 1;
    mety.data.buf = "entry";
    mety.data.len = strlen("entry");

    /* receive entry */
    msg_entry_response_t cr;
    raft_recv_entry(r, &mety, &cr);
    CuAssertTrue(tc, 0 == raft_get_timeout_elapsed(r));
}

void TestRaft_leader_recv_entry_is_committed_returns_0_if_not_committed(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .send_appendentries = __raft_send_appendentries,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);
    raft_set_commit_idx(r, 0);

    /* entry message */
    msg_entry_t mety = {};
    mety.id = 1;
    mety.data.buf = "entry";
    mety.data.len = strlen("entry");

    /* receive entry */
    msg_entry_response_t cr;
    raft_recv_entry(r, &mety, &cr);
    CuAssertTrue(tc, 0 == raft_msg_entry_response_committed(r, &cr));

    raft_set_commit_idx(r, 1);
    CuAssertTrue(tc, 1 == raft_msg_entry_response_committed(r, &cr));
}

void TestRaft_leader_recv_entry_is_committed_returns_neg_1_if_invalidated(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .send_appendentries = __raft_send_appendentries,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);
    raft_set_commit_idx(r, 0);

    /* entry message */
    msg_entry_t mety = {};
    mety.id = 1;
    mety.data.buf = "entry";
    mety.data.len = strlen("entry");

    /* receive entry */
    msg_entry_response_t cr;
    raft_recv_entry(r, &mety, &cr);
    CuAssertTrue(tc, 0 == raft_msg_entry_response_committed(r, &cr));
    CuAssertTrue(tc, cr.term == 1);
    CuAssertTrue(tc, cr.idx == 1);
    CuAssertTrue(tc, 1 == raft_get_current_idx(r));
    CuAssertTrue(tc, 0 == raft_get_commit_idx(r));

    /* append entry that invalidates entry message */
    msg_appendentries_t ae;
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.leader_commit = 1;
    ae.term = 2;
    ae.prev_log_idx = 0;
    ae.prev_log_term = 0;
    msg_appendentries_response_t aer;
    msg_entry_t e[1];
    memset(&e, 0, sizeof(msg_entry_t) * 1);
    e[0].term = 2;
    e[0].id = 999;
    e[0].data.buf = "aaa";
    e[0].data.len = 3;
    ae.entries = e;
    ae.n_entries = 1;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertTrue(tc, 1 == aer.success);
    CuAssertTrue(tc, 1 == raft_get_current_idx(r));
    CuAssertTrue(tc, 1 == raft_get_commit_idx(r));
    CuAssertTrue(tc, -1 == raft_msg_entry_response_committed(r, &cr));
}

void TestRaft_leader_recv_entry_fails_if_prevlogidx_less_than_commit(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .send_appendentries = __raft_send_appendentries,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 2);
    raft_set_commit_idx(r, 0);

    /* entry message */
    msg_entry_t mety = {};
    mety.id = 1;
    mety.data.buf = "entry";
    mety.data.len = strlen("entry");

    /* receive entry */
    msg_entry_response_t cr;
    raft_recv_entry(r, &mety, &cr);
    CuAssertTrue(tc, 0 == raft_msg_entry_response_committed(r, &cr));
    CuAssertTrue(tc, cr.term == 2);
    CuAssertTrue(tc, cr.idx == 1);
    CuAssertTrue(tc, 1 == raft_get_current_idx(r));
    CuAssertTrue(tc, 0 == raft_get_commit_idx(r));

    raft_set_commit_idx(r, 1);

    /* append entry that invalidates entry message */
    msg_appendentries_t ae;
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.leader_commit = 1;
    ae.term = 2;
    ae.prev_log_idx = 1;
    ae.prev_log_term = 1;
    msg_appendentries_response_t aer;
    msg_entry_t e[1];
    memset(&e, 0, sizeof(msg_entry_t) * 1);
    e[0].term = 2;
    e[0].id = 999;
    e[0].data.buf = "aaa";
    e[0].data.len = 3;
    ae.entries = e;
    ae.n_entries = 1;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertIntEquals(tc, 0, aer.success);
}

void TestRaft_leader_recv_entry_does_not_send_new_appendentries_to_slow_nodes(CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .send_appendentries          = sender_appendentries,
    };

    void *sender = sender_new(NULL);
    raft_set_callbacks(r, &funcs, sender);

    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);
    raft_set_commit_idx(r, 0);

    /* make the node slow */
    raft_node_set_next_idx(raft_get_node(r, 2), 1);

    /* append entries */
    raft_entry_t ety = {};
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = "aaaa";
    ety.data.len = 4;
    raft_append_entry(r, &ety);

    /* entry message */
    msg_entry_t mety = {};
    mety.id = 1;
    mety.data.buf = "entry";
    mety.data.len = strlen("entry");

    /* receive entry */
    msg_entry_response_t cr;
    raft_recv_entry(r, &mety, &cr);

    /* check if the slow node got sent this appendentries */
    msg_appendentries_t* ae;
    ae = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL == ae);
}

void TestRaft_leader_recv_appendentries_response_failure_does_not_set_node_nextid_to_0(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .send_appendentries          = sender_appendentries,
        .log                         = NULL
    };

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_callbacks(r, &funcs, sender);

    /* I'm the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);
    raft_set_commit_idx(r, 0);

    /* append entries */
    raft_entry_t ety = {};
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = "aaaa";
    ety.data.len = 4;
    raft_append_entry(r, &ety);

    /* send appendentries -
     * server will be waiting for response */
    raft_send_appendentries(r, raft_get_node(r, 2));

    /* receive mock success response */
    msg_appendentries_response_t aer;
    memset(&aer, 0, sizeof(msg_appendentries_response_t));
    aer.term = 1;
    aer.success = 0;
    aer.current_idx = 0;
    aer.first_idx = 0;
    raft_node_t* p = raft_get_node(r, 2);
    raft_recv_appendentries_response(r, p, &aer);
    CuAssertTrue(tc, 1 == raft_node_get_next_idx(p));
    raft_recv_appendentries_response(r, p, &aer);
    CuAssertTrue(tc, 1 == raft_node_get_next_idx(p));
}

void TestRaft_leader_recv_appendentries_response_increment_idx_of_node(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .send_appendentries          = sender_appendentries,
        .log                         = NULL
    };

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_callbacks(r, &funcs, sender);

    /* I'm the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);

    raft_node_t* p = raft_get_node(r, 2);
    CuAssertTrue(tc, 1 == raft_node_get_next_idx(p));

    /* receive mock success responses */
    msg_appendentries_response_t aer;
    aer.term = 1;
    aer.success = 1;
    aer.current_idx = 0;
    aer.first_idx = 0;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    CuAssertIntEquals(tc, 1, raft_node_get_next_idx(p));
}

void TestRaft_leader_recv_appendentries_response_drop_message_if_term_is_old(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .send_appendentries          = sender_appendentries,
        .log                         = NULL
    };

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_callbacks(r, &funcs, sender);

    /* I'm the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 2);

    raft_node_t* p = raft_get_node(r, 2);
    CuAssertTrue(tc, 1 == raft_node_get_next_idx(p));

    /* receive OLD mock success responses */
    msg_appendentries_response_t aer;
    aer.term = 1;
    aer.success = 1;
    aer.current_idx = 1;
    aer.first_idx = 1;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    CuAssertTrue(tc, 1 == raft_node_get_next_idx(p));
}

void TestRaft_leader_recv_appendentries_response_steps_down_if_term_is_newer(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries          = sender_appendentries,
        .log                         = NULL
    };

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_callbacks(r, &funcs, sender);

    /* I'm the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 2);

    raft_node_t* p = raft_get_node(r, 2);
    CuAssertTrue(tc, 1 == raft_node_get_next_idx(p));

    /* receive NEW mock failed responses */
    msg_appendentries_response_t aer;
    aer.term = 3;
    aer.success = 0;
    aer.current_idx = 2;
    aer.first_idx = 0;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    CuAssertTrue(tc, 1 == raft_is_follower(r));
    CuAssertTrue(tc, -1 == raft_get_current_leader(r));
}

void TestRaft_leader_recv_appendentries_steps_down_if_newer(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 5);
    /* check that node 1 considers itself the leader */
    CuAssertTrue(tc, 1 == raft_is_leader(r));
    CuAssertTrue(tc, 1 == raft_get_current_leader(r));

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 6;
    ae.prev_log_idx = 6;
    ae.prev_log_term = 5;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    /* after more recent appendentries from node 2, node 1 should
     * consider node 2 the leader. */
    CuAssertTrue(tc, 1 == raft_is_follower(r));
    CuAssertTrue(tc, 2 == raft_get_current_leader(r));
}

void TestRaft_leader_recv_appendentries_steps_down_if_newer_term(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 5);

    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 6;
    ae.prev_log_idx = 5;
    ae.prev_log_term = 5;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    CuAssertTrue(tc, 1 == raft_is_follower(r));
}

void TestRaft_leader_sends_empty_appendentries_every_request_timeout(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .send_appendentries = sender_appendentries,
        .log                = NULL
    };

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_set_callbacks(r, &funcs, sender);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);
    raft_set_election_timeout(r, 1000);
    raft_set_request_timeout(r, 500);
    CuAssertTrue(tc, 0 == raft_get_timeout_elapsed(r));

    /* candidate to leader */
    raft_set_state(r, RAFT_STATE_CANDIDATE);
    raft_become_leader(r);

    /* receive appendentries messages for both nodes */
    msg_appendentries_t* ae;
    ae = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL != ae);
    ae = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL != ae);

    ae = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL == ae);

    /* force request timeout */
    raft_periodic(r, 501);
    ae = sender_poll_msg_data(sender);
    CuAssertTrue(tc, NULL != ae);
}

/* TODO: If a server receives a request with a stale term number, it rejects the request. */
#if 0
void T_estRaft_leader_sends_appendentries_when_receive_entry_msg(CuTest * tc)
#endif

void TestRaft_leader_recv_requestvote_responds_without_granting(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_vote = __raft_persist_vote,
        .persist_term = __raft_persist_term,
        .send_requestvote = __raft_send_requestvote,
        .send_appendentries = sender_appendentries,
    };

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_set_callbacks(r, &funcs, sender);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);
    raft_set_election_timeout(r, 1000);
    raft_set_request_timeout(r, 500);
    CuAssertTrue(tc, 0 == raft_get_timeout_elapsed(r));

    raft_election_start(r);

    msg_requestvote_response_t rvr;
    memset(&rvr, 0, sizeof(msg_requestvote_response_t));
    rvr.term = 1;
    rvr.vote_granted = 1;
    raft_recv_requestvote_response(r, raft_get_node(r, 2), &rvr);
    CuAssertTrue(tc, 1 == raft_is_leader(r));

    /* receive request vote from node 3 */
    msg_requestvote_t rv;
    memset(&rv, 0, sizeof(msg_requestvote_t));
    rv.term = 1;
    raft_recv_requestvote(r, raft_get_node(r, 3), &rv, &rvr);
    CuAssertTrue(tc, 0 == rvr.vote_granted);
}

#if 0
/* This test is disabled because it violates the Raft paper's view on 
 * ignoring RequestVotes when a leader is established.
 */
void T_estRaft_leader_recv_requestvote_responds_with_granting_if_term_is_higher(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_vote = __raft_persist_vote,
        .persist_term = __raft_persist_term,
        .send_requestvote = __raft_send_requestvote,
        .send_appendentries = sender_appendentries,
    };

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_set_callbacks(r, &funcs, sender);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);
    raft_set_election_timeout(r, 1000);
    raft_set_request_timeout(r, 500);
    CuAssertTrue(tc, 0 == raft_get_timeout_elapsed(r));

    raft_election_start(r);

    msg_requestvote_response_t rvr;
    memset(&rvr, 0, sizeof(msg_requestvote_response_t));
    rvr.term = 1;
    rvr.vote_granted = 1;
    raft_recv_requestvote_response(r, raft_get_node(r, 2), &rvr);
    CuAssertTrue(tc, 1 == raft_is_leader(r));

    /* receive request vote from node 3 */
    msg_requestvote_t rv;
    memset(&rv, 0, sizeof(msg_requestvote_t));
    rv.term = 2;
    raft_recv_requestvote(r, raft_get_node(r, 3), &rv, &rvr);
    CuAssertTrue(tc, 1 == raft_is_follower(r));
}
#endif

void TestRaft_leader_recv_appendentries_response_set_has_sufficient_logs_after_voting_committed(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .applylog = __raft_applylog,
        .persist_term = __raft_persist_term,
        .node_has_sufficient_logs = __raft_node_has_sufficient_logs,
        .log_get_node_id = __raft_log_get_node_id,
        .log_offer = __raft_log_offer
    };

    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);

    int has_sufficient_logs_flag = 0;
    raft_set_callbacks(r, &funcs, &has_sufficient_logs_flag);

    /* I'm the leader */
    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);
    raft_set_commit_idx(r, 0);
    raft_set_last_applied_idx(r, 0);

    /* Add two non-voting nodes */
    raft_entry_t ety = {
        .term = 1, .id = 1,
        .data.buf = "2", .data.len = 2,
        .type = RAFT_LOGTYPE_ADD_NONVOTING_NODE
    };
    msg_entry_response_t etyr;
    CuAssertIntEquals(tc, 0, raft_recv_entry(r, &ety, &etyr));
    ety.id++;
    ety.data.buf = "3";
    CuAssertIntEquals(tc, 0, raft_recv_entry(r, &ety, &etyr));

    msg_appendentries_response_t aer = {
        .term = 1, .success = 1, .current_idx = 2, .first_idx = 0
    };

    /* node 3 responds so it has sufficient logs and will be promoted */
    raft_recv_appendentries_response(r, raft_get_node(r, 3), &aer);
    CuAssertIntEquals(tc, 1, has_sufficient_logs_flag);

    ety.id++;
    ety.type = RAFT_LOGTYPE_ADD_NODE;
    raft_recv_entry(r, &ety, &etyr);

    /* we now get a response from node 2, but it's still behind */
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    CuAssertIntEquals(tc, 1, has_sufficient_logs_flag);

    /* both nodes respond to the promotion */
    aer.first_idx = 2;
    aer.current_idx = 3;
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    raft_recv_appendentries_response(r, raft_get_node(r, 3), &aer);
    raft_apply_all(r);

    /* voting change is committed, so next time we hear from node 2
     * it should be considered as having all logs and can be promoted
     * as well.
     */
    CuAssertIntEquals(tc, 1, has_sufficient_logs_flag);
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    CuAssertIntEquals(tc, 2, has_sufficient_logs_flag);
}

/* T3: Server property accessor tests */

void TestRaft_server_get_current_leader_defaults_to_neg1(CuTest * tc)
{
    void *r = raft_new();
    CuAssertIntEquals(tc, -1, raft_get_current_leader(r));
}

void TestRaft_server_get_current_leader_node_defaults_to_null(CuTest * tc)
{
    void *r = raft_new();
    CuAssertPtrEquals(tc, NULL, raft_get_current_leader_node(r));
}

void TestRaft_server_get_current_leader_node_returns_leader(CuTest * tc)
{
    void *r = raft_new();
    raft_set_callbacks(r, &generic_funcs, NULL);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* become leader: single-voting-node shortcut */
    raft_set_current_term(r, 1);
    raft_become_leader(r);

    raft_node_t* leader = raft_get_current_leader_node(r);
    CuAssertPtrNotNull(tc, leader);
    CuAssertIntEquals(tc, 1, raft_node_get_id(leader));
    CuAssertIntEquals(tc, 1, raft_get_current_leader(r));
}

void TestRaft_server_get_last_applied_entry_defaults_to_null(CuTest * tc)
{
    void *r = raft_new();
    CuAssertPtrEquals(tc, NULL, raft_get_last_applied_entry(r));
}

void TestRaft_server_get_last_applied_entry_returns_correct_entry(CuTest * tc)
{
    void *r = raft_new();
    raft_set_callbacks(r, &generic_funcs, NULL);
    raft_add_node(r, NULL, 1, 1);

    raft_entry_t ety = {};
    ety.data.buf = "hello";
    ety.data.len = 5;
    ety.id = 1;
    ety.term = 1;
    raft_set_current_term(r, 1);
    raft_append_entry(r, &ety);
    raft_set_commit_idx(r, 1);

    /* become leader and apply */
    raft_become_leader(r);

    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
        .applylog = __raft_applylog,
    };
    raft_set_callbacks(r, &funcs, NULL);
    raft_apply_all(r);

    raft_entry_t *applied = raft_get_last_applied_entry(r);
    CuAssertPtrNotNull(tc, applied);
    CuAssertIntEquals(tc, 1, applied->id);
}

void TestRaft_server_get_last_log_term_defaults_to_0(CuTest * tc)
{
    void *r = raft_new();
    CuAssertIntEquals(tc, 0, raft_get_last_log_term(r));
}

void TestRaft_server_get_last_log_term_returns_term_of_last_entry(CuTest * tc)
{
    void *r = raft_new();
    raft_set_callbacks(r, &generic_funcs, NULL);
    raft_set_current_term(r, 5);

    raft_entry_t ety = {};
    ety.data.buf = "aaa";
    ety.data.len = 3;
    ety.id = 1;
    ety.term = 5;
    raft_append_entry(r, &ety);
    CuAssertIntEquals(tc, 5, raft_get_last_log_term(r));
}

void TestRaft_server_get_num_voting_nodes_with_mix(CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_non_voting_node(r, NULL, 3, 0);
    raft_add_non_voting_node(r, NULL, 4, 0);

    /* nodes 1 and 2 are voting, 3 and 4 are non-voting */
    CuAssertIntEquals(tc, 2, raft_get_num_voting_nodes(r));
}

void TestRaft_server_get_udata_returns_user_data(CuTest * tc)
{
    void *r = raft_new();
    int mydata = 42;
    raft_set_callbacks(r, &generic_funcs, &mydata);
    CuAssertPtrEquals(tc, &mydata, raft_get_udata(r));
}

void TestRaft_server_get_udata_defaults_to_null(CuTest * tc)
{
    void *r = raft_new();
    CuAssertPtrEquals(tc, NULL, raft_get_udata(r));
}

void TestRaft_server_get_snapshot_last_idx_defaults_to_0(CuTest * tc)
{
    void *r = raft_new();
    CuAssertIntEquals(tc, 0, raft_get_snapshot_last_idx(r));
}

void TestRaft_server_get_snapshot_last_term_defaults_to_0(CuTest * tc)
{
    void *r = raft_new();
    CuAssertIntEquals(tc, 0, raft_get_snapshot_last_term(r));
}

void TestRaft_server_get_first_entry_idx_defaults_to_1(CuTest * tc)
{
    void *r = raft_new();
    raft_set_callbacks(r, &generic_funcs, NULL);

    /* get_first_entry_idx asserts current_idx > 0, so add an entry */
    raft_entry_t ety = {};
    ety.data.buf = "aaa";
    ety.data.len = 3;
    ety.id = 1;
    ety.term = 1;
    raft_append_entry(r, &ety);

    CuAssertIntEquals(tc, 1, raft_get_first_entry_idx(r));
}

void TestRaft_server_get_state_returns_correct_states(CuTest * tc)
{
    void *r = raft_new();
    raft_set_callbacks(r, &generic_funcs, NULL);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    CuAssertIntEquals(tc, RAFT_STATE_FOLLOWER, raft_get_state(r));

    raft_set_current_term(r, 1);
    raft_become_candidate(r);
    CuAssertIntEquals(tc, RAFT_STATE_CANDIDATE, raft_get_state(r));

    raft_become_leader(r);
    CuAssertIntEquals(tc, RAFT_STATE_LEADER, raft_get_state(r));
}

void TestRaft_server_get_nodeid_defaults_to_neg1(CuTest * tc)
{
    void *r = raft_new();
    CuAssertIntEquals(tc, -1, raft_get_nodeid(r));
}

void TestRaft_server_get_nodeid_returns_self_id(CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 5, 1);
    CuAssertIntEquals(tc, 5, raft_get_nodeid(r));
}

void TestRaft_server_get_my_node_defaults_to_null(CuTest * tc)
{
    void *r = raft_new();
    CuAssertPtrEquals(tc, NULL, raft_get_my_node(r));
}

void TestRaft_server_is_apply_allowed_defaults_to_1(CuTest * tc)
{
    void *r = raft_new();
    CuAssertIntEquals(tc, 1, raft_is_apply_allowed(r));
}

void TestRaft_server_is_apply_allowed_returns_0_during_snapshot(CuTest * tc)
{
    void *r = raft_new();
    raft_set_callbacks(r, &generic_funcs, NULL);
    raft_add_node(r, NULL, 1, 1);
    raft_set_current_term(r, 1);
    raft_become_leader(r);

    /* need committed entries to snapshot */
    raft_entry_t ety = {};
    ety.data.buf = "aaa";
    ety.data.len = 3;
    ety.id = 1;
    ety.term = 1;
    raft_append_entry(r, &ety);

    ety.id = 2;
    raft_append_entry(r, &ety);

    raft_set_commit_idx(r, 2);

    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
        .applylog = __raft_applylog,
    };
    raft_set_callbacks(r, &funcs, NULL);
    raft_apply_all(r);

    CuAssertIntEquals(tc, 1, raft_is_apply_allowed(r));
    CuAssertIntEquals(tc, 0, raft_begin_snapshot(r, 0));
    CuAssertIntEquals(tc, 0, raft_is_apply_allowed(r));
}

void TestRaft_server_is_apply_allowed_returns_1_during_nonblocking_snapshot(CuTest * tc)
{
    void *r = raft_new();
    raft_set_callbacks(r, &generic_funcs, NULL);
    raft_add_node(r, NULL, 1, 1);
    raft_set_current_term(r, 1);
    raft_become_leader(r);

    raft_entry_t ety = {};
    ety.data.buf = "aaa";
    ety.data.len = 3;
    ety.id = 1;
    ety.term = 1;
    raft_append_entry(r, &ety);

    ety.id = 2;
    raft_append_entry(r, &ety);

    raft_set_commit_idx(r, 2);

    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
        .applylog = __raft_applylog,
    };
    raft_set_callbacks(r, &funcs, NULL);
    raft_apply_all(r);

    CuAssertIntEquals(tc, 0, raft_begin_snapshot(r, RAFT_SNAPSHOT_NONBLOCKING_APPLY));
    CuAssertIntEquals(tc, 1, raft_is_apply_allowed(r));
}

/* Callback error propagation tests */

static int __raft_persist_term_fail(
    raft_server_t* raft,
    void *udata,
    raft_term_t term,
    int vote
    )
{
    return -1;
}

static int __raft_persist_vote_fail(
    raft_server_t* raft,
    void *udata,
    int vote
    )
{
    return -1;
}

static int __raft_log_offer_fail(
    raft_server_t* raft,
    void* udata,
    raft_entry_t *entry,
    raft_index_t entry_idx)
{
    return -1;
}

static int __raft_log_offer_shutdown(
    raft_server_t* raft,
    void* udata,
    raft_entry_t *entry,
    raft_index_t entry_idx)
{
    return RAFT_ERR_SHUTDOWN;
}

static int __raft_send_requestvote_fail(raft_server_t* raft,
                            void* udata,
                            raft_node_t* node,
                            msg_requestvote_t* msg)
{
    return -1;
}

static int __raft_send_appendentries_fail(raft_server_t* raft,
                              void* udata,
                              raft_node_t* node,
                              msg_appendentries_t* msg)
{
    return -1;
}

static int __raft_log_poll_fail(
    raft_server_t* raft,
    void* udata,
    raft_entry_t *entry,
    raft_index_t entry_idx)
{
    return -1;
}

static int __raft_log_pop_fail(
    raft_server_t* raft,
    void* udata,
    raft_entry_t *entry,
    raft_index_t entry_idx)
{
    return -1;
}

void TestRaft_server_persist_term_fail_propagates_from_set_current_term(CuTest * tc)
{
    void *r = raft_new();
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term_fail,
    };
    raft_set_callbacks(r, &funcs, NULL);
    /* term must increase for persist_term to be called */
    CuAssertIntEquals(tc, -1, raft_set_current_term(r, 1));
    /* term should not have been updated */
    CuAssertIntEquals(tc, 0, raft_get_current_term(r));
}

void TestRaft_server_persist_vote_fail_propagates_from_vote_for_nodeid(CuTest * tc)
{
    void *r = raft_new();
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote_fail,
    };
    raft_set_callbacks(r, &funcs, NULL);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    CuAssertIntEquals(tc, -1, raft_vote_for_nodeid(r, 2));
}

void TestRaft_server_log_offer_fail_propagates_from_append_entry(CuTest * tc)
{
    void *r = raft_new();
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .log_offer = __raft_log_offer_fail,
    };
    raft_set_callbacks(r, &funcs, NULL);

    raft_entry_t ety = {};
    ety.data.buf = "aaa";
    ety.data.len = 3;
    ety.id = 1;
    ety.term = 1;
    CuAssertIntEquals(tc, -1, raft_append_entry(r, &ety));
    /* entry should not have been added */
    CuAssertIntEquals(tc, 0, raft_get_current_idx(r));
}

void TestRaft_server_log_offer_shutdown_propagates_from_append_entry(CuTest * tc)
{
    void *r = raft_new();
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .log_offer = __raft_log_offer_shutdown,
    };
    raft_set_callbacks(r, &funcs, NULL);

    raft_entry_t ety = {};
    ety.data.buf = "aaa";
    ety.data.len = 3;
    ety.id = 1;
    ety.term = 1;
    CuAssertIntEquals(tc, RAFT_ERR_SHUTDOWN, raft_append_entry(r, &ety));
}

void TestRaft_server_send_appendentries_fail_propagates_from_send_all(CuTest * tc)
{
    void *r = raft_new();
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
        .send_appendentries = __raft_send_appendentries,
    };
    raft_set_callbacks(r, &funcs, NULL);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);
    raft_become_leader(r);

    /* now switch to the failing callback */
    raft_cbs_t funcs2 = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
        .send_appendentries = __raft_send_appendentries_fail,
    };
    raft_set_callbacks(r, &funcs2, NULL);

    CuAssertIntEquals(tc, -1, raft_send_appendentries_all(r));
}

void TestRaft_server_applylog_shutdown_propagates_from_periodic(CuTest * tc)
{
    void *r = raft_new();
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
        .applylog = __raft_applylog_shutdown,
    };
    raft_set_callbacks(r, &funcs, NULL);
    raft_add_node(r, NULL, 1, 1);
    raft_set_current_term(r, 1);
    raft_become_leader(r);

    raft_entry_t ety = {};
    ety.data.buf = "aaa";
    ety.data.len = 3;
    ety.id = 1;
    ety.term = 1;
    raft_append_entry(r, &ety);
    raft_set_commit_idx(r, 1);

    CuAssertIntEquals(tc, RAFT_ERR_SHUTDOWN, raft_periodic(r, 1));
}

void TestRaft_server_log_poll_fail_propagates_from_poll_entry(CuTest * tc)
{
    void *r = raft_new();
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .log_poll = __raft_log_poll_fail,
    };
    raft_set_callbacks(r, &funcs, NULL);

    raft_entry_t ety = {};
    ety.data.buf = "aaa";
    ety.data.len = 3;
    ety.id = 1;
    ety.term = 1;
    raft_append_entry(r, &ety);

    raft_entry_t *polled;
    CuAssertIntEquals(tc, -1, raft_poll_entry(r, &polled));
}

/* raft_delete_entry_from_idx is not in a public header but is defined in raft_server.c */
int raft_delete_entry_from_idx(raft_server_t* me_, raft_index_t idx);

void TestRaft_server_log_pop_fail_propagates_from_delete_entry(CuTest * tc)
{
    void *r = raft_new();
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .log_pop = __raft_log_pop_fail,
    };
    raft_set_callbacks(r, &funcs, NULL);

    raft_entry_t ety = {};
    ety.data.buf = "aaa";
    ety.data.len = 3;
    ety.id = 1;
    ety.term = 1;
    raft_append_entry(r, &ety);

    CuAssertIntEquals(tc, -1, raft_delete_entry_from_idx(r, 1));
    /* entry should still be there since pop failed */
    CuAssertIntEquals(tc, 1, raft_get_current_idx(r));
}

void TestRaft_server_persist_term_fail_in_recv_requestvote(CuTest * tc)
{
    void *r = raft_new();
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term_fail,
    };
    raft_set_callbacks(r, &funcs, NULL);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    msg_requestvote_t rv = {
        .term = 2,
        .candidate_id = 2,
        .last_log_idx = 0,
        .last_log_term = 0,
    };
    msg_requestvote_response_t rvr;
    int e = raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);
    CuAssertIntEquals(tc, -1, e);
    CuAssertIntEquals(tc, 0, rvr.vote_granted);
}

void TestRaft_server_persist_vote_fail_in_recv_requestvote(CuTest * tc)
{
    void *r = raft_new();
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote_fail,
    };
    raft_set_callbacks(r, &funcs, NULL);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    msg_requestvote_t rv = {
        .term = 1,
        .candidate_id = 2,
        .last_log_idx = 0,
        .last_log_term = 0,
    };
    msg_requestvote_response_t rvr;
    int e = raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);
    CuAssertIntEquals(tc, -1, e);
    CuAssertIntEquals(tc, 0, rvr.vote_granted);
}

/* T5: Election edge cases */

void TestRaft_follower_dont_grant_vote_if_candidate_log_is_shorter_same_term(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_current_term(r, 1);

    /* voter has 3 entries at term 1 */
    raft_entry_t ety = {};
    ety.term = 1;
    ety.id = 100;
    ety.data.len = 4;
    ety.data.buf = (unsigned char*)"aaa";
    raft_append_entry(r, &ety);
    ety.id = 101;
    raft_append_entry(r, &ety);
    ety.id = 102;
    raft_append_entry(r, &ety);

    /* candidate has same last_log_term but shorter log (idx=2 vs our 3) */
    msg_requestvote_t rv = {
        .term = 1,
        .candidate_id = 2,
        .last_log_idx = 2,
        .last_log_term = 1,
    };
    msg_requestvote_response_t rvr;
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);
    CuAssertIntEquals(tc, 0, rvr.vote_granted);
}

void TestRaft_follower_dont_grant_vote_if_candidate_last_log_term_is_lower(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_current_term(r, 2);

    /* voter has 1 entry at term 2 */
    raft_entry_t ety = {};
    ety.term = 2;
    ety.id = 100;
    ety.data.len = 4;
    ety.data.buf = (unsigned char*)"aaa";
    raft_append_entry(r, &ety);

    /* candidate has longer log but lower last_log_term */
    msg_requestvote_t rv = {
        .term = 2,
        .candidate_id = 2,
        .last_log_idx = 5,
        .last_log_term = 1,
    };
    msg_requestvote_response_t rvr;
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);
    CuAssertIntEquals(tc, 0, rvr.vote_granted);
}

void TestRaft_follower_grant_vote_when_log_is_empty(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* empty log should grant vote to any candidate */
    msg_requestvote_t rv = {
        .term = 1,
        .candidate_id = 2,
        .last_log_idx = 5,
        .last_log_term = 3,
    };
    msg_requestvote_response_t rvr;
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);
    CuAssertIntEquals(tc, 1, rvr.vote_granted);
}

void TestRaft_follower_grant_vote_uses_snapshot_term_when_entry_is_null(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* Use begin_load_snapshot to set up snapshotted state properly.
     * This creates state where current_idx == snapshot_last_idx
     * but no log entry exists — testing the snapshot branch in __should_grant_vote */
    raft_begin_load_snapshot(r, 2, 5);
    raft_end_load_snapshot(r);

    /* re-add the other node since load_snapshot removes non-self nodes */
    raft_add_node(r, NULL, 2, 0);

    /* now at term 2 (set by load_snapshot), bump to 3 */
    raft_set_current_term(r, 3);

    /* candidate has higher last_log_term than snapshot term (2) → grant */
    msg_requestvote_t rv = {
        .term = 3,
        .candidate_id = 2,
        .last_log_idx = 5,
        .last_log_term = 3,
    };
    msg_requestvote_response_t rvr;
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);
    CuAssertIntEquals(tc, 1, rvr.vote_granted);

    /* candidate has lower last_log_term than snapshot term → reject */
    raft_set_current_term(r, 4);
    rv.term = 4;
    rv.last_log_term = 1;
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);
    CuAssertIntEquals(tc, 0, rvr.vote_granted);
}

void TestRaft_election_timeout_with_zero_elapsed_does_not_trigger(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
        .send_requestvote = __raft_send_requestvote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_election_timeout(r, 1000);

    /* 0ms elapsed should not trigger election */
    raft_periodic(r, 0);
    CuAssertTrue(tc, 1 == raft_is_follower(r));
}

void TestRaft_candidate_recv_requestvote_from_other_candidate_same_term(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
        .send_requestvote = __raft_send_requestvote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);

    raft_set_current_term(r, 1);
    raft_become_candidate(r);
    CuAssertTrue(tc, 1 == raft_is_candidate(r));
    CuAssertIntEquals(tc, 2, raft_get_current_term(r));

    /* another candidate with same term requests vote — already voted for self */
    msg_requestvote_t rv = {
        .term = 2,
        .candidate_id = 3,
        .last_log_idx = 0,
        .last_log_term = 0,
    };
    msg_requestvote_response_t rvr;
    raft_recv_requestvote(r, raft_get_node(r, 3), &rv, &rvr);
    /* should not grant — already voted for self this term */
    CuAssertIntEquals(tc, 0, rvr.vote_granted);
    /* should remain candidate */
    CuAssertTrue(tc, 1 == raft_is_candidate(r));
}

void TestRaft_candidate_duplicate_vote_response_does_not_double_count(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
        .send_requestvote = __raft_send_requestvote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);
    raft_add_node(r, NULL, 4, 0);
    raft_add_node(r, NULL, 5, 0);

    raft_set_current_term(r, 1);
    raft_become_candidate(r);
    CuAssertIntEquals(tc, 2, raft_get_current_term(r));

    /* receive vote from node 2 */
    msg_requestvote_response_t rvr = {
        .term = 2,
        .vote_granted = RAFT_REQUESTVOTE_ERR_GRANTED,
    };
    raft_recv_requestvote_response(r, raft_get_node(r, 2), &rvr);
    /* 2 votes: self + node 2 — not majority of 5 */
    CuAssertIntEquals(tc, 2, raft_get_nvotes_for_me(r));
    CuAssertTrue(tc, 1 == raft_is_candidate(r));

    /* duplicate vote from node 2 — vote_for_me is already set, count should stay same */
    raft_recv_requestvote_response(r, raft_get_node(r, 2), &rvr);
    CuAssertIntEquals(tc, 2, raft_get_nvotes_for_me(r));
    CuAssertTrue(tc, 1 == raft_is_candidate(r));
}

void TestRaft_candidate_nvotes_for_me_includes_self_vote(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
        .send_requestvote = __raft_send_requestvote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);

    /* before election: no votes */
    CuAssertIntEquals(tc, 0, raft_get_nvotes_for_me(r));

    raft_set_current_term(r, 1);
    raft_become_candidate(r);

    /* after becoming candidate: 1 vote (self) */
    CuAssertIntEquals(tc, 1, raft_get_nvotes_for_me(r));

    /* receive vote from node 2 */
    msg_requestvote_response_t rvr = {
        .term = 2,
        .vote_granted = RAFT_REQUESTVOTE_ERR_GRANTED,
    };
    raft_recv_requestvote_response(r, raft_get_node(r, 2), &rvr);
    CuAssertIntEquals(tc, 2, raft_get_nvotes_for_me(r));

    /* receive vote from node 3 — majority reached, becomes leader */
    raft_recv_requestvote_response(r, raft_get_node(r, 3), &rvr);
    CuAssertTrue(tc, 1 == raft_is_leader(r));
}

void TestRaft_follower_rejects_vote_if_already_voted_for_another(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);

    raft_set_current_term(r, 1);

    /* vote for node 2 first */
    msg_requestvote_t rv = {
        .term = 1,
        .candidate_id = 2,
        .last_log_idx = 0,
        .last_log_term = 0,
    };
    msg_requestvote_response_t rvr;
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);
    CuAssertIntEquals(tc, 1, rvr.vote_granted);
    CuAssertIntEquals(tc, 2, raft_get_voted_for(r));

    /* node 3 requests vote in same term — should be rejected */
    rv.candidate_id = 3;
    raft_recv_requestvote(r, raft_get_node(r, 3), &rv, &rvr);
    CuAssertIntEquals(tc, 0, rvr.vote_granted);
    /* still voted for node 2 */
    CuAssertIntEquals(tc, 2, raft_get_voted_for(r));
}

void TestRaft_non_voting_node_does_not_grant_vote(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    /* self is non-voting */
    raft_add_non_voting_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    msg_requestvote_t rv = {
        .term = 1,
        .candidate_id = 2,
        .last_log_idx = 0,
        .last_log_term = 0,
    };
    msg_requestvote_response_t rvr;
    raft_recv_requestvote(r, raft_get_node(r, 2), &rv, &rvr);
    CuAssertIntEquals(tc, 0, rvr.vote_granted);
}

/* T6: AppendEntries edge cases */

void TestRaft_follower_recv_appendentries_first_entry_prev_log_idx_0(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* empty log, first AE ever */
    CuAssertIntEquals(tc, 0, raft_get_log_count(r));

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_idx = 0;
    ae.prev_log_term = 0;
    msg_entry_t e[1];
    memset(&e, 0, sizeof(msg_entry_t));
    e[0].term = 1;
    e[0].id = 1;
    e[0].data.buf = "aaa";
    e[0].data.len = 3;
    ae.entries = e;
    ae.n_entries = 1;

    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertIntEquals(tc, 1, aer.success);
    CuAssertIntEquals(tc, 1, raft_get_log_count(r));
    CuAssertIntEquals(tc, 1, aer.current_idx);
    CuAssertIntEquals(tc, 1, aer.first_idx);
}

/* Fills the TODO at line 1487 */
void TestRaft_follower_recv_appendentries_delete_entries_if_term_is_different(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_current_term(r, 1);

    /* append 3 entries at term 1 */
    raft_entry_t ety = {};
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = "aaa";
    ety.data.len = 3;
    raft_append_entry(r, &ety);
    ety.id = 2;
    raft_append_entry(r, &ety);
    ety.id = 3;
    raft_append_entry(r, &ety);
    CuAssertIntEquals(tc, 3, raft_get_log_count(r));

    /* send AE with entry at index 2 with different term */
    msg_appendentries_t ae;
    msg_appendentries_response_t aer;
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 2;
    ae.prev_log_idx = 1;
    ae.prev_log_term = 1;
    msg_entry_t e[1];
    memset(&e, 0, sizeof(msg_entry_t));
    e[0].term = 2;  /* different term than existing entry at idx 2 */
    e[0].id = 4;
    e[0].data.buf = "bbb";
    e[0].data.len = 3;
    ae.entries = e;
    ae.n_entries = 1;

    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertIntEquals(tc, 1, aer.success);
    /* entries 2 and 3 were deleted, new entry appended at idx 2 */
    CuAssertIntEquals(tc, 2, raft_get_log_count(r));
    /* verify the new entry has term 2 */
    raft_entry_t *appended = raft_get_entry_from_idx(r, 2);
    CuAssertTrue(tc, NULL != appended);
    CuAssertIntEquals(tc, 2, appended->term);
    CuAssertTrue(tc, !strncmp(appended->data.buf, "bbb", 3));
}

void TestRaft_follower_recv_appendentries_partial_overlap_some_match_some_new(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_current_term(r, 1);

    /* append 2 entries at term 1 */
    raft_entry_t ety = {};
    ety.term = 1;
    ety.id = 1;
    raft_append_entry(r, &ety);
    ety.id = 2;
    raft_append_entry(r, &ety);
    CuAssertIntEquals(tc, 2, raft_get_log_count(r));

    /* send AE that overlaps: entries at idx 1 and 2 match, idx 3 is new */
    msg_appendentries_t ae;
    msg_appendentries_response_t aer;
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_idx = 0;
    ae.prev_log_term = 0;
    msg_entry_t e[3];
    memset(&e, 0, sizeof(msg_entry_t) * 3);
    e[0].term = 1;  /* matches existing */
    e[0].id = 1;
    e[1].term = 1;  /* matches existing */
    e[1].id = 2;
    e[2].term = 1;  /* new */
    e[2].id = 3;
    ae.entries = e;
    ae.n_entries = 3;

    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertIntEquals(tc, 1, aer.success);
    /* 2 existing + 1 new = 3 entries */
    CuAssertIntEquals(tc, 3, raft_get_log_count(r));
    CuAssertIntEquals(tc, 3, aer.current_idx);
}

void TestRaft_follower_recv_appendentries_leader_commit_capped_to_current_idx(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    /* send 2 entries with leader_commit=10 (way beyond our log) */
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_idx = 0;
    ae.prev_log_term = 0;
    ae.leader_commit = 10;
    msg_entry_t e[2];
    memset(&e, 0, sizeof(msg_entry_t) * 2);
    e[0].term = 1;
    e[0].id = 1;
    e[1].term = 1;
    e[1].id = 2;
    ae.entries = e;
    ae.n_entries = 2;

    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertIntEquals(tc, 1, aer.success);
    /* commit_idx should be capped to current_idx (2), not leader_commit (10) */
    CuAssertIntEquals(tc, 2, raft_get_commit_idx(r));
}

void TestRaft_follower_recv_appendentries_commit_idx_not_decreased(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    /* send entries with leader_commit=3 */
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_idx = 0;
    ae.prev_log_term = 0;
    ae.leader_commit = 3;
    msg_entry_t e[4];
    memset(&e, 0, sizeof(msg_entry_t) * 4);
    e[0].term = 1;
    e[0].id = 1;
    e[1].term = 1;
    e[1].id = 2;
    e[2].term = 1;
    e[2].id = 3;
    e[3].term = 1;
    e[3].id = 4;
    ae.entries = e;
    ae.n_entries = 4;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertIntEquals(tc, 1, aer.success);
    CuAssertIntEquals(tc, 3, raft_get_commit_idx(r));

    /* send heartbeat with leader_commit=1 (lower) */
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_idx = 4;
    ae.prev_log_term = 1;
    ae.leader_commit = 1;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertIntEquals(tc, 1, aer.success);
    /* commit_idx should stay at 3, not decrease to 1 */
    CuAssertIntEquals(tc, 3, raft_get_commit_idx(r));
}

void TestRaft_follower_recv_appendentries_heartbeat_updates_commit_idx(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    /* first append some entries with no commit */
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_idx = 0;
    ae.prev_log_term = 0;
    ae.leader_commit = 0;
    msg_entry_t e[3];
    memset(&e, 0, sizeof(msg_entry_t) * 3);
    e[0].term = 1;
    e[0].id = 1;
    e[1].term = 1;
    e[1].id = 2;
    e[2].term = 1;
    e[2].id = 3;
    ae.entries = e;
    ae.n_entries = 3;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertIntEquals(tc, 1, aer.success);
    CuAssertIntEquals(tc, 0, raft_get_commit_idx(r));

    /* send pure heartbeat (n_entries=0) with leader_commit=2 */
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 1;
    ae.prev_log_idx = 3;
    ae.prev_log_term = 1;
    ae.leader_commit = 2;
    ae.n_entries = 0;
    ae.entries = NULL;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    CuAssertIntEquals(tc, 1, aer.success);
    /* heartbeat should update commit_idx to 2 */
    CuAssertIntEquals(tc, 2, raft_get_commit_idx(r));
}

void TestRaft_follower_recv_appendentries_prev_log_idx_in_compacted_region(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* load a snapshot up to idx=5, term=2 */
    CuAssertIntEquals(tc, 0, raft_begin_load_snapshot(r, 2, 5));
    CuAssertIntEquals(tc, 0, raft_end_load_snapshot(r));

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    /* send AE with prev_log_idx=3, which is in the compacted region
     * (below snapshot_last_idx=5) and no entry exists there */
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 3;
    ae.prev_log_idx = 3;
    ae.prev_log_term = 2;
    msg_entry_t e[1];
    memset(&e, 0, sizeof(msg_entry_t));
    e[0].term = 3;
    e[0].id = 4;
    ae.entries = e;
    ae.n_entries = 1;

    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    /* prev_log_idx=3 doesn't match snapshot_last_idx=5 and no entry exists,
     * so the follower should reject */
    CuAssertIntEquals(tc, 0, aer.success);
}

void TestRaft_follower_recv_appendentries_snapshot_prev_log_term_mismatch(
    CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    /* load a snapshot up to idx=5, term=2 */
    CuAssertIntEquals(tc, 0, raft_begin_load_snapshot(r, 2, 5));
    CuAssertIntEquals(tc, 0, raft_end_load_snapshot(r));

    msg_appendentries_t ae;
    msg_appendentries_response_t aer;

    /* send AE with prev_log_idx=5 (matches snapshot_last_idx) but wrong term */
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 3;
    ae.prev_log_idx = 5;
    ae.prev_log_term = 99;  /* does NOT match snapshot_last_term=2 */
    msg_entry_t e[1];
    memset(&e, 0, sizeof(msg_entry_t));
    e[0].term = 3;
    e[0].id = 6;
    ae.entries = e;
    ae.n_entries = 1;

    int rc = raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);
    /* this is a fatal inconsistency — SHUTDOWN */
    CuAssertIntEquals(tc, RAFT_ERR_SHUTDOWN, rc);
}

void TestRaft_leader_recv_appendentries_response_with_current_idx_0(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .send_appendentries = sender_appendentries,
    };

    void *sender = sender_new(NULL);
    void *r = raft_new();
    raft_set_callbacks(r, &funcs, sender);

    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);

    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);

    /* append an entry */
    raft_entry_t ety = {};
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = "aaa";
    ety.data.len = 3;
    raft_append_entry(r, &ety);

    /* simulate follower response with current_idx=0 (follower has nothing) */
    msg_appendentries_response_t aer;
    memset(&aer, 0, sizeof(msg_appendentries_response_t));
    aer.term = 1;
    aer.success = 0;
    aer.current_idx = 0;
    aer.first_idx = 1;

    int rc = raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);
    CuAssertIntEquals(tc, 0, rc);
    /* next_idx should be set to 1 (current_idx+1 or clamped) */
    CuAssertIntEquals(tc, 1, raft_node_get_next_idx(raft_get_node(r, 2)));
}

/* T7: Membership change coverage */

void TestRaft_membership_add_node_with_is_self_sets_my_node(CuTest * tc)
{
    void *r = raft_new();
    CuAssertTrue(tc, NULL == raft_get_my_node(r));
    CuAssertIntEquals(tc, -1, raft_get_nodeid(r));

    raft_node_t* n = raft_add_node(r, NULL, 1, 1);
    CuAssertTrue(tc, NULL != n);
    CuAssertIntEquals(tc, 1, raft_get_nodeid(r));
    CuAssertTrue(tc, n == raft_get_my_node(r));
}

void TestRaft_membership_remove_then_readd_same_id(CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_node_t* n = raft_add_node(r, NULL, 9, 0);
    CuAssertTrue(tc, NULL != n);
    CuAssertIntEquals(tc, 2, raft_get_num_nodes(r));

    raft_remove_node(r, n);
    CuAssertIntEquals(tc, 1, raft_get_num_nodes(r));
    CuAssertTrue(tc, NULL == raft_get_node(r, 9));

    /* re-add same ID */
    raft_node_t* n2 = raft_add_node(r, NULL, 9, 0);
    CuAssertTrue(tc, NULL != n2);
    CuAssertIntEquals(tc, 2, raft_get_num_nodes(r));
    CuAssertTrue(tc, NULL != raft_get_node(r, 9));
}

void TestRaft_membership_entry_is_cfg_change_for_all_types(CuTest * tc)
{
    raft_entry_t ety = {};

    ety.type = RAFT_LOGTYPE_ADD_NODE;
    CuAssertIntEquals(tc, 1, raft_entry_is_cfg_change(&ety));

    ety.type = RAFT_LOGTYPE_ADD_NONVOTING_NODE;
    CuAssertIntEquals(tc, 1, raft_entry_is_cfg_change(&ety));

    ety.type = RAFT_LOGTYPE_DEMOTE_NODE;
    CuAssertIntEquals(tc, 1, raft_entry_is_cfg_change(&ety));

    ety.type = RAFT_LOGTYPE_REMOVE_NODE;
    CuAssertIntEquals(tc, 1, raft_entry_is_cfg_change(&ety));

    ety.type = RAFT_LOGTYPE_NORMAL;
    CuAssertIntEquals(tc, 0, raft_entry_is_cfg_change(&ety));
}

void TestRaft_membership_entry_is_voting_cfg_change(CuTest * tc)
{
    raft_entry_t ety = {};

    /* ADD_NODE and DEMOTE_NODE are voting cfg changes */
    ety.type = RAFT_LOGTYPE_ADD_NODE;
    CuAssertIntEquals(tc, 1, raft_entry_is_voting_cfg_change(&ety));

    ety.type = RAFT_LOGTYPE_DEMOTE_NODE;
    CuAssertIntEquals(tc, 1, raft_entry_is_voting_cfg_change(&ety));

    /* ADD_NONVOTING_NODE and REMOVE_NODE are not voting cfg changes */
    ety.type = RAFT_LOGTYPE_ADD_NONVOTING_NODE;
    CuAssertIntEquals(tc, 0, raft_entry_is_voting_cfg_change(&ety));

    ety.type = RAFT_LOGTYPE_REMOVE_NODE;
    CuAssertIntEquals(tc, 0, raft_entry_is_voting_cfg_change(&ety));

    ety.type = RAFT_LOGTYPE_NORMAL;
    CuAssertIntEquals(tc, 0, raft_entry_is_voting_cfg_change(&ety));
}

void TestRaft_membership_voting_change_is_in_progress(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
        .log_offer = __raft_log_offer,
        .log_get_node_id = __raft_log_get_node_id,
        .send_appendentries = __raft_send_appendentries,
    };

    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_set_callbacks(r, &funcs, NULL);
    CuAssertIntEquals(tc, 0, raft_voting_change_is_in_progress(r));

    raft_become_leader(r);

    /* submit a voting cfg change (ADD_NODE) */
    msg_entry_t ety = {};
    ety.type = RAFT_LOGTYPE_ADD_NODE;
    ety.id = 1;
    ety.data.buf = "2";
    ety.data.len = 2;
    msg_entry_response_t cr;
    CuAssertIntEquals(tc, 0, raft_recv_entry(r, &ety, &cr));

    CuAssertIntEquals(tc, 1, raft_voting_change_is_in_progress(r));
}

void TestRaft_membership_one_voting_change_only_guard(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
        .log_offer = __raft_log_offer,
        .log_get_node_id = __raft_log_get_node_id,
        .send_appendentries = __raft_send_appendentries,
    };

    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_set_callbacks(r, &funcs, NULL);
    raft_become_leader(r);

    /* first voting change succeeds */
    msg_entry_t ety = {};
    ety.type = RAFT_LOGTYPE_ADD_NODE;
    ety.id = 1;
    ety.data.buf = "2";
    ety.data.len = 2;
    msg_entry_response_t cr;
    CuAssertIntEquals(tc, 0, raft_recv_entry(r, &ety, &cr));

    /* second voting change (DEMOTE_NODE) is rejected */
    msg_entry_t ety2 = {};
    ety2.type = RAFT_LOGTYPE_DEMOTE_NODE;
    ety2.id = 2;
    ety2.data.buf = "3";
    ety2.data.len = 2;
    CuAssertIntEquals(tc, RAFT_ERR_ONE_VOTING_CHANGE_ONLY, raft_recv_entry(r, &ety2, &cr));

    /* but a non-voting change (ADD_NONVOTING_NODE) is allowed */
    msg_entry_t ety3 = {};
    ety3.type = RAFT_LOGTYPE_ADD_NONVOTING_NODE;
    ety3.id = 3;
    ety3.data.buf = "4";
    ety3.data.len = 2;
    CuAssertIntEquals(tc, 0, raft_recv_entry(r, &ety3, &cr));
}

static int __membership_event_count_add;
static int __membership_event_count_remove;

static void __raft_notify_membership_event(
    raft_server_t* raft,
    void *user_data,
    raft_node_t *node,
    raft_entry_t *entry,
    raft_membership_e type)
{
    if (type == RAFT_MEMBERSHIP_ADD)
        __membership_event_count_add++;
    else if (type == RAFT_MEMBERSHIP_REMOVE)
        __membership_event_count_remove++;
}

void TestRaft_membership_notify_membership_event_fires(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
        .notify_membership_event = __raft_notify_membership_event,
    };

    __membership_event_count_add = 0;
    __membership_event_count_remove = 0;

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);

    raft_node_t* n1 = raft_add_node(r, NULL, 1, 1);
    CuAssertIntEquals(tc, 1, __membership_event_count_add);
    CuAssertIntEquals(tc, 0, __membership_event_count_remove);

    raft_node_t* n2 = raft_add_node(r, NULL, 2, 0);
    CuAssertIntEquals(tc, 2, __membership_event_count_add);

    raft_remove_node(r, n2);
    CuAssertIntEquals(tc, 1, __membership_event_count_remove);

    raft_remove_node(r, n1);
    CuAssertIntEquals(tc, 2, __membership_event_count_remove);
}

void TestRaft_membership_cfg_change_committed_via_ae_response(CuTest * tc)
{
    raft_cbs_t funcs = {
        .applylog = __raft_applylog,
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
        .log_offer = __raft_log_offer,
        .log_get_node_id = __raft_log_get_node_id,
        .send_appendentries = __raft_send_appendentries,
    };

    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_non_voting_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);
    raft_set_callbacks(r, &funcs, NULL);

    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);
    raft_set_commit_idx(r, 0);
    raft_set_last_applied_idx(r, 0);

    /* submit ADD_NODE for non-voting node 2 (promotes it to voting) */
    msg_entry_t ety = {};
    ety.type = RAFT_LOGTYPE_ADD_NODE;
    ety.id = 1;
    ety.data.buf = "2";
    ety.data.len = 2;
    msg_entry_response_t cr;
    CuAssertIntEquals(tc, 0, raft_recv_entry(r, &ety, &cr));

    CuAssertIntEquals(tc, 1, raft_voting_change_is_in_progress(r));

    /* node 3 responds, committing the entry */
    msg_appendentries_response_t aer = {
        .term = 1, .success = 1, .current_idx = 1, .first_idx = 0
    };
    raft_recv_appendentries_response(r, raft_get_node(r, 3), &aer);

    /* entry should be committed */
    CuAssertIntEquals(tc, 1, raft_get_commit_idx(r));
}

void TestRaft_membership_demote_node_becomes_non_voting(CuTest * tc)
{
    raft_cbs_t funcs = {
        .applylog = __raft_applylog,
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
        .log_offer = __raft_log_offer,
        .log_get_node_id = __raft_log_get_node_id,
        .send_appendentries = __raft_send_appendentries,
    };

    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_callbacks(r, &funcs, NULL);

    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);
    raft_set_commit_idx(r, 0);
    raft_set_last_applied_idx(r, 0);

    /* node 2 is currently voting */
    CuAssertTrue(tc, raft_node_is_voting(raft_get_node(r, 2)));

    /* submit a DEMOTE_NODE entry for node 2 */
    msg_entry_t ety = {};
    ety.type = RAFT_LOGTYPE_DEMOTE_NODE;
    ety.id = 1;
    ety.data.buf = "2";
    ety.data.len = 2;
    msg_entry_response_t cr;
    CuAssertIntEquals(tc, 0, raft_recv_entry(r, &ety, &cr));

    /* commit by getting AE response from node 2 */
    msg_appendentries_response_t aer = {
        .term = 1, .success = 1, .current_idx = 1, .first_idx = 0
    };
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);

    /* apply committed entries */
    raft_apply_all(r);

    /* after apply, node 2 should have voting_committed=0 */
    CuAssertTrue(tc, !raft_node_is_voting_committed(raft_get_node(r, 2)));
}

void TestRaft_membership_remove_node_via_committed_entry(CuTest * tc)
{
    raft_cbs_t funcs = {
        .applylog = __raft_applylog,
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
        .log_offer = __raft_log_offer,
        .log_get_node_id = __raft_log_get_node_id,
        .send_appendentries = __raft_send_appendentries,
    };

    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);
    raft_set_callbacks(r, &funcs, NULL);

    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);
    raft_set_commit_idx(r, 0);
    raft_set_last_applied_idx(r, 0);

    CuAssertIntEquals(tc, 3, raft_get_num_nodes(r));

    /* submit REMOVE_NODE for node 3 */
    msg_entry_t ety = {};
    ety.type = RAFT_LOGTYPE_REMOVE_NODE;
    ety.id = 1;
    ety.data.buf = "3";
    ety.data.len = 2;
    msg_entry_response_t cr;
    CuAssertIntEquals(tc, 0, raft_recv_entry(r, &ety, &cr));

    /* commit by getting AE response from node 2 */
    msg_appendentries_response_t aer = {
        .term = 1, .success = 1, .current_idx = 1, .first_idx = 0
    };
    raft_recv_appendentries_response(r, raft_get_node(r, 2), &aer);

    /* apply committed entries */
    raft_apply_all(r);

    /* node 3 should be removed */
    CuAssertTrue(tc, NULL == raft_get_node(r, 3));
    CuAssertIntEquals(tc, 2, raft_get_num_nodes(r));
}

/* ===================== Memory management tests ===================== */

void TestRaft_memory_free_after_basic_setup(CuTest * tc)
{
    void *r = raft_new();
    CuAssertTrue(tc, NULL != r);
    raft_free(r);
    /* no crash = pass */
}

void TestRaft_memory_free_after_nodes_and_entries(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
        .log_offer = __raft_log_offer,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);
    raft_set_current_term(r, 1);

    /* append some log entries */
    raft_entry_t ety = {};
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = "aaa";
    ety.data.len = 3;
    raft_append_entry(r, &ety);
    ety.id = 2;
    ety.data.buf = "bbb";
    raft_append_entry(r, &ety);
    ety.id = 3;
    ety.data.buf = "ccc";
    raft_append_entry(r, &ety);

    raft_free(r);
    /* no crash = pass */
}

void TestRaft_memory_clear_resets_state_and_reusable(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
        .log_offer = __raft_log_offer,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 5);
    raft_entry_t ety = {};
    ety.term = 5;
    ety.id = 1;
    ety.data.buf = "data";
    ety.data.len = 4;
    raft_append_entry(r, &ety);
    raft_set_commit_idx(r, 1);

    raft_clear(r);

    /* state should be reset */
    CuAssertIntEquals(tc, 0, raft_get_current_term(r));
    CuAssertIntEquals(tc, -1, raft_get_voted_for(r));
    CuAssertIntEquals(tc, 0, raft_get_commit_idx(r));
    CuAssertIntEquals(tc, 0, raft_get_last_applied_idx(r));
    CuAssertIntEquals(tc, 0, raft_get_num_nodes(r));
    CuAssertIntEquals(tc, 0, raft_get_log_count(r));
    CuAssertTrue(tc, raft_is_follower(r));

    raft_free(r);
}

void TestRaft_memory_clear_then_readd_nodes(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
        .log_offer = __raft_log_offer,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 3);

    raft_clear(r);

    /* re-add nodes and operate */
    raft_add_node(r, NULL, 10, 1);
    raft_add_node(r, NULL, 20, 0);
    CuAssertIntEquals(tc, 2, raft_get_num_nodes(r));
    CuAssertTrue(tc, NULL != raft_get_node(r, 10));
    CuAssertTrue(tc, NULL != raft_get_node(r, 20));

    raft_set_current_term(r, 1);
    raft_entry_t ety = {};
    ety.term = 1;
    ety.id = 1;
    ety.data.buf = "new";
    ety.data.len = 3;
    raft_append_entry(r, &ety);
    CuAssertIntEquals(tc, 1, raft_get_log_count(r));

    raft_free(r);
}

/* Custom heap tracking */
static int custom_malloc_count = 0;
static int custom_calloc_count = 0;
static int custom_realloc_count = 0;
static int custom_free_count = 0;

static void *test_malloc(size_t sz)
{
    custom_malloc_count++;
    return malloc(sz);
}

static void *test_calloc(size_t nmemb, size_t sz)
{
    custom_calloc_count++;
    return calloc(nmemb, sz);
}

static void *test_realloc(void *ptr, size_t sz)
{
    custom_realloc_count++;
    return realloc(ptr, sz);
}

static void test_free(void *ptr)
{
    custom_free_count++;
    free(ptr);
}

void TestRaft_memory_custom_heap_functions(CuTest * tc)
{
    custom_malloc_count = 0;
    custom_calloc_count = 0;
    custom_realloc_count = 0;
    custom_free_count = 0;

    raft_set_heap_functions(test_malloc, test_calloc, test_realloc, test_free);

    void *r = raft_new();
    CuAssertTrue(tc, NULL != r);
    /* raft_new uses calloc for the server and log */
    CuAssertTrue(tc, custom_calloc_count > 0);

    raft_add_node(r, NULL, 1, 1);
    /* add_node uses realloc for node array and calloc for node */
    CuAssertTrue(tc, custom_realloc_count > 0);

    int free_before = custom_free_count;
    raft_free(r);
    CuAssertTrue(tc, custom_free_count > free_before);

    /* restore default heap functions */
    raft_set_heap_functions(malloc, calloc, realloc, free);
}

static void *test_calloc_fail(size_t nmemb, size_t sz)
{
    (void)nmemb;
    (void)sz;
    return NULL;
}

void TestRaft_memory_custom_heap_malloc_null_returns_nomem(CuTest * tc)
{
    /* Set calloc to always fail — raft_new should return NULL */
    raft_set_heap_functions(malloc, test_calloc_fail, realloc, free);

    void *r = raft_new();
    CuAssertTrue(tc, NULL == r);

    /* restore default heap functions */
    raft_set_heap_functions(malloc, calloc, realloc, free);
}

/* ===================== End memory management tests ================= */

/* ===================== Entry type helpers and misc API tests ======= */

void TestRaft_misc_entry_response_committed_returns_0_for_uncommitted(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .send_appendentries = __raft_send_appendentries,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);
    raft_set_commit_idx(r, 0);

    msg_entry_t ety = {};
    ety.id = 1;
    ety.data.buf = "aaa";
    ety.data.len = 3;
    msg_entry_response_t cr;
    raft_recv_entry(r, &ety, &cr);

    /* not yet committed */
    CuAssertIntEquals(tc, 0, raft_msg_entry_response_committed(r, &cr));

    /* now commit it */
    raft_set_commit_idx(r, 1);
    CuAssertIntEquals(tc, 1, raft_msg_entry_response_committed(r, &cr));
}

void TestRaft_misc_entry_response_committed_returns_neg1_for_invalidated(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .send_appendentries = __raft_send_appendentries,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);
    raft_set_commit_idx(r, 0);

    msg_entry_t ety = {};
    ety.id = 1;
    ety.data.buf = "aaa";
    ety.data.len = 3;
    msg_entry_response_t cr;
    raft_recv_entry(r, &ety, &cr);
    CuAssertIntEquals(tc, 0, raft_msg_entry_response_committed(r, &cr));

    /* invalidate by receiving AE with higher term that replaces the entry */
    msg_appendentries_t ae;
    memset(&ae, 0, sizeof(msg_appendentries_t));
    ae.term = 2;
    ae.prev_log_idx = 0;
    ae.prev_log_term = 0;
    ae.leader_commit = 1;
    msg_entry_t e[1];
    memset(&e, 0, sizeof(msg_entry_t));
    e[0].term = 2;
    e[0].id = 999;
    e[0].data.buf = "bbb";
    e[0].data.len = 3;
    ae.entries = e;
    ae.n_entries = 1;
    msg_appendentries_response_t aer;
    raft_recv_appendentries(r, raft_get_node(r, 2), &ae, &aer);

    CuAssertIntEquals(tc, -1, raft_msg_entry_response_committed(r, &cr));
}

void TestRaft_misc_vote_and_get_voted_for_round_trip(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);

    /* initially voted_for is -1 */
    CuAssertIntEquals(tc, -1, raft_get_voted_for(r));

    /* vote for node 2 via raft_vote */
    raft_node_t* node2 = raft_get_node(r, 2);
    CuAssertIntEquals(tc, 0, raft_vote(r, node2));
    CuAssertIntEquals(tc, 2, raft_get_voted_for(r));
}

void TestRaft_misc_vote_for_nodeid_round_trip(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);
    raft_add_node(r, NULL, 1, 1);
    raft_add_node(r, NULL, 2, 0);
    raft_set_current_term(r, 1);

    CuAssertIntEquals(tc, 0, raft_vote_for_nodeid(r, 2));
    CuAssertIntEquals(tc, 2, raft_get_voted_for(r));
}

void TestRaft_misc_set_commit_idx_get_commit_idx_round_trip(CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    CuAssertIntEquals(tc, 0, raft_get_commit_idx(r));

    /* add entries so commit_idx can advance */
    raft_entry_t ety1 = {};
    ety1.term = 1;
    ety1.id = 1;
    ety1.data.buf = "a";
    ety1.data.len = 1;
    raft_append_entry(r, &ety1);

    raft_entry_t ety2 = {};
    ety2.term = 1;
    ety2.id = 2;
    ety2.data.buf = "b";
    ety2.data.len = 1;
    raft_append_entry(r, &ety2);

    raft_set_commit_idx(r, 1);
    CuAssertIntEquals(tc, 1, raft_get_commit_idx(r));

    raft_set_commit_idx(r, 2);
    CuAssertIntEquals(tc, 2, raft_get_commit_idx(r));
}

void TestRaft_misc_become_leader_sets_state(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .send_appendentries = __raft_send_appendentries,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);
    raft_add_node(r, NULL, 1, 1);
    raft_set_current_term(r, 1);

    CuAssertIntEquals(tc, RAFT_STATE_FOLLOWER, raft_get_state(r));
    raft_become_leader(r);
    CuAssertIntEquals(tc, RAFT_STATE_LEADER, raft_get_state(r));
}

void TestRaft_misc_become_follower_from_leader(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
        .send_appendentries = __raft_send_appendentries,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);
    raft_add_node(r, NULL, 1, 1);
    raft_set_current_term(r, 1);

    raft_become_leader(r);
    CuAssertIntEquals(tc, RAFT_STATE_LEADER, raft_get_state(r));

    raft_become_follower(r);
    CuAssertIntEquals(tc, RAFT_STATE_FOLLOWER, raft_get_state(r));
}

void TestRaft_misc_get_entry_from_idx_returns_null_for_invalid(CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);

    /* no entries — idx 1 should return NULL */
    CuAssertTrue(tc, NULL == raft_get_entry_from_idx(r, 1));
    /* idx 0 should also return NULL */
    CuAssertTrue(tc, NULL == raft_get_entry_from_idx(r, 0));
}

void TestRaft_misc_get_entry_from_idx_returns_correct_entry(CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);

    raft_entry_t ety = {};
    ety.term = 1;
    ety.id = 42;
    ety.data.buf = "hello";
    ety.data.len = 5;
    raft_append_entry(r, &ety);

    raft_entry_t* result = raft_get_entry_from_idx(r, 1);
    CuAssertTrue(tc, NULL != result);
    CuAssertIntEquals(tc, 42, result->id);
    CuAssertIntEquals(tc, 1, result->term);

    /* beyond current log returns NULL */
    CuAssertTrue(tc, NULL == raft_get_entry_from_idx(r, 2));
}

void TestRaft_misc_get_node_returns_null_for_unknown_id(CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);

    CuAssertTrue(tc, NULL != raft_get_node(r, 1));
    CuAssertTrue(tc, NULL == raft_get_node(r, 99));
    CuAssertTrue(tc, NULL == raft_get_node(r, 0));
}

void TestRaft_misc_get_node_from_idx_returns_correct_node(CuTest * tc)
{
    void *r = raft_new();
    raft_add_node(r, NULL, 10, 1);
    raft_add_node(r, NULL, 20, 0);
    raft_add_node(r, NULL, 30, 0);

    raft_node_t* n0 = raft_get_node_from_idx(r, 0);
    CuAssertTrue(tc, NULL != n0);
    CuAssertIntEquals(tc, 10, raft_node_get_id(n0));

    raft_node_t* n1 = raft_get_node_from_idx(r, 1);
    CuAssertTrue(tc, NULL != n1);
    CuAssertIntEquals(tc, 20, raft_node_get_id(n1));

    raft_node_t* n2 = raft_get_node_from_idx(r, 2);
    CuAssertTrue(tc, NULL != n2);
    CuAssertIntEquals(tc, 30, raft_node_get_id(n2));

}

void TestRaft_misc_poll_entry_returns_oldest_and_removes(CuTest * tc)
{
    raft_cbs_t funcs = {
        .persist_term = __raft_persist_term,
    };

    void *r = raft_new();
    raft_set_callbacks(r, &funcs, NULL);
    raft_add_node(r, NULL, 1, 1);

    raft_entry_t ety1 = {};
    ety1.term = 1;
    ety1.id = 1;
    ety1.data.buf = "aaa";
    ety1.data.len = 3;
    raft_append_entry(r, &ety1);

    raft_entry_t ety2 = {};
    ety2.term = 1;
    ety2.id = 2;
    ety2.data.buf = "bbb";
    ety2.data.len = 3;
    raft_append_entry(r, &ety2);

    CuAssertIntEquals(tc, 2, raft_get_current_idx(r));

    raft_entry_t* polled = NULL;
    CuAssertIntEquals(tc, 0, raft_poll_entry(r, &polled));
    CuAssertTrue(tc, NULL != polled);
    CuAssertIntEquals(tc, 1, polled->id);

    /* after polling, current_idx stays at 2, 1 entry remains */
    CuAssertIntEquals(tc, 2, raft_get_current_idx(r));
}

/* ===================== End entry type helpers and misc API tests ==== */

void TestRaft_membership_add_node_committed_sets_flags(CuTest * tc)
{
    raft_cbs_t funcs = {
        .applylog = __raft_applylog,
        .persist_term = __raft_persist_term,
        .persist_vote = __raft_persist_vote,
        .log_offer = __raft_log_offer,
        .log_get_node_id = __raft_log_get_node_id,
        .send_appendentries = __raft_send_appendentries,
    };

    void *r = raft_new();
    raft_add_node(r, NULL, 1, 1);
    raft_add_non_voting_node(r, NULL, 2, 0);
    raft_add_node(r, NULL, 3, 0);
    raft_set_callbacks(r, &funcs, NULL);

    raft_set_state(r, RAFT_STATE_LEADER);
    raft_set_current_term(r, 1);
    raft_set_commit_idx(r, 0);
    raft_set_last_applied_idx(r, 0);

    /* node 2 is non-voting, not yet addition-committed */
    CuAssertTrue(tc, !raft_node_is_voting(raft_get_node(r, 2)));
    CuAssertTrue(tc, !raft_node_is_addition_committed(raft_get_node(r, 2)));

    /* submit ADD_NODE entry for node 2 (promotes to voting) */
    msg_entry_t ety = {};
    ety.type = RAFT_LOGTYPE_ADD_NODE;
    ety.id = 1;
    ety.data.buf = "2";
    ety.data.len = 2;
    msg_entry_response_t cr;
    CuAssertIntEquals(tc, 0, raft_recv_entry(r, &ety, &cr));

    /* commit by getting AE response from node 3 */
    msg_appendentries_response_t aer = {
        .term = 1, .success = 1, .current_idx = 1, .first_idx = 0
    };
    raft_recv_appendentries_response(r, raft_get_node(r, 3), &aer);

    /* apply committed entries */
    raft_apply_all(r);

    /* after commit and apply, node 2 should be addition_committed and voting_committed */
    CuAssertTrue(tc, raft_node_is_addition_committed(raft_get_node(r, 2)));
    CuAssertTrue(tc, raft_node_is_voting_committed(raft_get_node(r, 2)));
}
