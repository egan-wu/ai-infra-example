# SystemC TPU Simulation

This project simulates a simplified SoC with a CPU, Memory (RAM), and a TPU Accelerator using SystemC and TLM-2.0.

## Project Structure

*   `main.cpp`, `ram.h/cpp`, `simplebus.h/cpp`, `tpu.h/cpp`, `cpu.h/cpp`: The source code split into modules.
*   `memory_map.h`: Shared constants for memory addressing and registers.
*   `Dockerfile`: Docker configuration to build the environment and the project.
*   `Makefile`: Build script.

## Instructions

### 1. Build the Docker Image

Run the following command in the terminal to build the Docker image. This will download SystemC, compile it, and build the simulation project.

```bash
docker build -t systemc-tpu .
```

### 2. Run the Simulation

Run the container to execute the simulation:

```bash
docker run --rm systemc-tpu
```

You should see output similar to:

```
[CPU] Starting Testbench...
[CPU] Initializing RAM...
[CPU] Data for Matrix A:
   1.00    2.00 ...
[CPU] Data for Matrix B:
   ...
[CPU] Configuring TPU...
[CPU] Starting TPU...
[CPU] Waiting for Interrupt...
[CPU] Interrupt Received!
[CPU] TPU reported DONE.
[CPU] Data for Matrix C (Result):
   ...
[CPU] Verifying Results...
TEST PASSED
```
