#ifndef CPU_H
#define CPU_H

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include "memory_map.h"

using namespace sc_core;
using namespace sc_dt;
using namespace std;

class CPU : public sc_module {
public:
    tlm_utils::simple_initiator_socket<CPU> i_socket;
    sc_in<bool> irq_in;

    SC_HAS_PROCESS(CPU); // Added as requested

    CPU(sc_module_name name);

    void bus_write(uint64_t addr, uint32_t data);
    uint32_t bus_read(uint64_t addr);
    void mem_write_float(uint64_t addr, float data);
    float mem_read_float(uint64_t addr);
    void print_matrix(const char* name, uint64_t base_addr, int n);
    void run_test();
};

#endif // CPU_H
