# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

C implementation of the Raft consensus protocol (Ongaro 2014). Self-contained library with no runtime dependencies — networking and persistence are handled by the user via callbacks.

## Build Commands

```bash
make tests           # Build and run unit tests (with gcov coverage)
make static          # Build static library (libraft.a)
make shared          # Build shared library (libraft.so)
make test_fuzzer     # Property-based log fuzzing (Python/Hypothesis)
make test_virtraft   # Cluster simulator - 7 runs with different seeds
make tests_full      # Clean + all tests (unit, fuzzer, simulator)
make amalgamation    # Generate single-header raft.h
make infer           # Facebook Infer static analysis
make download-contrib # Fetch test dependency (CLinkedListQueue)
```

Testing requires Python 3 with: cffi, hypothesis, colorama, coloredlogs, docopt, terminaltables.

## Architecture

**Callback-driven design**: The library handles consensus logic; the user implements callbacks (`raft_cbs_t`) for networking, persistence, and state machine application. Not thread-safe — requires external synchronization.

**Core types**:
- `raft_server_t` (opaque) — server state: term, vote, log, commit index, election timeouts
- `raft_node_t` (opaque) — peer tracking: next_idx, match_idx, voting status
- `log_private_t` — circular buffer log with dynamic capacity and base index for snapshots
- `raft_cbs_t` — callback struct the user must populate (send_requestvote, send_appendentries, persist_vote, persist_term, log_offer, log_poll, log_pop, applylog, etc.)

**Message flow**: RequestVote/AppendEntries RPCs with response types. State transitions: Follower → Candidate → Leader (or back to Follower on higher term).

**Membership changes** use a 2-phase approach: add as non-voting (`RAFT_LOGTYPE_ADD_NONVOTING_NODE`), wait for catch-up (`node_has_sufficient_logs` callback), then promote (`RAFT_LOGTYPE_ADD_NODE`).

**Snapshotting**: User-initiated via `raft_begin_snapshot()` / `raft_end_snapshot()`. Receiver loads via `raft_begin_load_snapshot()`.

## Code Conventions

- Public API prefix: `raft_`, log functions: `log_`, messages: `msg_`
- Internal/private functions: `__` double-underscore prefix
- Types use `_t` suffix
- Return codes: 0 = success, negative = error (`RAFT_ERR_*`), positive = specific conditions
- Pluggable heap via `raft_set_heap_functions()`
- Persistent callbacks MUST flush to disk before returning

## Test Framework

CuTest (embedded in `tests/`). Tests link directly against source files. Key test files:
- `test_server.c` (~100+ cases covering elections, replication, membership, snapshots)
- `test_log.c` (circular buffer, index management)
- `test_snapshotting.c`
- `virtraft2.py` — cluster simulator with chaos (partitions, drops, duplicates) validating safety invariants
