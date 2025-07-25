## Dynamic Memory Allocator

## Goal: Further my underingstanding of low-level memory management, demonstrating a practical understanding of how programs request and release memory from the operating system.

## Key Features & Implementation Strategies

My implementation focuses on balancing performance with strict adherence to memory safety and correctness.

* **Explicit Free List:** Instead of an implicit (block-by-block traversal) approach, this allocator utilizes an **explicit doubly linked list** to manage all free blocks. This significantly speeds up the search for available memory, especially in large heaps with many free blocks, as only free blocks need to be traversed.
* **Block Splitting:** When `mm_malloc` finds a free block larger than the requested size, it intelligently **splits** the block. The portion used for the allocation is marked as occupied, and the remaining space is converted into a new, smaller free block, which is then added back to the free list. This strategy minimizes internal fragmentation by preventing the allocation of excessively large blocks for small requests.
* **Coalescing:** To combat external fragmentation (where memory becomes fragmented into small, unusable free blocks), `mm_free` implements **immediate coalescing**. When a block is freed, the allocator checks its adjacent neighbors. If an adjacent block is also free, they are **merged** into a single, larger free block. This maximizes the size of available free blocks, making it easier to satisfy larger future allocation requests.
* **Heap Management Interface:** The allocator interacts with a simulated memory system (`memlib.c`) which provides functions like `mem_sbrk()` to expand the heap when no suitable free blocks are found.
* **Alignment:** All allocated blocks are guaranteed to be aligned to `ALIGNMENT` bytes (equivalent to `sizeof(FreeBlockInfo_t)`), ensuring compatibility with typical system memory access requirements for various data types.

## Project Results & Performance

* **Correctness Score: 100/100**
    * Correctly handled all allocation and deallocation requests across all test traces without any memory errors such as:
        * Overlapping payloads
        * Accessing memory outside of the heap boundaries
        * Attempting to free unallocated blocks
        * Heap consistency errors
* **Performance Index: 73/100**
    * **Utilization (47%):** This score reflects the average percentage of the heap actually used for client data (payloads) versus total heap size. My implementation achieves good utilization by employing block splitting and coalescing to reduce wasted space.
    * **Throughput (26%):** This measures the speed of the allocator in terms of operations per second, compared to a baseline (typically `libc`'s `malloc`). This indicates efficient execution of `malloc` and `free` operations.


## Building and Running

To build and test this dynamic memory allocator:

1.  **Navigate to the project root directory:**
    ```bash
    cd /path/to/file
    ```
2.  **Compile the project using `make`:**
    ```bash
    make
    ```
    This command will compile `mm.c`, `memlib.c`, `mdriver.c`, and other necessary helper files into a single executable named `mdriver`.
3.  **Run the test driver:**
    ```bash
    ./mdriver
    ```
    This will execute your `mm.c` against a series of memory traces and output its correctness, utilization, and throughput scores.

## Files in this Repository

* `mm.c`: Your custom dynamic memory allocator implementation.
* `mm.h`: Header file for `mm.c`, containing declarations for `mm_malloc`, `mm_free`, and internal helper functions.
* `memlib.c`: A simulated memory system that provides a low-level interface for heap expansion (e.g., `mem_sbrk`).
* `memlib.h`: Header for `memlib.c`.
* `mdriver.c`: The trace-driven driver program used to test the correctness and performance of the allocator.
* `Makefile`: Defines the build process for the project.
* `config.h`, `fsecs.h`, `fcyc.h`, `ftimer.h`, `clock.h`, `fsecs.c`, `fcyc.c`, `ftimer.c`, `clock.c`: Support files for timing and configuration of the driver.
* `traces/`: Directory containing various trace files used by `mdriver` for testing.

---
