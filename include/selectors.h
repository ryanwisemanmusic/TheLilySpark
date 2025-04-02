#include <stdint.h>

enum agx_selector 
{
    AGX_SELECTOR_SET_API = 0x7,
    AGX_SELECTOR_ALLOCATE_MEM = 0xA,
    AGX_SELECTOR_CREATE_CMDBUF = 0xF,
    AGX_SELECTOR_SUBMIT_COMMAND_BUFFERS = 0x1E,
    AGX_SELECTOR_GET_VERSION = 0x23,
    AGX_NUM_SELECTORS = 0x30
};

static const char *selector_table[AGX_NUM_SELECTORS] = 
{
    "unk0",
    "unk1",
    "unk2",
    "unk3",
    "unk4",
    "unk5",
    "unk6",
    "SET_API",
    "unk8",
    "unk9",
    "ALLOCATE_MEM",
    "unkB",
    "unkC",
    "unkD",
    "unkE",
    "CREATE_CMDBUF",
    "unk10",
    "unk11",
    "unk12",
    "unk13",
    "unk14",
    "unk15",
    "unk16",
    "unk17",
    "unk18",
    "unk19",
    "unk1A",
    "unk1B",
    "unk1C",
    "unk1D",
    "SUBMIT_COMMAND_BUFFERS",
    "unk1F",
    "unk20",
    "unk21",
    "unk22",
    "GET_VERSION",
    "unk24",
    "unk25",
    "unk26",
    "unk27",
    "unk28",
    "unk29",
    "unk2A",
    "unk2B",
    "unk2C",
    "unk2D",
    "unk2E",
    "unk2F"
};

static inline const char *
wrap_selector_name(uint32_t selector)
{
    return (selector < AGX_NUM_SELECTORS) ? selector_table[selector] : "unk??";
}

struct agx_create_cmdbuf_resp 
{
    void *map;
    uint32_t size;
    uint32_t id;
} __attribute__((packed));
