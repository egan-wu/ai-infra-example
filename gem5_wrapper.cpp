#ifdef USE_GEM5

#include "gem5_wrapper.h"
#include <iostream>
#include <vector>

// Gem5 Headers
#include <base/cprintf.hh>
#include <sim/system.hh>
#include <sim/process.hh>
#include <sim/init.hh> // For gem5::main
#include <sim/simulate.hh> // For gem5::simulate
// SystemC Integration Headers (these are often in systemc/ subdir of gem5 source)
#include <systemc/sc_master_port.hh>
#include <systemc/tlm_bridge/sc_ext.hh>

using namespace gem5;

// Handler to catch the port registration from Gem5
class TpuPortHandler : public gem5::sc_ext::ExternalPortHandler {
public:
    TpuPortHandler(Gem5Wrapper* parent) : parent_(parent) {}

    gem5::Port* getExternalPort(const std::string &name,
                                gem5::ExternalMaster &owner,
                                const std::string &port_data) override {
        if (port_data == "gem5_tpu_port") {
            // Bind the internal Gem5 port to the SystemC initiator socket
            auto* bridge = new gem5::ScMasterPort(name, owner, parent_->i_socket);
            return bridge;
        }
        return nullptr;
    }

private:
    Gem5Wrapper* parent_;
};

Gem5Wrapper::Gem5Wrapper(sc_core::sc_module_name name, const std::string& config_file)
    : sc_core::sc_module(name), i_socket("i_socket"), config_file(config_file)
{
    // Register the handler for the "gem5_tpu_port"
    gem5::sc_ext::registerExternalPortHandler("tlm_slave", new TpuPortHandler(this));

    // Initialize Gem5
    // We construct the arguments vector
    std::vector<char*> args;
    std::string prog = "gem5";
    args.push_back(const_cast<char*>(prog.c_str()));
    args.push_back(const_cast<char*>(config_file.c_str()));

    // Call gem5::main
    // In v23.1, this initializes the python system.
    gem5::main(args.size(), args.data());

    // Register our thread
    SC_THREAD(run_gem5_loop);
}

Gem5Wrapper::~Gem5Wrapper() {
}

void Gem5Wrapper::run_gem5_loop() {
    // Synchronize Gem5 with SystemC
    // We run Gem5 for a small slice, then wait in SystemC to let other modules run.
    // Gem5's GlobalEvent::simulate(ticks) runs for 'ticks' amount of time.

    // Note: m5.simulate() in python usually runs until exit.
    // The C++ equivalent is simulate(ticks).

    // We'll use a time quantum of 100 ns.
    gem5::Tick quantum = 100000; // 100 ns in Gem5 ticks (1 tick = 1 ps usually)
    // Actually, check Gem5 clock frequency. 1GHz = 1000 ticks per cycle?
    // Default is 1 tick = 1 ps. 1ns = 1000 ticks.

    while(true) {
        gem5::GlobalSimLoopExitEvent *event = gem5::simulate(quantum);

        if (event && event->getCause() != "simulate() limit reached") {
            // Simulation finished or error
            std::cout << "[Gem5Wrapper] Simulation exited: " << event->getCause() << std::endl;
            sc_stop();
            break;
        }

        // Yield to SystemC kernel
        wait(sc_core::sc_time(100, sc_core::SC_NS));
    }
}

#endif // USE_GEM5
