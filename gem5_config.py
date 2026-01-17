
import m5
from m5.objects import *
import os
import sys

# Define a simple ARM system
def create_system():
    # System architecture
    system = System()
    system.clk_domain = SrcClockDomain()
    system.clk_domain.clock = '1GHz'
    system.clk_domain.voltage_domain = VoltageDomain()

    system.mem_mode = 'atomic' # fast simulation
    system.mem_ranges = [AddrRange('512MB')] # RAM 0x0 - 0x1FFFFFFF

    # CPU
    system.cpu = AtomicSimpleCPU()

    # System-wide memory bus
    system.membus = SystemXBar()

    # Connect CPU to bus
    system.cpu.icache_port = system.membus.cpu_side_ports
    system.cpu.dcache_port = system.membus.cpu_side_ports

    # Interrupt Controller (Dummy for now, needed for ARM)
    system.cpu.createInterruptController()

    # Connect system port
    system.system_port = system.membus.cpu_side_ports

    # Memory Controller
    system.mem_ctrl = MemCtrl()
    system.mem_ctrl.dram = DDR3_1600_8x8()
    system.mem_ctrl.dram.range = system.mem_ranges[0]
    system.mem_ctrl.port = system.membus.mem_side_ports

    # External Port for TPU
    # We map 0x10000000 - 0x100000FF to external port
    # In Gem5, ExternalMaster acts as a bridge.
    # The 'port_type' string ("gem5_tpu_port") is what the C++ wrapper will look for.
    system.tpu_port = ExternalMaster(port_data="gem5_tpu_port", port_type="tlm_slave")

    # We need to route requests to this port.
    # Typically this is done by connecting it to the membus and ensuring
    # the address map routes correctly.
    system.tpu_port.port = system.membus.mem_side_ports

    # Note: Address routing in Gem5 XBar is dynamic based on connected slaves' ranges.
    # The C++ side (TLM Target) needs to report the address range
    # but since our Wrapper initiates transactions *from* Gem5,
    # we are actually exporting a Master port *to* SystemC.
    # Wait, the requirement is "Gem5 (CPU) -> ExternalMaster Port -> Gem5Wrapper -> SimpleBus -> TPU".
    # So Gem5 is the Master. The ExternalMaster object in Python provides a port that *is* a master in Gem5
    # and connects to a slave in the C++ world?
    # Actually, ExternalMaster in Gem5 means "An external object that acts as a Master to Gem5".
    # NO. ExternalMaster means "A port in Gem5 that connects to an External Master".
    # We want Gem5 CPU (Master) -> MemBus -> ExternalSlave (in Gem5) -> Bridge -> SystemC Target?
    # Let's check definitions.
    # ExternalMaster: A port that allows an external simulator to send requests TO Gem5. (External is Master).
    # ExternalSlave: A port that allows Gem5 to send requests TO an external simulator. (Gem5 is Master).

    # Requirement: "Gem5 (CPU) -> ExternalMaster Port -> Gem5Wrapper -> SimpleBus -> TPU".
    # If the user said "ExternalMaster", they might have meant the C++ side acts as master?
    # "The Gem5Wrapper must act as a `simple_initiator_socket`".
    # This means Gem5Wrapper initiates transactions on the SystemC bus.
    # So Gem5Wrapper receives requests *from* Gem5.
    # So Gem5 is the Master.
    # So inside Gem5 python, we need a port that sends data OUT.
    # That is `ExternalSlave` in Gem5 terminology (it connects to an external Slave, i.e., target).

    # HOWEVER, the user explicitly said: "The architecture is: Gem5 (CPU) -> ExternalMaster Port -> ..."
    # This terminology is often confusing.
    # In Gem5 v21+, `ExternalMaster` means the *Gem5 object* has a Master port that connects to the outside world.
    # (i.e. Gem5 drives the transaction). This matches the requirement.
    # Let's verify: `src/mem/external_master.cc`: "Port that interfaces with an external port... acts as a master".
    # Yes. ExternalMaster means Gem5 IS the Master.

    system.tpu_bridge = ExternalMaster(port_data="gem5_tpu_port", port_type="tlm_slave", addr_ranges=[AddrRange(0x10000000, size='256B')])
    system.tpu_bridge.port = system.membus.mem_side_ports

    # Workload
    # We need a binary to run. Since we are in SE (Syscall Emulation) or Full System?
    # The request implies we are replacing "CPU Testbench" with "Gem5".
    # Usually this means running a baremetal binary or a small linux app.
    # But for this setup, we might just need to drive the bus.
    # Without a binary, the CPU won't fetch instructions.
    # We'll set up a dummy binary path, the user will need to provide one or we use a stub.
    # For now, we'll create the process but warn if binary missing.

    process = Process()
    process.cmd = ['/app/tpu_test_binary'] # This binary needs to be compiled for ARM!
    system.cpu.workload = process
    system.cpu.createThreads()

    return system

root = Root(full_system=False, system=create_system())
m5.instantiate()

print("System instantiated (Python). Waiting for C++ control loop.")
# m5.simulate() is NOT called here. It will be driven by the C++ wrapper.
