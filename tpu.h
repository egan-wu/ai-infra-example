#ifndef TPU_H
#define TPU_H

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
#include <vector>
#include "memory_map.h"

using namespace sc_core;
using namespace sc_dt;
using namespace std;

class TPU : public sc_module {
public:
    tlm_utils::simple_target_socket<TPU> t_socket;
    tlm_utils::simple_initiator_socket<TPU> i_socket;
    sc_out<bool> irq_out;

    SC_HAS_PROCESS(TPU); // Added as requested

    TPU(sc_module_name name);

    virtual void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay);
    void process_thread();

private:
    uint32_t reg_addr_a;
    uint32_t reg_addr_b;
    uint32_t reg_addr_c;
    uint32_t reg_dim;
    uint32_t reg_status;

    sc_event start_event;
};

#endif // TPU_H
