# Plan 001: Improve Test Coverage for Raft Consensus Library

**Goal**: Extend the existing test suite to achieve comprehensive coverage of all public API functions, error paths, edge cases, and safety-critical code paths — ensuring this is a robust foundation to build on.

**Branch**: `tests/improve-coverage` (from `master`, suitable for upstream contribution)

**Baseline**: 157 tests across 5 test files. 118 tests in test_server.c, 17 in test_log.c, 19 in test_snapshotting.c, 2 in test_node.c, 1 in test_scenario.c.

**Test framework**: CuTest (embedded). Tests link directly against source files. Register tests in `tests/main_test.c`.

---

## Progress

| Task | Status | Description |
|------|--------|-------------|
| T1   | TASK:COMPLETE | Node API coverage (test_node.c) |
| T2   | TASK:COMPLETE | Log edge cases (test_log.c) |
| T3   | TASK:COMPLETE | Server property accessors (test_server.c) |
| T4   | TASK:COMPLETE | Callback error propagation (test_server.c) |
| T5   | TASK:COMPLETE | Election edge cases (test_server.c) |
| T6   | TASK:COMPLETE | AppendEntries edge cases (test_server.c) |
| T7   | TASK:COMPLETE | Membership change coverage (test_server.c) |
| T8   | TASK:COMPLETE | Snapshot edge cases (test_snapshotting.c) |
| T9   | TASK:PENDING | Memory management (test_server.c) |
| T10  | TASK:PENDING | Entry type helpers and misc API (test_server.c) |

---

## Task Details

### T1: Node API Coverage (test_node.c)
**Currently**: Only 2 tests (is_voting_by_default, set_nextIdx). 21 node functions exist, 14 untested.

**Tests to add**:
- `raft_node_get_id` returns correct ID
- `raft_node_get_udata` / `raft_node_set_udata` round-trip
- `raft_node_get_match_idx` / `raft_node_set_match_idx` (via raft_node.c internals — set_match_idx is internal)
- `raft_node_is_active` / `raft_node_set_active` toggle
- `raft_node_is_voting_committed` / `raft_node_set_voting_committed` toggle
- `raft_node_is_addition_committed` / `raft_node_set_addition_committed` toggle
- `raft_node_has_sufficient_logs` default and after set
- `raft_node_set_voting` toggles voting status
- `raft_node_new` with NULL udata
- `raft_node_new` with id=0

**DoD**: All 21 node functions exercised. Tests pass.

### T2: Log Edge Cases (test_log.c)
**Currently**: 17 tests covering basic operations. Missing edge cases around the circular buffer.

**Tests to add**:
- `log_delete` with idx=0 returns error
- `log_delete` with idx below base index
- `log_delete` with idx beyond current entries
- `log_poll` on empty log
- `log_poll` all entries until empty
- `log_get_at_idx` with out-of-range index (too low, too high)
- `log_get_at_idx` with base offset (after polling)
- Circular buffer wraparound: fill to capacity, poll some, append more, verify all entries correct
- `log_count` after mixed poll/append operations
- `log_new_from_base` (log with non-zero base index)
- Capacity growth: append enough entries to trigger multiple realloc cycles

**DoD**: All log functions exercised including boundary conditions. Circular buffer wraparound explicitly tested.

### T3: Server Property Accessors (test_server.c)
**Currently**: Many getters used incidentally but not directly tested. Several completely untested.

**Tests to add**:
- `raft_get_current_leader` returns -1 when no leader known
- `raft_get_current_leader_node` returns NULL when no leader, returns correct node when leader known
- `raft_get_last_applied_entry` returns NULL initially, returns correct entry after apply
- `raft_get_last_log_term` returns 0 with empty log, correct term after entries added
- `raft_get_num_voting_nodes` with mix of voting/non-voting nodes
- `raft_get_udata` returns the user data set via `raft_set_callbacks`
- `raft_get_snapshot_entry_idx` returns 0 initially
- `raft_get_snapshot_last_term` returns 0 initially
- `raft_get_first_entry_idx` returns 1 initially, updates after snapshot/poll
- `raft_get_state` returns correct enum values for each state
- `raft_get_nodeid` returns -1 before self node added
- `raft_get_my_node` returns NULL before self node added, correct node after
- `raft_is_apply_allowed` returns 1 normally, 0 during snapshot (without NONBLOCKING flag)

**DoD**: All property accessor functions have at least one direct test.

### T4: Callback Error Propagation (test_server.c)
**Currently**: Callbacks mostly return 0. No tests for callback failure paths.

**Tests to add**:
- `persist_term` callback returns -1 → `raft_set_current_term` propagates error
- `persist_vote` callback returns -1 → `raft_vote` / `raft_vote_for_nodeid` propagates error
- `log_offer` callback returns -1 → `raft_append_entry` propagates error
- `log_offer` callback returns RAFT_ERR_SHUTDOWN → entry append triggers shutdown
- `send_requestvote` callback returns -1 → `raft_periodic` (election) propagates
- `send_appendentries` callback returns -1 → `raft_periodic` (heartbeat) propagates
- `applylog` callback returns RAFT_ERR_SHUTDOWN → `raft_apply_all` returns shutdown
- `log_poll` callback returns -1 → `raft_poll_entry` propagates error
- `log_pop` callback returns -1 → log truncation propagates error
- `persist_term` callback returns -1 during recv_requestvote → error propagated
- `persist_vote` callback returns -1 during recv_requestvote → error propagated

**DoD**: Every callback type tested with at least one failure return. Error codes verified at the API boundary.

### T5: Election Edge Cases (test_server.c)
**Currently**: Core election flow well-tested. Missing edge cases.

**Tests to add**:
- Vote not granted when candidate log is shorter than voter's (log length comparison)
- Vote not granted when candidate last_log_term < voter last_log_term (term comparison)
- `__should_grant_vote` when entry is NULL and snapshot matches current_idx (the TODO at line ~558)
- Election with single-node cluster: immediately becomes leader
- Election timeout with 0 elapsed time — no election triggered
- Candidate receiving requestvote from another candidate with same term
- Pre-vote / vote deduplication: receiving duplicate vote responses
- `raft_get_nvotes_for_me` accuracy during election
- Candidate steps down on receiving appendentries with equal term
- Follower rejects vote if already voted for someone else this term

**DoD**: All __should_grant_vote branches covered. Election timing edge cases verified.

### T6: AppendEntries Edge Cases (test_server.c)
**Currently**: Good basic coverage. Missing conflict resolution edge cases.

**Tests to add**:
- AE with prev_log_idx=0 and prev_log_term=0 (first entry)
- AE with entries that conflict with existing entries (different term at same index) — verify truncation
- AE with entries that partially overlap existing log (some match, some new)
- AE where leader_commit > last new entry index — commit_idx capped correctly
- AE where leader_commit < current commit_idx — commit_idx not decreased
- AE from a node that is not recognized (NULL node parameter)
- AE response handling when node is no longer leader
- AE with n_entries=0 (pure heartbeat) updates commit index
- AE where prev_log_idx refers to a snapshotted entry — RAFT_ERR_NEEDS_SNAPSHOT returned
- AE response with current_idx=0

**DoD**: All AppendEntries branches in raft_recv_appendentries exercised.

### T7: Membership Change Coverage (test_server.c)
**Currently**: Basic add/remove tested. Missing multi-phase and error paths.

**Tests to add**:
- `raft_add_node` with duplicate ID returns NULL
- `raft_add_non_voting_node` with duplicate ID returns NULL
- `raft_add_node` with is_self=1 sets the server's node
- `raft_remove_node` then re-add with same ID
- `raft_entry_is_cfg_change` returns 1 for all 4 membership types, 0 for NORMAL
- `raft_entry_is_voting_cfg_change` returns 1 for ADD_NODE and DEMOTE_NODE only
- `raft_voting_change_is_in_progress` returns 1 when non-voting add is uncommitted
- RAFT_ERR_ONE_VOTING_CHANGE_ONLY when attempting second config change
- Membership change committed via appendentries response (match_idx advances)
- `notify_membership_event` callback fires on add and remove
- Demote node → node becomes non-voting and inactive
- Remove node → node removed from nodes array

**DoD**: All RAFT_LOGTYPE_* entry types tested through apply. Config change guards verified.

### T8: Snapshot Edge Cases (test_snapshotting.c)
**Currently**: 19 tests covering main flows. Missing boundary conditions.

**Tests to add**:
- `raft_cancel_snapshot` when no snapshot in progress — returns error
- `raft_set_snapshot_metadata` sets term and idx correctly, verified via getters
- `raft_begin_snapshot` with flags=0 vs flags=RAFT_SNAPSHOT_NONBLOCKING_APPLY — verify apply behavior
- `raft_begin_load_snapshot` with last_included_index=0 — returns error
- `raft_begin_load_snapshot` removes all non-self nodes
- `raft_end_load_snapshot` updates commit_idx and last_applied_idx
- `raft_snapshot_is_in_progress` returns correct value at each phase
- `raft_get_snapshot_last_idx` / `raft_get_snapshot_last_term` after snapshot
- Snapshot during election — election suppressed
- Periodic tick during snapshot — no election timeout
- `raft_begin_snapshot` when commit_idx equals last_applied_idx (nothing to snapshot)

**DoD**: All snapshot API functions directly tested. State machine transitions during snapshot verified.

### T9: Memory Management (test_server.c)
**Currently**: No tests for raft_free, raft_clear, or custom heap.

**Tests to add**:
- `raft_free` after basic setup — no crash (valgrind-safe)
- `raft_free` after adding nodes and log entries — no crash
- `raft_clear` resets state but server is reusable
- `raft_clear` then re-add nodes and operate normally
- `raft_set_heap_functions` with custom malloc/calloc/realloc/free — all allocations routed through custom functions (use counters)
- Custom heap malloc returning NULL → RAFT_ERR_NOMEM from raft_new (if applicable)

**DoD**: raft_free and raft_clear exercised without crashes. Custom heap integration verified.

### T10: Entry Type Helpers and Misc API (test_server.c)
**Currently**: Some functions tested incidentally but deserve direct tests.

**Tests to add**:
- `raft_msg_entry_response_committed` returns 0 for uncommitted entry, 1 for committed
- `raft_msg_entry_response_committed` returns -1 for invalidated entry (term mismatch)
- `raft_vote` / `raft_vote_for_nodeid` round-trip with `raft_get_voted_for`
- `raft_set_commit_idx` / `raft_get_commit_idx` round-trip
- `raft_become_leader` directly (dangerous API) — verify state
- `raft_become_follower` from leader — verify state change
- `raft_get_entry_from_idx` returns NULL for invalid index
- `raft_get_entry_from_idx` returns correct entry
- `raft_get_node` returns NULL for unknown ID
- `raft_get_node_from_idx` returns correct node by position
- `raft_poll_entry` returns the oldest entry and removes it

**DoD**: All remaining untested public API functions exercised.

---

## Implementation Notes

- Follow existing test style: `void TestRaft_<area>_<description>(CuTest *tc)`
- Use existing mock infrastructure in `mock_send_functions.c`
- Register all new tests in `tests/main_test.c`
- Run `make tests` after each task (will succeed even without gcov on this branch — just ignore the gcov error at the end)
- Each task = one commit with clear message
- Avoid changing any production code — this is purely additive test work
- If a test reveals a genuine bug, note it but don't fix it (separate concern for upstream)
