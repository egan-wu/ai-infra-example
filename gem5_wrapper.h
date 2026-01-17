#ifndef GEM5_WRAPPER_H
#define GEM5_WRAPPER_H

#ifdef USE_GEM5

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <string>

// Gem5 Headers (Source Tree Structure)
// Gem5 v23.1 uses 'namespace gem5' but headers are usually in src/sim, src/base, etc.
#include <sim/sim_object.hh>

class Gem5Wrapper : public sc_core::sc_module {
public:
    tlm_utils::simple_initiator_socket<Gem5Wrapper> i_socket;
    sc_core::sc_in<bool> irq_in;

    SC_HAS_PROCESS(Gem5Wrapper);

    Gem5Wrapper(sc_core::sc_module_name name, const std::string& config_file);
    ~Gem5Wrapper();

    void run_gem5_loop();

private:
    std::string config_file;
};

#endif // USE_GEM5
#endif // GEM5_WRAPPER_H
