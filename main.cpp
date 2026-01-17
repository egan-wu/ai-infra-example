#include <systemc>
#include "ram.h"
#include "simplebus.h"
#include "tpu.h"
#include "cpu.h"

int sc_main(int argc, char* argv[]) {
    RAM ram("ram");
    SimpleBus bus("bus");
    TPU tpu("tpu");
    CPU cpu("cpu");

    // Connect CPU -> Bus
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
