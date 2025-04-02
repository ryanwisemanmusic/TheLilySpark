#ifndef __CMDSTREAM_H
#define __CMDSTREAM_H

#include <stdint.h>

struct agx_map_header 
{
 	uint32_t unk0; // cc c3 68 01
 	uint32_t unk1; // 01 00 00 00
 	uint32_t unk2; // 01 00 00 00
 	uint32_t unk3; // 28 05 00 80
 	uint32_t unk4; // cd c3 68 01
 	uint32_t unk5; // 01 00 00 00 
 	uint32_t unk6; // 00 00 00 00
 	uint32_t unk7; // 80 07 00 00
 	uint32_t nr_entries_1;
 	uint32_t nr_entries_2;
 	uint32_t unka; // 0b 00 00 00
 	uint32_t padding[4];
} __attribute__((packed));

struct agx_map_entry 
{
 	uint32_t unkAAA; // 20 00 00 00
 	uint32_t unk2; // 00 00 00 00 
 	uint32_t unk3; // 00 00 00 00
 	uint32_t unk4; // 00 00 00 00
 	uint32_t unk5; // 00 00 00 00
 	uint32_t unk6; // 00 00 00 00 
 	uint32_t unkBBB; // 01 00 00 00
 	uint32_t unk8; // 00 00 00 00
 	uint32_t unk9; // 00 00 00 00
 	uint32_t unka; // ff ff 01 00 
 	uint32_t index;
 	uint32_t padding[5];
} __attribute__((packed));

#endif