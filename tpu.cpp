#include "tpu.h"
#include <cstring>

TPU::TPU(sc_module_name name) : sc_module(name), t_socket("t_socket"), i_socket("i_socket") {
    t_socket.register_b_transport(this, &TPU::b_transport);

    SC_THREAD(process_thread);

    reg_addr_a = 0;
    reg_addr_b = 0;
    reg_addr_c = 0;
    reg_dim = 0;
    reg_status = STATUS_IDLE;

    irq_out.initialize(false);
}

void TPU::b_transport(tlm::tlm_generic_payload& trans, sc_time& delay) {
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

void TPU::process_thread() {
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
