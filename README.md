# LSAL: Hardware Acceleration & Software Optimization of Local Sequence Alignment

[![Language: C++](https://img.shields.io/badge/Language-C%2B%2B11-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B)
[![Platform: Xilinx FPGA](https://img.shields.io/badge/Platform-Xilinx%20Vitis%20%2F%20FPGA-red.svg)](https://www.xilinx.com/products/design-tools/vitis.html)
[![Arch: ARM & x86](https://img.shields.io/badge/Architecture-ARM%20Cortex--A9%20%7C%20x86__64-brightgreen.svg)](#benchmarks)
[![Framework: OpenCL / XRT](https://img.shields.io/badge/Framework-OpenCL%20%7C%20XRT-orange.svg)](https://github.com/Xilinx/XRT)
[![Parallelism: OpenMP](https://img.shields.io/badge/Parallelism-OpenMP-green.svg)](https://www.openmp.org/)

An end-to-end high-performance implementation and hardware acceleration of the **Smith-Waterman Local Sequence Alignment (LSAL)** dynamic programming algorithm. 

This project explores the full optimization spectrum: from cache-aware algorithm restructuring and anti-diagonal multi-threading on **x86_64** and **embedded ARM**, to dedicated custom hardware synthesis on **Xilinx FPGAs** using **Vitis HLS** and the **OpenCL / XRT** host runtime.

---

## Project Overview

Local sequence alignment is a foundational algorithm in computational genomics and bioinformatics (O(NM) complexity). It computes the optimal local alignment between a query sequence (N) and a reference database string (M). 

Because each matrix cell depends on its immediate neighbors (**North**, **West**, and **North-West**), naive implementations suffer from severe memory bottlenecks and pipeline stalls. This project tackles these challenges across two domains:

1. **Software Optimization (x86 & ARM Cortex-A9):** Iterative algorithmic profiling, memory access reduction, register caching, and anti-diagonal wavefront parallelization via OpenMP.
2. **Hardware Acceleration (Xilinx FPGA / Vitis HLS):** Custom streaming accelerator featuring dual-ported BRAM partitioning, sliding-window buffers, dedicated AXI master interfaces, and $II=1$ loop pipelining.

---

## Architectural Implementations

### 1. Software Optimization Hierarchy (`sw_x86/` & `sw_arm/`)

| Stage | Optimization Technique | Core Engineering Benefit |
| :--- | :--- | :--- |
| **`lsal_orig`** | Baseline implementation | Naive nested loops with 2D array division (`/`) & modulo (`%`) indexing per cell. |
| **`lsal_opt1`** | **Index Arithmetic Elimination** | Removed expensive integer division/modulo inside inner loops via incremental 1D/2D pointer indexing. |
| **`lsal_opt2`** | **Sliding Window / Register Caching** | Cached `north`, `west`, and `northwest` dependency values in CPU registers, dramatically reducing redundant DRAM/L1 cache reads. |
| **`lsal_opt3`** | **Database Character Hoisting** | Hoisted database character loads (`database[i]`) outside the inner loop to maximize spatial and temporal cache locality. |
| **`lsal_opt4`** | **Wavefront (Anti-Diagonal) OpenMP** | Restructured iteration space along anti-diagonals ($k = i + j$) to eliminate loop-carried dependencies, enabling thread-level parallel processing. |

---

### 2. FPGA Acceleration Architecture (`hw_baseline/`)

Developed with **AMD Xilinx Vitis High-Level Synthesis (HLS)** and integrated through **XRT / OpenCL**:

* **AXI Memory Hierarchy:**
  * Configured independent **AXI4 Master (`m_axi`)** memory bundles (`gmem0`–`gmem4`) for concurrent DMA bursts of query strings, database buffers, scoring matrices, and direction matrices.
  * **AXI4-Lite (`s_axilite`)** control interface for host CPU runtime dispatch and register status tracking.

  * **On-Chip Memory & Array Partitioning:**
  * Local query buffers partitioned completely (`#pragma HLS ARRAY_PARTITION complete dim=1`) across on-chip registers.
  * Database memory window cyclically partitioned (`cyclic factor=32`) to sustain maximum parallel read bandwidth.
* **Pipelined Wavefront Execution:**
  * Utilized local 3-row history buffers (`row_curr`, `row_prev`, `row_old`) to track data dependencies on-chip.
  * Targeted an **Initiation Interval of 1 ($II = 1$)** with loop pipelining for continuous, deterministic clock-cycle throughput.
* **OpenCL Host Driver:**
  * Developed a C++ host runtime managing device enumeration, `.xclbin` bitstream loading, memory pinning/migration, kernel execution event profiling, and automated CPU-vs-HW golden output verification.

---

## Benchmark Results

> **Test Configuration:** Query Size $N = 32$, Database Size $M = 65,536$. Averaged across multiple benchmark passes (outliers trimmed).

### Embedded ARM (Cortex-A9, Cross-Compiled)
* **Baseline (`lsal`):** `7.7219 s`
* **Opt 1 (Index Elimination):** `4.3624 s` (**1.77x speedup**)
* **Opt 2 (Register Caching):** `3.8153 s` (**2.02x speedup**)
* **Opt 3 (Memory Hoisting):** `3.5846 s` (**2.15x speedup**)

### Desktop / Workstation (x86_64)
* **Baseline (`lsal`):** `0.7098 s`
* **Opt 1:** `0.4707 s` (**1.51x speedup**)
* **Opt 2:** `0.4309 s` (**1.65x speedup**)
* **Opt 3:** `0.4117 s` (**1.72x speedup**)
* **Opt 4 (Wavefront OpenMP):** Multi-threaded execution parallelizing cell calculation along the anti-diagonals.

---

## Repository Structure
```
.
├── hw_baseline/              # FPGA Vitis HLS & OpenCL Implementation
│   ├── lab_input/            # Kernel sources & host runtime
│   │   ├── lsal.cpp          # Vitis HLS kernel with pragmas
│   │   └── lsal_host.cpp     # OpenCL host code (XRT runtime)
│   ├── Makefile              # Vitis build script (sw_emu, hw_emu, hw)
│   └── run_app.sh            # Target execution & emulation launcher
│
├── sw_arm/                   # Embedded ARM Implementation
│   ├── lab_input/            # Optimization revisions (orig, opt1-opt4)
│   ├── benchmarks/           # Automated measurement logs & results CSVs
│   ├── Makefile              # Cross-compilation (arm-linux-gnueabihf-g++)
│   └── run_all.sh            # Benchmark orchestration & reporting script
│
├── sw_x86/                   # Native x86_64 Implementation
│   ├── lab_input/            # Host optimizations & OpenMP sources
│   ├── benchmarks/           # x86 benchmark results & timing logs
│   ├── Makefile              # Native compilation (GCC, AVX2, OpenMP)
│   └── run_all.sh            # Batch benchmark runner
└── README.md
```
---

## Build & Execution
### 1. Native x86 Build
```
cd sw_x86
make
# Run a specific benchmark (N=32, M=65536)
./target/lsal_opt3 32 65536

# Run full automated benchmark suite
./run_all.sh 32 65536
```
---

### 2. ARM Cross-Compilation
Requires the Linaro / Xilinx ARM toolchain (arm-linux-gnueabihf-g++):
```
cd sw_arm
make
# Deploy 'target/' binaries and 'run_all.sh' to target board
./run_all.sh 32 65536
```
---

### 3. FPGA Build via Xilinx Vitis
```
cd hw_baseline

# Software Emulation (sw_emu)
make all TARGET=sw_emu DEVICE=<FPGA_PLATFORM>

# Hardware Emulation (hw_emu)
make all TARGET=hw_emu DEVICE=<FPGA_PLATFORM>

# Full Bitstream Generation & Package (Hardware)
make all TARGET=hw DEVICE=<FPGA_PLATFORM> HOST_ARCH=aarch32 EDGE_COMMON_SW=<SYSROOT_PATH>
```
---

## Key Technical Takeaways

* **High-Level Synthesis (HLS):** Translating software algorithms into efficient RTL pipelines through pragma-guided unrolling, pipelining (II=1), and memory bank partitioning.
* **Heterogeneous Architecture:** Bridging Host CPU and FPGA accelerator domains using OpenCL, memory alignment, and non-blocking command queues.
* **Memory Hierarchy Awareness:** Minimizing off-chip memory transactions through on-chip register sliding windows and cache-conscious loops.
* **Parallel Algorithms:** Resolving fine-grained 2D loop dependencies into independent anti-diagonal wavefronts for multi-threaded parallel execution.
