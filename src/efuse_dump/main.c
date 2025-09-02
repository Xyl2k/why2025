#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#define EFUSE_BASE_ADDR   0x5012D000u
#define EFUSE_SIZE_BYTES  0x1000u  /* 4 KB */
#define LINE_BYTES        16u

static void hex_dump_region(uintptr_t base, size_t len) {
    volatile const uint8_t *p = (volatile const uint8_t *)base;

    for (size_t off = 0; off < len; off += LINE_BYTES) {
        uint8_t buf[LINE_BYTES];
        size_t chunk = (off + LINE_BYTES <= len) ? LINE_BYTES : (len - off);

        for (size_t i = 0; i < chunk; ++i) buf[i] = p[off + i];

        printf("%08" PRIxPTR "  ", (uintptr_t)(base + off));
        for (size_t i = 0; i < LINE_BYTES; ++i) {
            if (i < chunk) printf("%02X ", buf[i]); else printf("   ");
            if (i == 7) printf(" ");
        }
        printf(" |");
        for (size_t i = 0; i < chunk; ++i) {
            uint8_t c = buf[i];
            putchar((c >= 32 && c <= 126) ? c : '.');
        }
        printf("|\n");
    }
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    printf("\n=== eFuse raw dump: 0x%08" PRIxPTR " - 0x%08" PRIxPTR " (%u bytes) ===\n",
           (uintptr_t)EFUSE_BASE_ADDR,
           (uintptr_t)(EFUSE_BASE_ADDR + EFUSE_SIZE_BYTES - 1u),
           (unsigned)EFUSE_SIZE_BYTES);

    hex_dump_region(EFUSE_BASE_ADDR, EFUSE_SIZE_BYTES);
    printf("=== End of eFuse dump ===\n");
    return 0;
}