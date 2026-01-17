#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

// Define Memory Map addresses
// These must match the SystemC configuration
#define RAM_BASE      0x00000000
#define TPU_BASE      0x10000000

// TPU Register Offsets
#define REG_ADDR_SRC_A 0x00
#define REG_ADDR_SRC_B 0x04
#define REG_ADDR_DST_C 0x08
#define REG_MATRIX_DIM 0x0C
#define REG_CMD_START  0x10
#define REG_STATUS     0x14

// Status Codes
#define STATUS_IDLE 0
#define STATUS_BUSY 1
#define STATUS_DONE 2

// Matrix Dimensions
#define N 32

// Helper to write to a physical address
void mmio_write(uintptr_t addr, uint32_t val) {
    volatile uint32_t *ptr = (volatile uint32_t *)addr;
    *ptr = val;
}

// Helper to read from a physical address
uint32_t mmio_read(uintptr_t addr) {
    volatile uint32_t *ptr = (volatile uint32_t *)addr;
    return *ptr;
}

int main() {
    printf("[GuestApp] Starting TPU Test...\n");

    // In a baremetal/syscall-emulation Gem5 setup,
    // we often need to be careful about where we place data.
    // Ideally, we malloc buffers. Since the RAM is at 0x0, standard malloc
    // should place them in the RAM range if the linker script is standard.
    // However, for direct DMA access, we need the PHYSICAL addresses.
    // In Gem5 SE mode, virtual=physical usually for simple setups,
    // or we can just pick hardcoded addresses in the lower RAM if we are careful.

    // Let's use hardcoded addresses for DMA to be safe and simple,
    // ensuring they don't clash with the code/stack (usually loaded higher up or at 0x10000).
    // Let's use 0x00100000 (1MB) as base for data.

    uintptr_t addr_a = 0x00100000;
    uintptr_t addr_b = 0x00200000;
    uintptr_t addr_c = 0x00300000;

    // Initialize Matrices
    // We need to write float data to these addresses.
    printf("[GuestApp] Initializing matrices...\n");

    volatile float* ptr_a = (float*)addr_a;
    volatile float* ptr_b = (float*)addr_b;
    volatile float* ptr_c = (float*)addr_c;

    for (int i = 0; i < N * N; i++) {
        ptr_a[i] = (float)(i % 9 + 1);
        ptr_b[i] = (float)((i + 3) % 7 + 1);
        ptr_c[i] = 0.0f; // Clear result
    }

    // Configure TPU
    printf("[GuestApp] Configuring TPU registers...\n");
    mmio_write(TPU_BASE + REG_ADDR_SRC_A, addr_a);
    mmio_write(TPU_BASE + REG_ADDR_SRC_B, addr_b);
    mmio_write(TPU_BASE + REG_ADDR_DST_C, addr_c);
    mmio_write(TPU_BASE + REG_MATRIX_DIM, N);

    // Start TPU
    printf("[GuestApp] Starting TPU...\n");
    mmio_write(TPU_BASE + REG_CMD_START, 1);

    // Poll for completion
    // Note: In a real OS we'd wait for interrupt, but polling is simpler here.
    printf("[GuestApp] Polling for completion...\n");
    while (1) {
        uint32_t status = mmio_read(TPU_BASE + REG_STATUS);
        if (status == STATUS_DONE) break;
    }
    printf("[GuestApp] TPU Done!\n");

    // Verify Result
    printf("[GuestApp] Verifying results...\n");
    int errors = 0;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            float sum = 0.0f;
            for (int k = 0; k < N; k++) {
                sum += ptr_a[i*N + k] * ptr_b[k*N + j];
            }
            float val = ptr_c[i*N + j];
            if (fabs(sum - val) > 0.01f) {
                if (errors < 5) {
                    printf("Error at [%d][%d]: Expected %f, Got %f\n", i, j, sum, val);
                }
                errors++;
            }
        }
    }

    if (errors == 0) {
        printf("TEST PASSED\n");
    } else {
        printf("TEST FAILED (%d errors)\n", errors);
    }

    return 0;
}
