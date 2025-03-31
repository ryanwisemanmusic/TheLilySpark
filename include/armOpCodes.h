#ifndef ARMOPCODES_H
#define ARMOPCODES_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

enum agx_opcodes 
{
    /*
    This is our foundational register checking. We are using it to
    check for the math and test where this occurs. If these are anyway
    off, we may be unable to do a lot of the computations needed
    */
    OPC_FFMA_CMPCT_16 = 0x36,
    OPC_FFMA_CMPCT_SAT_16 = 0x76,
    OPC_FMUL_16 = 0x96,
    OPC_FADD_16 = 0xA6,
    OPC_FFMA_16 = 0xB6,
    OPC_FMUL_SAT_16 = 0xD6,
    OPC_FADD_SAT_16 = 0xE6,
    OPC_FFMA_SAT_16 = 0xF6,

    OPC_FROUND_32 = 0x0A,
    OPC_FFMA_CMPCT_32 = 0x3A,
    OPC_FFMA_CMPCT_SAT_32 = 0x7A,
    OPC_FMUL_32 = 0x9A,
    OPC_FADD_32 = 0xAA,
    OPC_FFMA_32 = 0xBA,
    OPC_FMUL_SAT_32 = 0xDA,
    OPC_FADD_SAT_32 = 0xEA,
    OPC_FFMA_SAT_32 = 0xFA,

    OPC_IADD = 0x0E,
    OPC_IMAD = 0x1E,
    OPC_ISHL = 0x2E,
    OPC_IADDSAT = 0x4E,
    OPC_ISHR = 0xAE,
    OPC_I2F = 0xBE,

    //We may need to check this one
    OPC_LOAD = 0x05,
    //This one could also be problematic
    OPC_STORE = 0x45,
    OPC_FCSEL = 0x02,
    OPC_ICSEL = 0x12,
    OPC_MOVI = 0x62,
    OPC_LD_COMPUTE = 0x72,
    //Another problem area that we need to check
    OPC_LD_THREADS_PER_GRID = 0x7E,
    OPC_UNK38 = 0x38, 
    OPC_STOP = 0x00,

    OPC_LD_VAR_NO_PERSPECTIVE = 0xA1,
    OPC_LD_VAR = 0xE1,
    OPC_UNK48 = 0x48,
    OPC_BLEND = 0x09,

    OPC_UNKD2 = 0xD2,
    OPC_UNK42 = 0x42,
    OPC_UNK52 = 0x52,
};

#define I 0
#define C 1

static struct 
{
    const char *name;
    unsigned size;
    bool complete;
} 
agx_opcode_table[256] = 
{
    [OPC_FADD_16] = { "fadd.16", 6, I },
    [OPC_FADD_SAT_16] = { "fadd.sat.16", 6, I },
    [OPC_FMUL_16] = { "fmul.16", 6, I },
    [OPC_FMUL_SAT_16] = { "fmul.sat.16", 6, I },
    [OPC_FFMA_CMPCT_16] = { "ffma.cmpct.16", 6, I },
    [OPC_FFMA_CMPCT_SAT_16] = { "ffma.cmpct.sat.16", 6, I },
    [OPC_FFMA_16] = { "ffma.16", 8, I },
    [OPC_FFMA_SAT_16] = { "ffma.sat.16", 8, I },

    [OPC_FROUND_32] = { "fround.32", 6, I },
    [OPC_FADD_32] = { "fadd.32", 6, C },
    [OPC_FADD_SAT_32] = { "fadd.sat.32", 6, C },
    [OPC_FMUL_32] = { "fmul.32", 6, C },
    [OPC_FMUL_SAT_32] = { "fmul.sat.32", 6, C },
    [OPC_FFMA_32] = { "ffma..32", 8, I },
    [OPC_FFMA_SAT_32] = { "ffma.sat.32", 8, I },
    [OPC_FFMA_CMPCT_32] = { "ffma.cmpct.32", 6, I },
    [OPC_FFMA_CMPCT_SAT_32] = { "ffma.cmpct.sat.32", 6, I },

    [OPC_I2F] = { "i2f", 6, I },
    [OPC_IADD] = { "iadd", 8, I },
    [OPC_IMAD] = { "imad", 8, I },
    [OPC_ISHL] = { "ishl", 8, I },
    [OPC_IADDSAT] = { "iaddsat", 8, I },
    [OPC_ISHR] = { "ishr", 8, I },

    [OPC_LOAD] = { "load", 8, I },
    [OPC_LD_VAR_NO_PERSPECTIVE] = { "ld_var.no_perspective", 8, I },
    [OPC_LD_VAR] = { "ld_var", 8, I },
    [OPC_STORE] = { "store", 8, I },
    [OPC_FCSEL] = { "fcsel", 8, I },
    [OPC_ICSEL] = { "icsel", 8, I },
    [OPC_MOVI] = { "movi", 4, C },
    [OPC_LD_COMPUTE] = { "ld_compute", 4, C },
    [OPC_LD_THREADS_PER_GRID] = { "ld_threads_per_grid", 6, I },
    [OPC_BLEND] = { "blend", 8, I },
    [OPC_STOP] = { "stop", 4, I },

    [OPC_UNK38] = { "unk38", 2, I },
    [OPC_UNK48] = { "unk48", 4, I },
    [OPC_UNK42] = { "unk42", 6, I},
    [OPC_UNK52] = { "unk52", 6, I},
    [OPC_UNKD2] = { "unkd2", 12, I},
};

#undef I
#undef C

unsigned agx_instr_bytes(uint8_t opc, uint8_t reg);

struct agx_src {
    unsigned type : 2;
    unsigned reg;
    bool size32;
    bool abs;
    bool neg;
    unsigned unk;
};

void agx_print_src(FILE *fp, struct agx_src s);

void agx_print_float_src(
    FILE *fp, unsigned type, unsigned reg, 
    bool size32, bool abs, bool neg);

struct agx_src agx_decode_float_src(uint16_t packed);

void agx_print_fadd_f32(FILE *fp, uint8_t *code);

void agx_print_ld_compute(uint8_t *code, FILE *fp);

unsigned agx_disassemble_instr(
    uint8_t *code, bool *stop, bool verbose, FILE *fp);

void agx_disassemble(void *_code, size_t maxlen, FILE *fp);

#endif