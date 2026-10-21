# FilteringProgramBenchmark-CPP

A small C++ benchmark that simulates filtering detector events. It is a scaled-down version of the kind of event filtering PrecisionBeam works on. It accompanies the full system design report for the PrecisionBeam facility, which is included in this repository (see [Repository structure](#repository-structure)).

The program:

1. **Generates** 30,000 synthetic events. Each event holds 20,000 12-bit readings (40 kB per event), plus a checksum.
   - 15% are *signal* events with 12,000 strong readings (≥ 1000).
   - 85% are *background* events with only 2,000 strong readings.
   - 1% of events (background only) get a corrupted checksum.
2. **Filters** every event. It recomputes the checksum and discards the event on a mismatch, strips out readings below the noise threshold, and keeps the event only if at least 10,000 strong readings remain.
3. **Benchmarks** two ways of doing the filtering:
   - **Experiment 0:** a plain single-threaded loop on one core.
   - **Experiment 1:** the events split evenly across `NUM_THREADS` threads (12 by default).
4. **Reports** the kept-event count, the time and throughput (MB/s) for each experiment, the speedup, and an Amdahl's Law estimate of the serial and parallel fractions, including a predicted speedup on 28 cores.

## Repository structure

The repository has two parts: the full system design report I wrote for PrecisionBeam (a small fictional particle collider facility), and the source code of this benchmark, which supports the report.

```
FilteringProgramBenchmark-CPP/
├── README.md
├── Recommended System Report.pdf     the full system design report
├── Recommended System Report.docx    the same report in Word format
└── Source Code/                      the benchmark program
    ├── Makefile
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

### Source Code

Everything needed to build and run the benchmark:

| File | Purpose |
|------|---------|
| `filtering-program.h` | Shared constants (event count, thresholds, thread count) and function declarations |
| `events.cpp` | Generates the synthetic events and checksums (fixed seed, so every run gets the same data) |
| `filter.cpp` | `filter_event`: the checksum check, threshold filtering and keep/discard decision |
| `filtering-program.cpp` | `main`: runs the generation step and both experiments, then prints the results |
| `Makefile` | Build configuration |

## Requirements

- A C++17 compiler (`g++` or `clang++`)
- `make`
- About **2.5 GB of free RAM**. The event data is about 1.2 GB, and the program keeps a second copy so each experiment filters fresh data.

## Build and run

Run these from inside the `Source Code` directory:

```bash
cd "Source Code"

# build (output goes to Source Code/build/filtering-program)
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

All settings live as `constexpr` constants in [filtering-program.h](Source%20Code/filtering-program.h). Edit them and rebuild:

| Constant | Default | Meaning |
|----------|---------|---------|
| `NUM_THREADS` | 12 | Threads used in Experiment 1. Set this to your CPU's core count. |
| `NUM_EVENTS` | 30000 | Number of events generated |
| `EVENT_READINGS` | 20000 | Readings per event |
| `THRESHOLD` | 1000 | Readings below this value count as noise |
| `MIN_STRONG` | 10000 | Minimum strong readings needed to keep an event |

## Example output

```
Generating 30000 events...
Generation took X s

Experiment 0: baseline (one core, no threads)
Kept 4500 of 30000 events (15%)
Time: X s (X MB/s)
Experiment 1: scaling with 12 threads
Kept 4500 of 30000 events (15%)
Time: X s (X MB/s)
Speedup: Xx
Serial fraction (Amdahl): X%
Parallel proportion (Amdahl): X%
Predicted speedup on 28 cores (Amdahl): Xx
```

Timings depend on your machine. Both experiments should always keep 4,500 events (15%).

## Planned extensions

Right now each thread filters its own share of events with no shared state. Next, I plan to add experiments that use synchronization primitives to imitate a real data pipeline, where filtered events are written to a shared output buffer:

- **Mutexes:** threads lock a `std::mutex` to push kept events into a shared output buffer, to measure the cost of lock contention.
- **Atomic variables:** `std::atomic` counters (for example, kept-event counts or a write index into the buffer) as a lock-free alternative to the mutex.
- **Semaphores (maybe):** `std::counting_semaphore` (C++20) to limit how many threads can write to the buffer at once, or to model a fixed-size buffer.
- **Condition variables (maybe):** a producer-consumer setup where filter threads produce kept events and a separate consumer thread drains the output buffer, with `std::condition_variable` used to signal when data is ready or space is free.

Each of these would get its own experiment, so its throughput and speedup can be compared against the current lock-free baseline.
