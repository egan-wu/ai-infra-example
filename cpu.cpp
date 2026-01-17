#include "cpu.h"
#include <iostream>
#include <vector>
#include <iomanip>
#include <cmath>

CPU::CPU(sc_module_name name) : sc_module(name), i_socket("i_socket") {
    SC_THREAD(run_test);
    sensitive << irq_in;
}

void CPU::bus_write(uint64_t addr, uint32_t data) {
    tlm::tlm_generic_payload trans;
    sc_time delay = SC_ZERO_TIME;
    trans.set_command(tlm::TLM_WRITE_COMMAND);
    trans.set_address(addr);
    trans.set_data_ptr(reinterpret_cast<unsigned char*>(&data));
    trans.set_data_length(4);
    trans.set_streaming_width(4);
    i_socket->b_transport(trans, delay);
}

uint32_t CPU::bus_read(uint64_t addr) {
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

void CPU::mem_write_float(uint64_t addr, float data) {
    tlm::tlm_generic_payload trans;
    sc_time delay = SC_ZERO_TIME;
    trans.set_command(tlm::TLM_WRITE_COMMAND);
    trans.set_address(addr);
    trans.set_data_ptr(reinterpret_cast<unsigned char*>(&data));
    trans.set_data_length(sizeof(float));
    trans.set_streaming_width(sizeof(float));
    i_socket->b_transport(trans, delay);
}

float CPU::mem_read_float(uint64_t addr) {
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

void CPU::print_matrix(const char* name, uint64_t base_addr, int n) {
    cout << "[CPU] Data for " << name << ":" << endl;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            float val = mem_read_float(base_addr + (i * n + j) * sizeof(float));
            cout << setw(8) << fixed << setprecision(2) << val << " ";
        }
        cout << endl;
    }
}

void CPU::run_test() {
    cout << "[CPU] Starting Testbench..." << endl;

    int n = TEST_DIM;
    // Memory allocation in RAM
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
