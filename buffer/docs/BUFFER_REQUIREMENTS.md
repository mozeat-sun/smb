# ZOO Buffer Module Requirements

## 1. Purpose

This document defines the functional and non-functional requirements for the ZOO buffer module. The module provides two reusable buffering primitives:

- A bounded, thread-safe singly linked list
- A bounded, thread-safe message queue built on top of the list

The requirements are written against the current public API in [buffer/inc/zoo_list.h](/home/mozeat/zoo/buffer/inc/zoo_list.h) and [buffer/inc/zoo_queue.h](/home/mozeat/zoo/buffer/inc/zoo_queue.h), and the current implementation in [buffer/src/zoo_list.c](/home/mozeat/zoo/buffer/src/zoo_list.c) and [buffer/src/zoo_queue.c](/home/mozeat/zoo/buffer/src/zoo_queue.c).

## 2. Scope

The buffer module shall provide in-process buffering utilities for other ZOO modules that need:

- Ordered storage of opaque pointers
- Bounded capacity enforcement
- Multi-thread safe insertion and removal
- Blocking dequeue semantics for producer-consumer workflows
- Optional queue observer notifications
- Optional message prioritization and queue sorting

The module does not provide:

- Cross-process IPC
- Payload serialization
- Ownership transfer of pointed-to payload memory
- Lock-free algorithms
- Persistent storage or durable queues

## 3. Functional Requirements

### 3.1 Linked List Requirements

1. The module shall expose an opaque list handle type.
2. The list shall be created with a maximum element capacity greater than zero.
3. The list shall reject creation when the requested capacity is zero.
4. The list shall support insertion at the back of the list.
5. The list shall support insertion at the front of the list.
6. The list shall support removal from the back of the list.
7. The list shall support removal from the front of the list.
8. The list shall provide non-destructive access to the front element.
9. The list shall provide non-destructive access to the back element.
10. The list shall provide indexed access by zero-based position.
11. The list shall provide insert-at-index behavior.
12. The list shall provide erase-at-index behavior.
13. The list shall provide removal by pointer equality.
14. The list shall provide removal by predicate.
15. The list shall provide lookup by predicate.
16. The list shall provide size, empty, full, and capacity queries.
17. The list shall support clearing all nodes without destroying the list handle.
18. The list shall support destruction of the list handle and all internal nodes.
19. The list shall store caller-supplied pointers directly and shall not deep-copy payloads.
20. The list shall not free caller-owned payload pointers during element removal, clear, or destroy.

### 3.2 Message Queue Requirements

1. The module shall expose an opaque queue handle type.
2. The queue shall be created with a configured maximum queue size.
3. The queue shall support enqueue of a message pointer, context pointer, handler, and user data.
4. The queue shall reject enqueue requests when the queue handle is invalid.
5. The queue shall reject enqueue requests when the message pointer is null.
6. The queue shall reject enqueue requests when the handler is null.
7. The queue shall reject enqueue requests when the queue has reached its configured capacity.
8. The queue shall support non-blocking dequeue.
9. The queue shall support blocking dequeue until a message is available or an exit condition is signaled.
10. The queue shall return the stored message pointer, context pointer, handler, and optional user data on dequeue.
11. The queue shall allow a blocking waiter to be released by an explicit exit API.
12. The queue shall wake blocked waiters during queue destruction.
13. The queue shall support observer registration for queue change notifications.
14. The queue shall suppress duplicate observers with the same name and handler pair.
15. The queue shall support per-message priority metadata.
16. The queue shall support retrieval of stored priority metadata.
17. The queue shall support queue sorting by priority, timestamp, or FIFO strategy selector.
18. The queue shall store caller-supplied message pointers directly and shall not deep-copy payloads.
19. The queue shall not invoke handlers internally during dequeue; it shall return the handler to the caller for invocation.
20. The queue shall not free caller-owned message or context payloads during dequeue or destruction.

## 4. Concurrency Requirements

1. All public list mutation and query APIs shall be safe to call concurrently from multiple threads.
2. All public queue APIs shall be safe to call concurrently from multiple threads.
3. Queue blocking behavior shall use a condition variable coordinated with the queue message mutex.
4. Queue enqueue shall signal at least one waiting consumer after a successful push.
5. Queue exit and destroy paths shall broadcast to all waiting consumers.

## 5. Error Handling Requirements

1. The list APIs that return status shall use ZOO error codes.
2. The queue APIs shall use `ZOO_BOOL` success or failure signaling.
3. Invalid handles and null mandatory arguments shall be rejected.
4. Bounded containers shall reject push or enqueue operations when full.
5. Indexed erase shall reject out-of-range indices.
6. Dequeue shall fail cleanly when no message is available in non-blocking mode.
7. Dequeue shall fail cleanly when blocking is interrupted by queue exit or queue destruction.

## 6. Resource Requirements

1. The list implementation shall allocate its control block from the C heap.
2. The list implementation shall allocate one internal node per stored element from the C heap.
3. The queue implementation shall allocate its control block from the ZOO memory pool.
4. The queue implementation shall allocate one internal queue node per enqueued message from the ZOO memory pool.
5. The queue implementation shall allocate one internal observer record per registered observer from the ZOO memory pool.
6. The queue implementation shall depend on the list implementation for message and observer storage.

## 7. Platform and Integration Requirements

1. The module shall compile against the ZOO platform abstraction layer in [platform/inc/zoo.h](/home/mozeat/zoo/platform/inc/zoo.h).
2. The module shall use ZOO mutex, condition variable, atomic, time, and sleep abstractions instead of directly binding to a single OS API.
3. The queue implementation shall compile with ZOO memory pool and logging dependencies.
4. The module shall be buildable with the module-local CMake configuration in [buffer/CMakeLists.txt](/home/mozeat/zoo/buffer/CMakeLists.txt).

## 8. Performance Requirements

1. Push-front, push-back, pop-front, front, back, empty, full, size, and capacity operations should be constant time with respect to element count, excluding lock acquisition cost.
2. Pop-back, indexed access, indexed insert beyond the head, erase by index, remove by pointer, remove by predicate, and find by predicate may be linear in element count.
3. Queue enqueue and dequeue should be constant time, excluding lock acquisition cost and observer callback execution.
4. Queue sorting may be $O(n \log n)$ for ordering plus $O(n)$ for list rebuild.

## 9. Constraints and Known Behavioral Notes

1. Payload lifetime remains the caller's responsibility because the module stores raw pointers.
2. Queue observer capacity is currently bounded by an internal fixed-size observer list of 256 entries.
3. A queue created with `max_queue_size == 0` is currently constructible but cannot successfully accept messages.
4. The linked list contains an internal condition variable that is currently reserved and not used by list operations.
5. Queue destruction and queue exit currently include a short sleep window after broadcasting wakeups.
6. Queue sorting is not stable for equal keys because it uses quicksort over a temporary pointer array.

## 10. Verification Requirements

Verification of this module shall include:

1. API-level unit tests for all public list operations.
2. API-level unit tests for all public queue operations.
3. Capacity and boundary-condition tests.
4. Multi-thread producer-consumer tests for blocking dequeue behavior.
5. Observer registration and notification tests.
6. Priority set, get, and sorting tests.
7. Build verification through the buffer module build script in [buffer/build.sh](/home/mozeat/zoo/buffer/build.sh).
8. Test execution through the buffer test configuration in [buffer/tests/CMakeLists.txt](/home/mozeat/zoo/buffer/tests/CMakeLists.txt).

## 11. Acceptance Criteria

The module is acceptable for integration when:

1. The public APIs compile cleanly on supported target platforms through the ZOO abstraction layer.
2. The list honors capacity, ordering, and thread-safety requirements.
3. The queue honors capacity, blocking wakeup, observer, and sorting requirements.
4. No API path frees caller-owned payload pointers unexpectedly.
5. Destruction and exit paths release blocked consumers without deadlock.