/* SYSGEN.C - System Volume Packer for TMS99105 SBC */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MEMSIZE 65536
unsigned char mem[MEMSIZE];
unsigned char used_mem[MEMSIZE];
unsigned char temp_mem[MEMSIZE];
long lowest, highest;

int hexval(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

int hexbyte(const char *s) {
    int h = hexval((unsigned char)s[0]);
    int l = hexval((unsigned char)s[1]);
    if (h < 0 || l < 0) return -1;
    return (h << 4) | l;
}

void load_hex(const char *filename, unsigned char *buffer, unsigned char *tracker) {
    FILE *fp = fopen(filename, "r");
    char buf[256];
    lowest = -1; highest = -1;
    if (!fp) { perror(filename); exit(1); }

    while (fgets(buf, sizeof(buf), fp)) {
        if (buf[0] != ':') continue;
        int len = hexbyte(buf + 1);
        long addr = (hexbyte(buf + 3) << 8) | hexbyte(buf + 5);
        int type = hexbyte(buf + 7);
        if (type == 0 && len > 0) {
            for (int i = 0; i < len; i++) {
                buffer[addr + i] = hexbyte(buf + 9 + i * 2);
                if (tracker) tracker[addr + i] = 1;
                if (lowest < 0 || addr + i < lowest) lowest = addr + i;
                if (highest < 0 || addr + i > highest) highest = addr + i;
            }
        }
    }
    fclose(fp);
}

void format_name(const char *input, char *out) {
    memset(out, ' ', 11);
    int i = 0, j = 0;
    while (input[i] && input[i] != '.' && j < 8) out[j++] = toupper(input[i++]);
    if (input[i] == '.') {
        i++; j = 8;
        while (input[i] && input[i] != '.' && j < 11) out[j++] = toupper(input[i++]);
    }
}

int get_type(const char *ext) {
    if (strncmp(ext, "SYS", 3) == 0) return 0;
    if (strncmp(ext, "COM", 3) == 0) return 1;
    if (strncmp(ext, "EXE", 3) == 0) return 2;
    if (strncmp(ext, "PRO", 3) == 0) return 5;
    return 0; // Default
}

int main(int argc, char **argv) {
    if (argc < 4) {
        puts("Usage: sysgen PAYLOAD.HEX DSKINIT.HEX FILE1.SYS=FILE1.HEX [FILE2...]");
        return 1;
    }

    memset(mem, 0, MEMSIZE);
    memset(used_mem, 0, MEMSIZE);

    /* --- SAFE MEMORY MAP FOR PAYLOADS --- */
    int bat_ptr = 0x2000;
    int dir_ptr = 0x2200;
    int table_ptr = 0x2400;
    int staging_ptr = 0x3000;
    int next_free_block = 8;

    /* Initialize full 512-byte BAT Sector to 0000 (Free) and reserve blocks 0-7 */
    for(int j = 0; j < 512; j++) { mem[bat_ptr + j] = 0; used_mem[bat_ptr + j] = 1; }
    for(int i = 0; i < 8; i++) { mem[bat_ptr + i*2] = 0xFF; mem[bat_ptr + i*2 + 1] = 0x80; }

    /* Initialize full 512-byte DIR Sector to E5E5 (Empty) */
    for(int j = 0; j < 512; j++) { mem[dir_ptr + j] = 0xE5; used_mem[dir_ptr + j] = 1; }

    /* Pass DSKINIT straight through into the memory map (runs at >0500) */
    load_hex(argv[2], mem, used_mem);

    for (int i = 3; i < argc; i++) {
        char target_name[32], hex_file[128];
        char *eq = strchr(argv[i], '=');
        if (!eq) { puts("Error: Format must be TARGET.EXT=SOURCE.HEX"); return 1; }

        *eq = '\0';
        strcpy(target_name, argv[i]);
        strcpy(hex_file, eq + 1);

        memset(temp_mem, 0, MEMSIZE);
        load_hex(hex_file, temp_mem, NULL);

        int size_bytes = highest - lowest + 1;
        int sectors = (size_bytes + 511) / 512;
        int blocks = (sectors + 7) / 8;

        /* Pack flat binary data into Staging Area AND tag padded zeroes as used */
        memcpy(&mem[staging_ptr], &temp_mem[lowest], size_bytes);
        for(int j = 0; j < sectors * 512; j++) used_mem[staging_ptr + j] = 1;

        /* Build CP/M Directory Entry */
        char fcb_name[11];
        format_name(target_name, fcb_name);
        memcpy(&mem[dir_ptr], fcb_name, 11);
        mem[dir_ptr + 11] = get_type(strchr(target_name, '.') ? strchr(target_name, '.') + 1 : "");
        mem[dir_ptr + 12] = (next_free_block >> 8) & 0xFF;
        mem[dir_ptr + 13] = next_free_block & 0xFF;
        mem[dir_ptr + 14] = (sectors >> 8) & 0xFF;
        mem[dir_ptr + 15] = sectors & 0xFF;
        mem[dir_ptr + 16] = (lowest >> 8) & 0xFF;
        mem[dir_ptr + 17] = lowest & 0xFF;
        mem[dir_ptr + 18] = 0; /* Explicit Root Folder ID */
        dir_ptr += 32;

        /* BAT Chain */
        for(int b = 0; b < blocks; b++) {
            int blk = next_free_block + b;
            int next = (b == blocks - 1) ? 0xFF80 : (blk + 1);
            mem[bat_ptr + blk*2] = (next >> 8) & 0xFF;
            mem[bat_ptr + blk*2 + 1] = next & 0xFF;
        }

        /* Write-Table */
        mem[table_ptr] = (staging_ptr >> 8) & 0xFF; used_mem[table_ptr++] = 1;
        mem[table_ptr] = staging_ptr & 0xFF;        used_mem[table_ptr++] = 1;
        int lba = next_free_block * 16;
        mem[table_ptr] = (lba >> 8) & 0xFF;         used_mem[table_ptr++] = 1;
        mem[table_ptr] = lba & 0xFF;                used_mem[table_ptr++] = 1;
        mem[table_ptr] = (sectors >> 8) & 0xFF;     used_mem[table_ptr++] = 1;
        mem[table_ptr] = sectors & 0xFF;            used_mem[table_ptr++] = 1;

        staging_ptr += sectors * 512;
        next_free_block += blocks;
    }

    mem[table_ptr] = 0; used_mem[table_ptr++] = 1;
    mem[table_ptr] = 0; used_mem[table_ptr++] = 1;

    FILE *out = fopen(argv[1], "w");
    for (long a = 0; a < MEMSIZE; a += 16) {
        int active = 0;
        /* Trigger output based on Used Tracker, not byte value! */
        for(int j=0; j<16; j++) if (used_mem[a+j]) active = 1;

        if (active) {
            int sum = 16 + (a>>8) + (a&0xFF);
            fprintf(out, ":10%04lX00", a);
            for(int j=0; j<16; j++) {
                fprintf(out, "%02X", mem[a+j]);
                sum += mem[a+j];
            }
            fprintf(out, "%02X\n", (0x100 - (sum & 0xFF)) & 0xFF);
        }
    }
    fprintf(out, ":00000001FF\n");
    fclose(out);

    printf("Successfully packed %d files into %s\n", argc - 3, argv[1]);
    return 0;
}
