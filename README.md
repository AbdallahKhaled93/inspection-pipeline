# Real-time frame pipeline (C++20)

A small fixed-rate frame pipeline built to explore modern C++ concurrency patterns
in a domain adjacent to ADAS perception work.

A source produces annotated frames at a fixed rate on its own thread. A bounded,
thread-safe queue buffers them. A consumer thread drains the queue and processes
each frame. When the consumer cannot keep up, frames are dropped deliberately and
counted rather than being allowed to accumulate.

The frame source is synthetic and the "detection" is a placeholder — the point of
the project is the pipeline: timing, ownership, back-pressure and shutdown.

---

## Build and run

Requires CMake 3.20+ and a C++20 compiler. Catch2 is fetched automatically at
configure time.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/inspection_demo
ctest --test-dir build --output-on-failure
```

For latency measurement, build with optimisations — numbers taken from a `-O0`
build are not meaningful:

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
```

---

## Architecture

```
  SyntheticSource          FrameQueue              consumer thread
  ---------------          ----------              ---------------
  generates frames  push   bounded, capacity N     pop   processes
  at a fixed rate  ----->  drop-oldest on full   ----->  each frame
  (Producer thread)        mutex + condvar               (jthread)
```

| Type             | Owns                                              |
|------------------|---------------------------------------------------|
| `Frame`          | Pixel buffer, sequence id, capture timestamp      |
| `AnnotatedFrame` | A `Frame` plus optional ground-truth `Detection`  |
| `FrameQueue`     | Bounded deque, mutex, condition variable, counters|
| `Producer`       | A `SyntheticSource` and the thread driving it     |

Ownership is expressed through value semantics and RAII throughout: no raw `new`,
no manual `delete`, no explicit `join()`. `Frame` owns its pixels and follows the
rule of zero. `Producer` starts its thread in the constructor and stops it in the
destructor. `FrameQueue` is non-copyable by construction (it holds a mutex), so it
is created once and passed by reference.

### Fixed-rate production

The producer sleeps against an **absolute deadline** that advances by exactly one
period each iteration:

```cpp
next += config_.period;
std::this_thread::sleep_until(next);
```

The naive alternative, `sleep_for(period)` after the work, drifts: the true period
becomes `period + work_time`, and the error accumulates without bound. At 30 fps a
mean overhead of 50 µs puts the loop 5.4 seconds behind after an hour.

If production ever overruns its period, `next` falls into the past and
`sleep_until` returns immediately. Rather than spinning to catch up, the loop skips
the missed slots and resets the deadline — a camera does not fire twice quickly
because the software was late.

---

## Back-pressure

This was the main design decision, and the one worth explaining.

### The constraint

Back-pressure policy is not a property of the queue. It is determined by whether
the producer can be slowed down.

In this system the producer stands in for a camera, and a camera cannot be told to
wait. If the consumer falls behind, the world in front of the sensor continues at
30 frames per second regardless. **The production rate is an input, not an output.**

Given a fixed input rate and a consumer that cannot keep up, something has to give.

### Options considered

**Block the producer.** Correct when the producer is a file reader or a replay
harness. Wrong here. In a real system the producer is a driver callback fed by a
frame grabber; blocking inside it stalls acquisition and the driver's internal
buffers overrun. Frames are still lost — just somewhere they cannot be counted.
The real choice is not *drop or don't drop*, it is **drop where I can count it, or
drop in the driver where I cannot.**

**Unbounded queue.** Fails late and catastrophically instead of early and visibly.
A transient slowdown becomes permanent: once behind, the consumer never catches up,
so a throughput problem is converted into an unbounded latency problem plus an
eventual out-of-memory. Memory is spent storing frames that will be worthless by
the time they are read.

**Bounded queue, drop oldest.** Chosen.

### Why drop the *oldest*

Frames are tied to a physical scene with a fixed time budget for reacting to it. A
frame that has been sitting in the queue describes where an object *was*, not where
it is.

- **Drop-newest** keeps the consumer working through a stale backlog. The queue
  stays permanently full and latency is pinned at maximum.
- **Drop-oldest** is self-correcting. Under overload the queue drains toward
  freshness and latency stays bounded by `capacity × period`.

The system optimises for **bounded latency, not maximum throughput**.

### Why the queue is small

Capacity is 4, not 100. The queue absorbs *jitter*; it does not store work. A queue
deep enough to hold a hundred frames is not solving a buffering problem, it is
hiding a capacity problem — trading a visible drop for invisible latency.

### Drops are counted

A dropped frame is a moment the system was blind. That is a fact about system
health, not a performance footnote, so `FrameQueue::dropped_count()` reports it and
the demo prints it. In a production system it would be published alongside the
other metrics and alarmed above a threshold.

---

## Measurements

Two runs, 2 seconds each, queue capacity 4, consumer simulating 10 ms of detection
work per frame.

| Producer period | Produced | Processed | Dropped | Steady-state queue depth |
|-----------------|----------|-----------|---------|--------------------------|
| 33 ms (30 fps)  | ~61      | 61        | 0       | ~0 (consumer idle)       |
| 1 ms (1000 fps) | ~1961    | 169       | 1792    | 4 (full)                 |

Two things to note.

**Nothing is lost silently.** In the overload run, 169 + 1792 = 1961, which matches
the number of frames produced. Every frame is accounted for as either processed or
deliberately discarded.

**Memory and latency stay flat under overload.** The queue held 4 frames throughout,
so the consumer never worked on a frame older than roughly 4 ms. Under the same
load an unbounded queue would have accumulated 1792 frames — several megabytes of
pixel data — and by the end would have been analysing frames nearly two seconds
stale. Same throughput, useless output.

---

## Testing

Seven Catch2 cases covering the queue, which is where the non-obvious behaviour
lives:

- Frames are returned in order when under capacity
- **The oldest frame is dropped, not the newest** — six frames pushed into a queue
  of four must leave ids `{2,3,4,5}`. This pins the back-pressure policy as an
  executable assertion; changing it to drop-newest fails this test by name.
- `dropped_count()` matches the number of discarded frames
- The queue never grows beyond its capacity
- **`pop()` returns `nullopt` when a stop is requested** — a consumer blocked on an
  empty queue must wake on shutdown or the join deadlocks. The test asserts the
  consumer is *still blocked* before requesting the stop, so a `pop()` that wrongly
  returned early would not pass.
- A frame pushed while `pop()` is blocked is delivered
- Under concurrent load, `processed + dropped == produced`

```bash
ctest --test-dir build --output-on-failure
```

---

## Notable C++20 usage

| Feature | Where | Why |
|---------|-------|-----|
| `std::span` | `Frame::pixels()` | Replaces `(uint8_t*, size_t)`; const-correct via overload pair |
| `std::optional` | `pop()`, ground truth | Absence in the type; empty `pop()` signals shutdown |
| `std::jthread` | `Producer`, consumer | Destructor stops and joins — no forgotten `join()` |
| `std::stop_token` | Shutdown path | Composes with `condition_variable_any::wait` |
| `std::chrono` | Timestamps, period | `steady_clock` is monotonic; units are type-checked |
| Designated initialisers | `Config` structs | Named settings instead of positional arguments |
| Structured bindings | Consumer loop | Unpacking `AnnotatedFrame` |

The shutdown path is the piece worth highlighting. A consumer blocked in
`condition_variable::wait` cannot see a plain `bool` flag — it is asleep and never
reaches the check. `condition_variable_any::wait` takes a `stop_token` directly, so
requesting a stop wakes the wait. This removes a classic source of shutdown
deadlocks.

---

## What this is not

- **Soft real-time on a general-purpose OS.** `sleep_until` guarantees waking no
  *earlier* than the deadline; scheduling granularity on macOS or stock Linux means
  overshoot of hundreds of microseconds. A production system would need a real-time
  kernel, thread priorities, or hardware-triggered acquisition.
- **No real detection.** Frames are synthetic — noise with an occasional bright
  rectangle — and the consumer does not analyse them. The analyser is the obvious
  next component and would slot in behind an interface without changing the
  pipeline.
- **Single producer, single consumer.** The queue is correct for multiple of each,
  but has only been exercised with one.
- **A mutex-based queue, deliberately.** A lock-free SPSC ring buffer would remove
  the ~20 ns lock cost, which is 0.00006% of a 33 ms frame budget. It would also
  require the consumer to spin-wait, burning a core, and would complicate clean
  shutdown. The mutex is not the bottleneck.
- **No metrics export.** Counters are printed at exit rather than published.

## Possible next steps

- An `Analyzer` interface so detection can be swapped without touching the pipeline
- Latency percentiles (p50/p95/p99) measured from `Frame::captured_at()` to result
- Graceful degradation: reduce work per frame under load rather than dropping —
  the response a production ADAS system would prefer
- Publishing metrics over a message bus rather than printing them