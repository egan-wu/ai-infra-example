#include <systemc>
#include "ram.h"
#include "simplebus.h"
#include "tpu.h"

#ifdef USE_GEM5
#include "gem5_wrapper.h"
#else
#include "cpu.h"
#endif

int sc_main(int argc, char* argv[]) {
    RAM ram("ram");
    SimpleBus bus("bus");
    TPU tpu("tpu");

#ifdef USE_GEM5
    std::cout << "[Main] Mode: Gem5 Co-Simulation" << std::endl;
    // We assume the config file is at /app/gem5_config.py
    Gem5Wrapper cpu("gem5_wrapper", "/app/gem5_config.py");
#else
    std::cout << "[Main] Mode: Simple CPU Testbench" << std::endl;
    CPU cpu("cpu");
#endif

    // Connect CPU/Gem5 -> Bus
    cpu.i_socket.bind(bus.t_socket_cpu);

    // Connect Bus -> RAM
    bus.i_socket_ram.bind(ram.t_socket);

    // Connect Bus -> TPU (Config)
    bus.i_socket_tpu.bind(tpu.t_socket);

    // Connect TPU (DMA) -> Bus
    tpu.i_socket.bind(bus.t_socket_tpu);

    // Connect Interrupt
    sc_signal<bool> irq_sig;
    tpu.irq_out.bind(irq_sig);
    cpu.irq_in.bind(irq_sig);

    sc_start();

    return 0;
}
