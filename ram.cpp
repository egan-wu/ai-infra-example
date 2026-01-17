#include "ram.h"
#include <cstring>

RAM::RAM(sc_module_name name) : sc_module(name), t_socket("t_socket") {
    t_socket.register_b_transport(this, &RAM::b_transport);
    mem_size = 1024 * 1024 * 4; // 4MB
    memory = new uint8_t[mem_size];
    memset(memory, 0, mem_size);
}

RAM::~RAM() {
    delete[] memory;
}

void RAM::b_transport(tlm::tlm_generic_payload& trans, sc_time& delay) {
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
