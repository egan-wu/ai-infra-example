# SystemC TPU Simulation (Dual Mode: Simple & Gem5)

This project simulates a simplified SoC with a CPU, Memory (RAM), and a TPU Accelerator using SystemC and TLM-2.0. It supports two modes of operation: a fast internal testbench and a full-system Gem5 co-simulation.

## Project Structure

*   `main.cpp`: Entry point switching between modes.
*   `ram.h/cpp`, `simplebus.h/cpp`, `tpu.h/cpp`: Core SystemC modules.
*   `cpu.h/cpp`: Simple testbench module.
*   `gem5_wrapper.h/cpp`: Wrapper to integrate Gem5.
*   `gem5_config.py`: Gem5 Python configuration script.
*   `test_app.c`: Guest application running inside Gem5.
*   `Dockerfile`: Builds the environment (SystemC + Gem5).
*   `Makefile`: Build script for both targets.

## Instructions

### 1. Build the Docker Image

This step downloads SystemC, Gem5, and compiles everything. It may take 45+ minutes due to Gem5 compilation.

```bash
docker build -t systemc-tpu .
```

### 2. Run Mode A: Simple CPU (Fast)

Uses the internal SystemC testbench.

```bash
docker run --rm systemc-tpu make run_simple
```

### 3. Run Mode B: Gem5 CPU (Performance/Realism)

Uses the Gem5 simulator (ARM) to run `test_app.c` which drives the TPU.

```bash
docker run --rm systemc-tpu make run_gem5
```
