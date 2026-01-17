#ifndef SIMPLEBUS_H
#define SIMPLEBUS_H

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
#include "memory_map.h"

using namespace sc_core;
using namespace sc_dt;
using namespace std;

class SimpleBus : public sc_module {
public:
    tlm_utils::simple_target_socket<SimpleBus> t_socket_cpu;
    tlm_utils::simple_target_socket<SimpleBus> t_socket_tpu;
    tlm_utils::simple_initiator_socket<SimpleBus> i_socket_ram;
    tlm_utils::simple_initiator_socket<SimpleBus> i_socket_tpu;

    SimpleBus(sc_module_name name);
    virtual void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay);
};

#endif // SIMPLEBUS_H
