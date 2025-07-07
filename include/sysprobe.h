#ifndef SYSPROBE_H
#define SYSPROBE_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

//Sysprobe Read/Write related functions
int sysprobe_mem_read(uint64_t start_addr, size_t length, void *out_buf);
void sysprobe_save_report(const char *filename, const char *content);

//Opcode related functions
int sysprobe_exec_opcodes(const uint8_t *code, size_t len, FILE *log);
void sysprobe_prepare_arm64_asm(FILE *log);

//Print related functions
void sysprobe_print_summary(FILE *out);
void sysprobe_print_macho_info(const char *path, FILE *log);
void sysprobe_print_mem_info(FILE *log);
void sysprobe_print_cpu_info(FILE *log);
void sysprobe_print_gpu_info(FILE *log);

//Probing related functions
int sysprobe_mem_map(uint64_t addr, size_t length, void *out_buf);
int sysprobe_exec_custom(const uint8_t *code, size_t len, FILE *log);
uint64_t sysprobe_time_op(void (*fn)(void), FILE *log);

//Sysprobe Enumerate related functions
void sysprobe_dev_enumerate(FILE *log);
void sysprobe_dir_enumerate(const char *dirpath, FILE *log);

// Patch a Mach-O file
int sysprobe_patch_macho(const char *path, uint8_t new_byte, FILE *log);

//Sysprobe scan related functions
void sysprobe_recursive_dir(const char *dir, void (*file_cb)(const char*, void*), void *userdata, int depth);
void sysprobe_search_graphics_devices(FILE *log);

//Sysprobe List related functions
void sysprobe_list_kexts(FILE *log);
void sysprobe_list_iokit_devices(FILE *log);
void sysprobe_list_framebuffer_devices(FILE *log);
void sysprobe_list_display_dirs(FILE *log);

//Sysprobe Test/Output related functions
void sysprobe_log_findings(const char *msg, FILE *log);
void sysprobe_prepare_graphics_test(const char *device, FILE *log);
int sysprobe_devfile(const char *path, FILE *log);

#endif // SYSPROBE_H