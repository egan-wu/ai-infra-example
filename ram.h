#ifndef RAM_H
#define RAM_H

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include "memory_map.h"

using namespace sc_core;
using namespace sc_dt;
using namespace std;

class RAM : public sc_module {
public:
    tlm_utils::simple_target_socket<RAM> t_socket;

    RAM(sc_module_name name);
    ~RAM();

    virtual void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay);

private:
    uint8_t* memory;
    size_t mem_size;
};

#endif // RAM_H
