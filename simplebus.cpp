#include "simplebus.h"

SimpleBus::SimpleBus(sc_module_name name) : sc_module(name),
    t_socket_cpu("t_socket_cpu"), t_socket_tpu("t_socket_tpu"),
    i_socket_ram("i_socket_ram"), i_socket_tpu("i_socket_tpu")
{
    t_socket_cpu.register_b_transport(this, &SimpleBus::b_transport);
    t_socket_tpu.register_b_transport(this, &SimpleBus::b_transport);
}

void SimpleBus::b_transport(tlm::tlm_generic_payload& trans, sc_time& delay) {
    uint64_t addr = trans.get_address();

    if (addr >= RAM_BASE_ADDR && addr < RAM_BASE_ADDR + RAM_SIZE) {
        i_socket_ram->b_transport(trans, delay);
    } else if (addr >= TPU_BASE_ADDR && addr < TPU_BASE_ADDR + TPU_SIZE) {
        trans.set_address(addr - TPU_BASE_ADDR);
        i_socket_tpu->b_transport(trans, delay);
        trans.set_address(addr);
    } else {
        SC_REPORT_WARNING("BUS", "Address decode error");
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
    }
}
