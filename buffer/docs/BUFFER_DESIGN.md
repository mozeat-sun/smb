# ZOO Buffer Module Design

## 1. Purpose

This document describes the concrete software design of the ZOO buffer module as implemented today. It focuses on how the list and queue are structured internally, how they coordinate concurrency, and what design tradeoffs downstream users need to understand.

## 2. Design Overview

The module is split into two layers:

- `zoo_list`: a bounded, pointer-based, thread-safe singly linked list
- `zoo_queue`: a bounded, pointer-based, thread-safe message queue implemented on top of `zoo_list`

The queue composes two lists internally:

- A message list holding queue nodes
- An observer list holding observer registrations

This gives the queue a small surface area while reusing list locking and traversal behavior.

## 3. Public Design Intent

### 3.1 List API Intent

The list API is designed for modules that need:

- FIFO-style use through `push_back` and `pop_front`
- LIFO-style use through `push_front` and `pop_front`, or `push_back` and `pop_back`
- Occasional indexed access and removal
- Opaque handle usage with internal synchronization

The list stores raw `void*` payload pointers. It does not copy payload bytes and does not know payload size.

### 3.2 Queue API Intent

The queue API is designed for producer-consumer workflows where producers enqueue work and consumers dequeue message records containing:

- `msg`
- `context`
- `handler`
- `user_data`
- internal `priority`
- internal `arrived_timestamp`

The queue does not execute callbacks on its own when a message is dequeued. It returns the callback pointer to the caller, which keeps execution control outside the buffer module.

## 4. Internal Data Model

### 4.1 List Structures

The list implementation in [buffer/src/zoo_list.c](/home/mozeat/zoo/buffer/src/zoo_list.c) uses:

- `NODE_STRUCT`
  - `void* data`
  - `NODE_STRUCT* next`
- `ZOO_LIST_STRUCT`
  - `NODE_STRUCT* head`
  - `NODE_STRUCT* tail`
  - `ZOO_SIZE_T size`
  - `ZOO_SIZE_T max_list_length`
  - `ZOO_MUTEX_T mutex`
  - `ZOO_COND_T cond`

The list is singly linked. `tail` is maintained to keep append constant time. Because there is no `prev` pointer, `pop_back` remains linear.

### 4.2 Queue Structures

The queue implementation in [buffer/src/zoo_queue.c](/home/mozeat/zoo/buffer/src/zoo_queue.c) uses:

- `QUEUE_NODE_STRUCT`
  - `void* msg`
  - `void* context`
  - `ZOO_QUEUED_HANDLER handler`
  - `void* user_data`
  - `ZOO_INT32 priority`
  - `ZOO_TIME_T arrived_timestamp`
- `QUEUE_OBSERVER_STRUCT`
  - fixed-size `name` buffer
  - `ON_QUEUE_CHANGED_HANDLER handler`
  - `void* user_data`
- `ZOO_QUEUE_STRUCT`
  - `ZOO_LIST_HANDLE messages`
  - `ZOO_LIST_HANDLE observers`
  - `ZOO_SIZE_T max_queue_size`
  - `ZOO_ATOMIC_BOOL exit_flag`
  - `ZOO_MUTEX_T messages_mutex`
  - `ZOO_MUTEX_T observers_mutex`
  - `ZOO_COND_T cond`

## 5. Memory Design

The module intentionally mixes allocation domains:

- List control block and list nodes use `malloc` and `free`
- Queue control block, queue nodes, observer records, and temporary sort arrays use `zoo_allocate_from_pool` and `zoo_free_to_pool`

This design reflects the queue's intended integration with the broader ZOO runtime, while keeping the list implementation lightweight and self-contained.

The payload pointers stored in the list and queue are not owned by the module. The module only owns wrapper nodes.

## 6. Concurrency Design

### 6.1 List Locking

Every public list operation acquires the internal list mutex before reading or mutating state. This makes each individual API call thread-safe and easy to reason about.

Consequences:

- Callers do not need external locking for single list operations.
- Multi-step read-modify-write sequences are not atomic across multiple API calls unless the caller introduces higher-level coordination.
- Functions that traverse the list while holding the lock serialize concurrent access.

### 6.2 Queue Locking

The queue has two external mutexes:

- `messages_mutex` protects enqueue, dequeue, priority changes, and sorting flow.
- `observers_mutex` protects observer registration and notification traversal.

The queue also relies on the list's own internal mutexes because `messages` and `observers` are backed by `zoo_list` instances. In practice this means queue operations take the queue mutex and then call into list APIs that take the list mutex.

This layered locking gives clear component ownership but adds extra lock traffic. The current design favors implementation reuse over minimal lock depth.

### 6.3 Blocking Dequeue

Blocking dequeue follows this model:

1. Consumer locks `messages_mutex`.
2. If the queue is empty, blocking is requested, and `exit_flag` is clear, the consumer waits on the queue condition variable.
3. Enqueue signals the condition variable after a successful push.
4. `zoo_queue_exit_blocking` and `zoo_destroy_queue` set `exit_flag` and broadcast to all waiters.
5. Woken consumers re-check state and return failure when exit has been requested.

This is a standard condition-variable loop with an explicit shutdown signal.

## 7. Behavioral Design Details

### 7.1 List Behavior

- Capacity is enforced before node allocation.
- `push_back` appends at `tail`.
- `push_front` inserts at `head`.
- `pop_front` is constant time.
- `pop_back` scans to the second-to-last node.
- `at`, `insert`, `erase`, `remove`, `remove_if`, and `find_if` traverse from `head`.
- Removal-by-value uses pointer equality, not payload comparison.

### 7.2 Queue Behavior

- Enqueue rejects invalid queue handles, null message pointers, and null handlers.
- Enqueue creates a queue-node wrapper and stamps arrival time.
- Dequeue removes the oldest node from the message list unless the queue has been re-sorted.
- Priority and timestamp metadata live only in internal queue nodes.
- Observer callbacks are invoked after the enqueue lock is released.
- Observer registration deduplicates by `(name, handler)`.

## 8. Sorting Design

Queue sorting is implemented in three stages:

1. Read queue-node pointers from the message list into a temporary array.
2. Sort the array with quicksort using the selected comparison strategy.
3. Clear the backing list and rebuild it in the sorted order.

Design consequences:

- FIFO strategy is a no-op comparison and preserves current order only because rebuild uses the existing pointer sequence.
- Priority and timestamp strategies sort in ascending order.
- Equal-key ordering is not guaranteed to be stable.
- Sort cost includes temporary memory allocation and a full list rebuild.

## 9. Observer Design

Observers are intended as enqueue-side notifications. They receive:

- queue pointer
- observer user data
- message pointer
- context pointer
- queued handler

The current design keeps observer handling simple:

- Registration only, no removal API
- Duplicate suppression by name and handler
- Fixed internal observer capacity of 256
- Synchronous invocation in the producer thread after enqueue

Because notification runs synchronously, a slow observer directly adds latency to enqueue.

## 10. Error and Return-Value Design

The module uses two styles:

- List mutation APIs mostly return `ZOO_ERROR_T`
- Queue APIs mostly return `ZOO_BOOL`

This reflects historical evolution rather than a single uniform abstraction. Callers integrating both layers should normalize these return styles in higher-level wrappers if they need a uniform error model.

## 11. Key Design Tradeoffs

### 11.1 Chosen Tradeoffs

- Opaque handles over exposed structs to preserve implementation freedom
- Internal mutexes over caller-managed synchronization for safer integration
- Pointer storage over payload copying for lower overhead and broader payload compatibility
- List reuse inside queue over custom queue-specific containers for faster implementation and smaller API surface

### 11.2 Resulting Costs

- Pointer lifetime remains a caller burden
- Nested locking increases contention and complexity
- `pop_back` and many search operations are linear
- Queue observers can delay producer throughput
- Queue sort rebuilds the list rather than sorting in place

## 12. Known Design Constraints

1. `zoo_list` includes a condition variable that is currently not used by list logic.
2. `zoo_queue` supports shutdown of blocking consumers through a sticky `exit_flag`; after exit is requested, blocking dequeue stays disabled for that queue instance.
3. Zero-capacity queue creation is allowed by the current implementation but behaves as a permanently full queue for enqueue.
4. There is no observer removal API.
5. There is no timed dequeue API.
6. There is no deep-copy mode for payload ownership.

## 13. Recommended Usage Guidance

1. Use the list when the stored element lifetime is managed elsewhere and only pointer ordering is needed.
2. Use the queue when producers and consumers are decoupled and callback dispatch is performed by the consumer side.
3. Avoid queue observers for heavy processing; use them only for light notifications or instrumentation.
4. Avoid sorting on every enqueue in high-throughput paths; batch updates when possible.