# FilteringProgramBenchmark-CPP

A small C++ benchmark that simulates filtering detector events. It is a scaled-down version of the kind of event filtering PrecisionBeam works on.

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

## Project structure

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

All settings live as `constexpr` constants in [filtering-program.h](filtering-program.h). Edit them and rebuild:

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
