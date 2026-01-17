# SystemC TPU Simulation

This project simulates a simplified SoC with a CPU, Memory (RAM), and a TPU Accelerator using SystemC and TLM-2.0.

## Project Structure

*   `main.cpp`: The complete C++ source code.
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
[CPU] Configuring TPU...
[CPU] Starting TPU...
[CPU] Waiting for Interrupt...
[CPU] Interrupt Received!
[CPU] TPU reported DONE.
[CPU] Verifying Results...
TEST PASSED
```
