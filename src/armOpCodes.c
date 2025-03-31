//Credit to Alyssa Rosenzweig

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

#include "armOpCodes.h"


unsigned agx_instr_bytes(uint8_t opc, uint8_t reg)
{
    if (opc == OPC_MOVI && (reg & 0x1))
        return 6;
    else
        return agx_opcode_table[opc].size ?: 2;
}

void agx_print_src(FILE *fp, struct agx_src s)
{
    const char *types[] = { "#", "unk1:", "const_", "" };

    fprintf(fp, ", %s%s%u%s%s%s", s.size32 ? "w": "h",
                    types[s.type], s.reg, 
                    s.abs ? ".abs" : "", s.neg ? ".neg" : "", 
                    s.unk ? ".unk" : "" );

}

void agx_print_float_src(
    FILE *fp, unsigned type, unsigned reg, 
    bool size32, bool abs, bool neg)
{
    assert (type <= 3);
    agx_print_src(fp, (struct agx_src)
    {
        .type = type, .reg = reg, .size32 = size32, 
        .abs = abs, .neg = neg
    }
                 );
}

struct agx_src agx_decode_float_src(uint16_t packed)
{
    return (struct agx_src) 
    {
        .reg = (packed & 0x3F),
        .type = (packed & 0xC0) >> 6,
        .unk = (packed & 0x100),
        .size32 = (packed & 0x200),
        .abs = (packed & 0x400),
        .neg = (packed & 0x800),
    };
}

void agx_print_fadd_f32(FILE *fp, uint8_t *code)
{
    agx_print_src(fp, agx_decode_float_src(code[2] | ((code[3] & 0xF) << 8)));
    agx_print_src(fp, agx_decode_float_src((code[3] >> 4) | (code[4] << 4)));

    if (code[5])
        fprintf(fp, " /* unk5 = %02X */", code[5]);
}

void agx_print_ld_compute(uint8_t *code, FILE *fp)
{
    uint16_t arg = code[2] | (code[3] << 8);

    unsigned component = arg & 0x3;
    uint16_t selector = arg >> 2;

    fprintf(fp, ", ");

    switch (selector)
    {
        case 0x00:
            fprintf(fp, "[threadgroun_position_in_grid]");
            break;
        case 0x0c:
            fprintf(fp, "[threadgroup_position_in_threadgroup]");
            break;
        case 0x0d:
            fprintf(fp, "[thread_position_in_simdgroup]");
            break;
        case 0x104:
            fprintf(fp, "[thread_position_in_grid]");
            break;
        default:
            fprintf(fp, "[unk_%X]", selector);
            break;
    }

    fprintf(fp, ".%c", "xyzw"[component]);
}

//Dissasembly code
unsigned agx_disassemble_instr(
    uint8_t *code, bool *stop, bool verbose, FILE *fp)
{
    //Use two bytes to decode the opcode
    uint8_t opc = (code [0] & 0x7F) | (code[1] & 0x80);

    unsigned bytes = agx_instr_bytes(opc, code[1]);

    if (verbose || !agx_opcode_table[opc].complete)
    {
        fprintf(fp, "#");
        for (unsigned i = 0; i < bytes; ++i)
            fprintf(fp, " %02X", code[i]);
        fprintf(fp, "\n");
    }
    unsigned op_unk80 = code[0] & 0x80;
    fprintf(fp, "%c", op_unk80 ? '+' : '-');

    if (agx_opcode_table[opc].name)
        fputs(agx_opcode_table[opc].name, fp);
    else
        fprintf(fp, "op_%02X", opc);

    if (opc == OPC_ICSEL)
    {
        unsigned mode = (code[7] & 0xF0) >> 4;
        if (mode == 0x1)
            fprintf(fp, ".eq"); 
        else if (mode == 0x2)
            fprintf(fp, ".imin");
        else if (mode == 0x3)
            fprintf(fp, ".ult");
        else if (mode == 0x4)
            fprintf(fp, ".imax");
        else if (mode == 0x5) 
            fprintf(fp, ".ugt");
        else
            fprintf(fp, ".unk%X", mode);
    }
    else if (opc == OPC_FCSEL)
    {
        unsigned mode = (code[7] & 0xF0) >> 4;

        if (mode == 0x6)
            fprintf(fp, ".fmin");
        else if (mode == 0xE)
            fprintf(fp, ".fmax");
        else
            fprintf(fp, ".unk%X", mode);
    }

    uint8_t dest = code[1];
    bool dest_32 = dest & 0x1;
    unsigned dest_reg = (dest >> 1) & 0x3F;

    fprintf(fp, " %s%u", 
                    dest_32 ? "w" : "h", 
                    dest_reg);

    switch (opc)
    {
        case OPC_LD_COMPUTE:
            agx_print_ld_compute(code, fp);
            break;
        case OPC_MOVI:
        {
            uint32_t imm = code[2] | (code[3] << 8);

            if (dest_32)
                imm |= (code[4] << 16) | (code[5] << 24);

            fprintf(fp, ", #0x%X", imm);
            break;
        }
        case OPC_FADD_32:
        case OPC_FADD_16:
        case OPC_FADD_SAT_16:
        case OPC_FADD_SAT_32:
        case OPC_FMUL_32:
        case OPC_FMUL_16:
        case OPC_FMUL_SAT_16:
        case OPC_FMUL_SAT_32:
            agx_print_fadd_f32(fp, code);
            break;
        default:
        {
            bool iadd = opc == OPC_IADD;

            if (bytes > 2)
            {
                agx_print_float_src(fp,
                        (code[2] & 0xC0) >> 6,
                        (code[2] & 0x3F) | (iadd ? ((code[5] & 0x0C) << 4) : 0),
                        code[3] & 0x20,
                        code[3] & 0x04,
                        code[3] & 0x08
                    );

                    agx_print_float_src(fp,
                        (code[4] & 0x0C) >> 2,
                        ((code[3] >> 4) & 0xF) | ((code[4] & 0x3) << 4) | ((code[7] & 0x3) << 6),
                        code[4] & 0x20, 
                        code[4] & 0x04,
                        code[4] & 0x80
                    );
            }

            if (bytes > 6 && !iadd)
            {
                agx_print_float_src(fp, 
                        (code[5] & 0xC0) >> 6,
                        (code[5] & 0x3F) | (code[6] & 0xC0),
                        code[6] & 0x20,
                        code[6] & 0x04,
                        code[6] & 0x08
                    );
            }

            break;

        }
    }

    fprintf(fp, "\n");

    if (code[0] == (OPC_STOP | 0x80))
        *stop = true;
    
    return bytes;
}

void agx_disassemble(void *_code, size_t maxlen, FILE *fp)
{
    if (maxlen > 256)
        maxlen = 256;

    uint8_t *code = _code;
    bool stop = false;
    unsigned bytes = 0;
    bool verbose = getenv("ASAHI_VERBOSE") != NULL;

    while ((bytes + 8) < maxlen && !stop)
        bytes += agx_disassemble_instr(code + bytes, &stop, verbose, fp);

    if (!stop)
        fprintf(fp, "// error: stop instruction not found\n");

}