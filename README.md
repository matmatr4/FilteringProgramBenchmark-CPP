# FilteringProgramBenchmark-CPP

A small C++ benchmark that simulates filtering detector events. It is a scaled-down version of the kind of event filtering PrecisionBeam works on. It accompanies the full system design report for the PrecisionBeam facility, which is included in this repository (see [Repository structure](#repository-structure)).

The program:

1. **Generates** 30,000 synthetic events. Each event holds 20,000 12-bit readings (40 kB per event), plus a checksum.
   - 15% are *signal* events with 12,000 strong readings (≥ 1000).
   - 85% are *background* events with only 2,000 strong readings.
   - 1% of events (background only) get a corrupted checksum.
2. **Filters** every event. It recomputes the checksum and discards the event on a mismatch, strips out readings below the noise threshold, and keeps the event only if at least 10,000 strong readings remain.
3. **Benchmarks** six ways of doing the filtering:
   - **Experiment 0:** a plain single-threaded loop on one core.
   - **Experiment 1:** the events split evenly across `NUM_THREADS` threads (12 by default), with no shared state.
   - **Experiment 2 (mutex, per event):** the same split, but each thread moves every kept event into one shared output structure, protected by a `std::mutex`. This imitates a shared output space in a real pipeline. Each kept event takes the lock once.
   - **Experiment 3 (mutex, buffered):** like Experiment 2, but each thread first collects kept events in its own private output buffer of `BUFFER_SIZE` events (64 by default). It takes the lock only to flush a full buffer, plus once at the end for the leftovers, so it locks about 64 times less often.
   - **Experiment 4 (atomic, per event):** no mutex. The shared output is sized to `NUM_EVENTS` up front, and a `std::atomic<std::size_t>` counter hands out the next free slot. Each thread claims a slot with `fetch_add(1)` and moves the kept event into it, so no two threads ever write to the same slot. After the run, the output is shrunk to the number of slots used.
   - **Experiment 5 (atomic, buffered):** like Experiment 4, but kept events first go into a private 64-event buffer, the same as in Experiment 3. On each flush, the thread claims a whole block of slots with a single `fetch_add(buffer.size())` and moves its events into them. This makes one atomic operation per 64 events instead of one per event.
4. **Reports** the kept-event count, the time and throughput (MB/s) for each experiment, the speedup over the baseline, and an Amdahl's Law estimate of the serial and parallel fractions, including a predicted speedup on 28 cores. For Experiments 2–5 it also prints how many events ended up in the shared output and the gain from adding the output buffer (2 vs 3 and 4 vs 5). It then compares mutex against atomic (2 vs 4 and 3 vs 5) and names the fastest of Experiments 2–5.

**Purpose of the experiments.** Experiments 0 and 1 measure how the filtering itself scales across cores. Experiments 2–5 add communication through memory, with every thread writing its kept events into one shared output, to imitate shared storage in a real pipeline. Each is run with and without a private output buffer, to show whether a buffer speeds up writing to that shared output, using either a mutex or an atomic counter to coordinate the threads.

Kept events are moved with `std::move`, not copied. Only the vector's internal pointer moves into the shared output, so the 40 kB of readings never get copied.

## Repository structure

The repository has two parts: the full system design report I wrote for PrecisionBeam (a small fictional particle collider facility), and the source code of this benchmark, which supports the report.

```
FilteringProgramBenchmark-CPP/
├── README.md
├── Makefile                          build configuration for the benchmark
├── Recommended System Report.pdf     the full system design report
├── Recommended System Report.docx    the same report in Word format
└── src/                              the benchmark source code
    ├── filtering-program.h
    ├── filtering-program.cpp
    ├── filter.cpp
    └── events.cpp
```

### The report

[Recommended System Report.pdf](Recommended%20System%20Report.pdf) (*Hardware System Design and Performance Report for PrecisionBeam*) is the full system design that this benchmark was built for. PrecisionBeam's detector upgrade overwhelmed its single-core 2008 server, and the report:

- diagnoses the current bottleneck and estimates the data rates of the upgraded detector
- specifies a replacement system (processor, memory, storage, motherboard, network, chassis, PSU and cooling) within the facility's budget, power and space limits, and reuses parts where they still fit
- covers system integration, error detection and maintenance, and an alternative architecture that was considered
- uses this benchmark to measure how event filtering scales across cores, with an Amdahl's Law analysis of the results

The benchmark's settings (15% signal events, the noise threshold and the 28-core prediction) come from the assumptions in the report. The program output and source code are also included in the report's appendix.

### Source code

The benchmark's source files, in `src/`:

| File | Purpose |
|------|---------|
| `filtering-program.h` | Shared constants (event count, thresholds, thread count, buffer size) and function declarations |
| `events.cpp` | Generates the synthetic events and checksums (fixed seed, so every run gets the same data) |
| `filter.cpp` | `filter_event`: the checksum check, threshold filtering and keep/discard decision |
| `filtering-program.cpp` | The per-thread filtering functions for each experiment, the shared outputs (mutex-protected for 2 and 3, atomic-indexed for 4 and 5), and `main`, which runs the generation step and all six experiments and then prints the results |

The `Makefile` in the repository root builds every `.cpp` file in `src/`.

## Requirements

- A C++17 compiler (`g++` or `clang++`)
- `make`
- About **8 GB of free RAM**. The event data is about 1.2 GB, and the program keeps six copies (one per experiment) so each experiment filters fresh data.

## Build and run

Run these from the repository root:

```bash
# build (output goes to build/filtering-program)
make

# build and run
make run

# or run the binary directly
./build/filtering-program

# remove build artifacts
make clean
```

To use a different compiler or different flags:

```bash
make CXX=clang++
make CXXFLAGS="-std=c++17 -O3 -march=native"
```

## Configuration

All settings live as `constexpr` constants in [filtering-program.h](src/filtering-program.h). Edit them and rebuild:

| Constant | Default | Meaning |
|----------|---------|---------|
| `NUM_THREADS` | 12 | Threads used in Experiments 1–5. Set this to your CPU's core count. |
| `BUFFER_SIZE` | 64 | Events held in each thread's private output buffer in Experiments 3 and 5. 64 kept events × 12,000 strong readings × 2 bytes ≈ 1.5 MB of readings, roughly the buffer size the report assumes. |
| `NUM_EVENTS` | 30000 | Number of events generated |
| `EVENT_READINGS` | 20000 | Readings per event |
| `THRESHOLD` | 1000 | Readings below this value count as noise |
| `MIN_STRONG` | 10000 | Minimum strong readings needed to keep an event |

## Output

For each experiment, the program prints the number of kept events (and, for Experiments 2–5, how many reached the shared output), the time and the throughput in MB/s. After each pair of experiments, it prints the speedup over Experiment 0, the Amdahl serial and parallel fractions, the predicted speedup on 28 cores and the gain from the output buffer. It ends with the mutex-vs-atomic comparisons (2 vs 4, 3 vs 5) and the fastest of Experiments 2–5. Every experiment should keep 4,500 of 30,000 events (15%).

## Results

Single run on a Mac with 12 threads. All experiments kept 4,500 events, and all 4,500 reached the shared output in Experiments 2–5.

| Experiment | Time (s) | MB/s | Speedup | Serial fraction | Predicted on 28 cores |
|------------|---------:|-----:|--------:|----------------:|----------------------:|
| 0: single thread | 0.5174 | 2,319 | 1.00x | n/a | n/a |
| 1: 12 threads | 0.0565 | 21,232 | 9.15x | 2.83% | 15.88x |
| 2: mutex, per event | 0.0591 | 20,290 | 8.75x | 3.38% | 14.64x |
| 3: mutex, buffered | 0.0550 | 21,809 | 9.40x | 2.51% | 16.69x |
| 4: atomic, per event | 0.0589 | 20,358 | 8.78x | 3.34% | 14.73x |
| 5: atomic, buffered | 0.0589 | 20,365 | 8.78x | 3.33% | 14.74x |

Buffer gain: 1.075x for the mutex (2 / 3) and 1.000x for the atomic (4 / 5). Mutex vs atomic: 1.003x per event (2 / 4) and 0.934x buffered (3 / 5).

## Conclusion

**What we expected.**
- Threads should give a large speedup, because each event can be filtered on its own.
- Buffering (3 and 5) should beat writing per event (2 and 4). A mutex lock costs tens of nanoseconds, or around a microsecond when contended, while writing to a private buffer costs a few nanoseconds. Buffering takes the shared lock, or calls the shared `fetch_add`, 64 times less often.
- The atomic (4 and 5) should beat the mutex (2 and 3), because `fetch_add` is a single hardware instruction that never blocks a thread.

**What happened.** Threads gave about a 9x speedup. The buffered mutex (Experiment 3) was the fastest run overall, 7.5% faster than the per-event mutex (Experiment 2). The buffered atomic (Experiment 5) tied with the per-event atomic (Experiment 4), and the atomic and the mutex tied when writing per event. All the multithreaded runs, 1 to 5, landed within about 7% of each other. An earlier run gave a mutex buffer gain of only 1.006x.

**Why.** Writing to the shared output is too small a part of the run time for the buffer or the atomic to make a consistent difference. Only 15% of events are kept, so there are just 4,500 shared writes, and each one only moves a pointer instead of copying 40 kB of readings. Even at about 1 µs per contended write, that's at most about 4.5 ms against a run of about 55 ms, spread over 12 threads. Almost all the time goes into reading 1.2 GB of data through memory, and that is the same in every experiment. Differences of a few percent are within run-to-run noise. So the 7.5% buffer gain for the mutex points in the expected direction, but it needs repeated runs to confirm.

**When it would matter.** Buffering and atomics pay off when shared writes are expensive or frequent. That means copying the readings instead of moving a pointer, keeping far more events, or filtering more cheaply. In a real pipeline that copies data into its output buffer, the buffer should also fit in cache: about 1.5 MB at the default size, which fits in an L2 of 2 MB or more. This benchmark doesn't test that, because it never copies the readings.

## Planned extensions

- **Stress tests:** copy readings instead of moving them, keep more events and use more threads, so the cost of synchronisation becomes visible. Repeat each run several times to average out noise.
- **Semaphores:** `std::counting_semaphore` (C++20) to limit concurrent writers or model a fixed-size buffer.
- **Condition variables:** a producer-consumer setup, with a consumer thread draining the output buffer.
