#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
#include <iostream>
#include <vector>
#include <iomanip>
#include <cstring>
#include <cmath>

using namespace sc_core;
using namespace sc_dt;
using namespace std;

// Memory Map
const uint64_t RAM_BASE_ADDR = 0x00000000;
const uint64_t RAM_SIZE      = 0x10000000; // 256 MB
const uint64_t TPU_BASE_ADDR = 0x10000000;
const uint64_t TPU_SIZE      = 0x00000100; // 256 Bytes

// Register Offsets
const uint64_t REG_ADDR_SRC_A = 0x00;
const uint64_t REG_ADDR_SRC_B = 0x04;
const uint64_t REG_ADDR_DST_C = 0x08;
const uint64_t REG_MATRIX_DIM = 0x0C;
const uint64_t REG_CMD_START  = 0x10;
const uint64_t REG_STATUS     = 0x14;

// Status Codes
const uint32_t STATUS_IDLE = 0;
const uint32_t STATUS_BUSY = 1;
const uint32_t STATUS_DONE = 2;

// Matrix Config
const int TEST_DIM = 32;

// =============================================================================
// RAM Module
// =============================================================================
class RAM : public sc_module {
public:
    tlm_utils::simple_target_socket<RAM> t_socket;

    RAM(sc_module_name name) : sc_module(name), t_socket("t_socket") {
        t_socket.register_b_transport(this, &RAM::b_transport);
        // Allocate memory. For simulation efficiency, we'll allocate a flat array
        // but checking bounds. 256MB is large but manageable on modern systems.
        // Or we can just allocate enough for our test (e.g., 1MB) and map it to 0.
        mem_size = 1024 * 1024 * 4; // 4MB should be plenty for 32x32 matrices
        memory = new uint8_t[mem_size];
        memset(memory, 0, mem_size);
    }

    ~RAM() {
        delete[] memory;
    }

    virtual void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay) {
        tlm::tlm_command cmd = trans.get_command();
        uint64_t    addr = trans.get_address();
        unsigned char* ptr = trans.get_data_ptr();
        unsigned int   len = trans.get_data_length();
        unsigned char* byt = trans.get_byte_enable_ptr();
        unsigned int   wid = trans.get_streaming_width();

        if (addr + len > mem_size) {
            SC_REPORT_ERROR("RAM", "Address out of bounds");
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
            return;
        }
        if (byt != 0 || wid < len) {
            SC_REPORT_ERROR("RAM", "Byte enable or streaming not supported");
            trans.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
            return;
        }

        if (cmd == tlm::TLM_READ_COMMAND) {
            memcpy(ptr, &memory[addr], len);
        } else if (cmd == tlm::TLM_WRITE_COMMAND) {
            memcpy(&memory[addr], ptr, len);
        }

        trans.set_dmi_allowed(true);
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    }

private:
    uint8_t* memory;
    size_t mem_size;
};

// =============================================================================
// SimpleBus Module
// =============================================================================
class SimpleBus : public sc_module {
public:
    // Targets for initiators to connect to
    tlm_utils::simple_target_socket<SimpleBus> t_socket_cpu;
    tlm_utils::simple_target_socket<SimpleBus> t_socket_tpu; // For DMA

    // Initiators to connect to targets
    tlm_utils::simple_initiator_socket<SimpleBus> i_socket_ram;
    tlm_utils::simple_initiator_socket<SimpleBus> i_socket_tpu; // For Config

    SimpleBus(sc_module_name name) : sc_module(name),
        t_socket_cpu("t_socket_cpu"), t_socket_tpu("t_socket_tpu"),
        i_socket_ram("i_socket_ram"), i_socket_tpu("i_socket_tpu")
    {
        t_socket_cpu.register_b_transport(this, &SimpleBus::b_transport);
        t_socket_tpu.register_b_transport(this, &SimpleBus::b_transport);
    }

    virtual void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay) {
        uint64_t addr = trans.get_address();

        if (addr >= RAM_BASE_ADDR && addr < RAM_BASE_ADDR + RAM_SIZE) {
            // Route to RAM
            // Adjust address if needed (RAM is base 0 so no adjustment needed)
            i_socket_ram->b_transport(trans, delay);
        } else if (addr >= TPU_BASE_ADDR && addr < TPU_BASE_ADDR + TPU_SIZE) {
            // Route to TPU Registers
            trans.set_address(addr - TPU_BASE_ADDR); // Normalize to offset 0
            i_socket_tpu->b_transport(trans, delay);
            trans.set_address(addr); // Restore address
        } else {
            SC_REPORT_WARNING("BUS", "Address decode error");
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        }
    }
};

// =============================================================================
// TPU Accelerator
// =============================================================================
class TPU : public sc_module {
public:
    tlm_utils::simple_target_socket<TPU> t_socket;    // Config/Status
    tlm_utils::simple_initiator_socket<TPU> i_socket; // DMA to RAM
    sc_out<bool> irq_out;

    TPU(sc_module_name name) : sc_module(name), t_socket("t_socket"), i_socket("i_socket") {
        t_socket.register_b_transport(this, &TPU::b_transport);

        SC_THREAD(process_thread);

        // Initialize registers
        reg_addr_a = 0;
        reg_addr_b = 0;
        reg_addr_c = 0;
        reg_dim = 0;
        reg_status = STATUS_IDLE;

        irq_out.initialize(false);
    }

    virtual void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay) {
        tlm::tlm_command cmd = trans.get_command();
        uint64_t    addr = trans.get_address();
        unsigned char* ptr = trans.get_data_ptr();
        unsigned int   len = trans.get_data_length();

        if (len != 4) {
            SC_REPORT_ERROR("TPU", "Registers must be accessed as 4 bytes");
            trans.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
            return;
        }

        uint32_t data;
        memcpy(&data, ptr, 4);

        if (cmd == tlm::TLM_WRITE_COMMAND) {
            switch(addr) {
                case REG_ADDR_SRC_A: reg_addr_a = data; break;
                case REG_ADDR_SRC_B: reg_addr_b = data; break;
                case REG_ADDR_DST_C: reg_addr_c = data; break;
                case REG_MATRIX_DIM: reg_dim = data; break;
                case REG_CMD_START:
                    if (data == 1) start_event.notify(delay);
                    break;
                default:
                    SC_REPORT_WARNING("TPU", "Write to read-only or invalid register");
                    break;
            }
        } else if (cmd == tlm::TLM_READ_COMMAND) {
            switch(addr) {
                case REG_ADDR_SRC_A: data = reg_addr_a; break;
                case REG_ADDR_SRC_B: data = reg_addr_b; break;
                case REG_ADDR_DST_C: data = reg_addr_c; break;
                case REG_MATRIX_DIM: data = reg_dim; break;
                case REG_STATUS:     data = reg_status; break;
                default: data = 0; break;
            }
            memcpy(ptr, &data, 4);
        }

        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    }

    void process_thread() {
        while (true) {
            wait(start_event);

            reg_status = STATUS_BUSY;
            irq_out.write(false);

            uint32_t n = reg_dim;
            if (n == 0) {
                 reg_status = STATUS_DONE;
                 irq_out.write(true);
                 continue;
            }

            // Allocate local buffers
            std::vector<float> A(n * n);
            std::vector<float> B(n * n);
            std::vector<float> C(n * n, 0.0f);

            sc_time delay = SC_ZERO_TIME;
            tlm::tlm_generic_payload trans;

            // DMA Read A
            for (size_t i = 0; i < A.size(); i++) {
                uint32_t addr = reg_addr_a + i * sizeof(float);
                trans.set_command(tlm::TLM_READ_COMMAND);
                trans.set_address(addr);
                trans.set_data_ptr(reinterpret_cast<unsigned char*>(&A[i]));
                trans.set_data_length(sizeof(float));
                trans.set_streaming_width(sizeof(float));
                trans.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

                i_socket->b_transport(trans, delay);

                if (trans.get_response_status() != tlm::TLM_OK_RESPONSE) {
                     SC_REPORT_ERROR("TPU", "DMA Read Error A");
                }
            }

            // DMA Read B
            for (size_t i = 0; i < B.size(); i++) {
                uint32_t addr = reg_addr_b + i * sizeof(float);
                trans.set_command(tlm::TLM_READ_COMMAND);
                trans.set_address(addr);
                trans.set_data_ptr(reinterpret_cast<unsigned char*>(&B[i]));
                trans.set_data_length(sizeof(float));
                trans.set_streaming_width(sizeof(float));
                trans.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

                i_socket->b_transport(trans, delay);

                if (trans.get_response_status() != tlm::TLM_OK_RESPONSE) {
                     SC_REPORT_ERROR("TPU", "DMA Read Error B");
                }
            }

            // Compute C = A * B
            for (uint32_t i = 0; i < n; i++) {
                for (uint32_t j = 0; j < n; j++) {
                    float sum = 0.0f;
                    for (uint32_t k = 0; k < n; k++) {
                        sum += A[i*n + k] * B[k*n + j];
                    }
                    C[i*n + j] = sum;
                }
            }

            // DMA Write C
            for (size_t i = 0; i < C.size(); i++) {
                uint32_t addr = reg_addr_c + i * sizeof(float);
                trans.set_command(tlm::TLM_WRITE_COMMAND);
                trans.set_address(addr);
                trans.set_data_ptr(reinterpret_cast<unsigned char*>(&C[i]));
                trans.set_data_length(sizeof(float));
                trans.set_streaming_width(sizeof(float));
                trans.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

                i_socket->b_transport(trans, delay);

                if (trans.get_response_status() != tlm::TLM_OK_RESPONSE) {
                     SC_REPORT_ERROR("TPU", "DMA Write Error C");
                }
            }

            reg_status = STATUS_DONE;
            irq_out.write(true);
        }
    }

private:
    // Registers
    uint32_t reg_addr_a;
    uint32_t reg_addr_b;
    uint32_t reg_addr_c;
    uint32_t reg_dim;
    uint32_t reg_status;

    sc_event start_event;
};

// =============================================================================
// CPU (Testbench)
// =============================================================================
class CPU : public sc_module {
public:
    tlm_utils::simple_initiator_socket<CPU> i_socket;
    sc_in<bool> irq_in;

    CPU(sc_module_name name) : sc_module(name), i_socket("i_socket") {
        SC_THREAD(run_test);
        sensitive << irq_in;
    }

    void bus_write(uint64_t addr, uint32_t data) {
        tlm::tlm_generic_payload trans;
        sc_time delay = SC_ZERO_TIME;
        trans.set_command(tlm::TLM_WRITE_COMMAND);
        trans.set_address(addr);
        trans.set_data_ptr(reinterpret_cast<unsigned char*>(&data));
        trans.set_data_length(4);
        trans.set_streaming_width(4);
        i_socket->b_transport(trans, delay);
    }

    uint32_t bus_read(uint64_t addr) {
        tlm::tlm_generic_payload trans;
        sc_time delay = SC_ZERO_TIME;
        uint32_t data;
        trans.set_command(tlm::TLM_READ_COMMAND);
        trans.set_address(addr);
        trans.set_data_ptr(reinterpret_cast<unsigned char*>(&data));
        trans.set_data_length(4);
        trans.set_streaming_width(4);
        i_socket->b_transport(trans, delay);
        return data;
    }

    // Helper for float access
    void mem_write_float(uint64_t addr, float data) {
        tlm::tlm_generic_payload trans;
        sc_time delay = SC_ZERO_TIME;
        trans.set_command(tlm::TLM_WRITE_COMMAND);
        trans.set_address(addr);
        trans.set_data_ptr(reinterpret_cast<unsigned char*>(&data));
        trans.set_data_length(sizeof(float));
        trans.set_streaming_width(sizeof(float));
        i_socket->b_transport(trans, delay);
    }

    float mem_read_float(uint64_t addr) {
        tlm::tlm_generic_payload trans;
        sc_time delay = SC_ZERO_TIME;
        float data;
        trans.set_command(tlm::TLM_READ_COMMAND);
        trans.set_address(addr);
        trans.set_data_ptr(reinterpret_cast<unsigned char*>(&data));
        trans.set_data_length(sizeof(float));
        trans.set_streaming_width(sizeof(float));
        i_socket->b_transport(trans, delay);
        return data;
    }

    void print_matrix(const char* name, uint64_t base_addr, int n) {
        cout << "[CPU] Data for " << name << ":" << endl;
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                float val = mem_read_float(base_addr + (i * n + j) * sizeof(float));
                cout << setw(8) << fixed << setprecision(2) << val << " ";
            }
            cout << endl;
        }
    }

    void run_test() {
        cout << "[CPU] Starting Testbench..." << endl;

        int n = TEST_DIM;
        // Memory allocation in RAM
        // A: 0x1000, B: 0x2000, C: 0x3000 (Offsets)
        // Ensure enough space: 32*32*4 = 4096 bytes.
        // 0x1000 = 4096. Perfect.
        uint32_t addr_a = 0x00001000;
        uint32_t addr_b = 0x00003000;
        uint32_t addr_c = 0x00005000;

        // Initialize Data
        cout << "[CPU] Initializing RAM..." << endl;
        vector<float> ref_A(n*n);
        vector<float> ref_B(n*n);

        for (int i=0; i<n*n; ++i) {
            ref_A[i] = static_cast<float>(i % 9 + 1);
            ref_B[i] = static_cast<float>((i + 3) % 7 + 1);
            mem_write_float(addr_a + i*4, ref_A[i]);
            mem_write_float(addr_b + i*4, ref_B[i]);
        }

        // Print Input Matrices
        print_matrix("Matrix A", addr_a, n);
        print_matrix("Matrix B", addr_b, n);

        // Configure TPU
        cout << "[CPU] Configuring TPU..." << endl;
        bus_write(TPU_BASE_ADDR + REG_ADDR_SRC_A, addr_a);
        bus_write(TPU_BASE_ADDR + REG_ADDR_SRC_B, addr_b);
        bus_write(TPU_BASE_ADDR + REG_ADDR_DST_C, addr_c);
        bus_write(TPU_BASE_ADDR + REG_MATRIX_DIM, n);

        // Start TPU
        cout << "[CPU] Starting TPU..." << endl;
        bus_write(TPU_BASE_ADDR + REG_CMD_START, 1);

        // Wait for IRQ
        cout << "[CPU] Waiting for Interrupt..." << endl;
        wait(irq_in.posedge_event());
        cout << "[CPU] Interrupt Received!" << endl;

        // Verify Status
        uint32_t status = bus_read(TPU_BASE_ADDR + REG_STATUS);
        if (status == STATUS_DONE) {
            cout << "[CPU] TPU reported DONE." << endl;
        } else {
            cout << "[CPU] Unexpected TPU status: " << status << endl;
        }

        // Print Output Matrix
        print_matrix("Matrix C (Result)", addr_c, n);

        // Verify Result
        cout << "[CPU] Verifying Results..." << endl;
        bool pass = true;
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                float sum = 0.0f;
                for (int k = 0; k < n; k++) {
                    sum += ref_A[i*n + k] * ref_B[k*n + j];
                }

                float tpu_val = mem_read_float(addr_c + (i*n + j)*4);

                if (abs(sum - tpu_val) > 0.001) {
                    cout << "Mismatch at [" << i << "][" << j << "]: Ref=" << sum << " TPU=" << tpu_val << endl;
                    pass = false;
                    break; // Stop on first error
                }
            }
            if (!pass) break;
        }

        if (pass) {
            cout << "TEST PASSED" << endl;
        } else {
            cout << "TEST FAILED" << endl;
        }

        sc_stop();
    }
};

// =============================================================================
// Top Level
// =============================================================================
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
