#ifndef MACHO_PROBE_H
#define MACHO_PROBE_H
#include <stdio.h>
#include <stdint.h>
// Print Mach-O header info for a file
void macho_print_info(const char *path, FILE *log);

//We print the Macho-O header into a file here
void print_macho_header(const char *path, FILE *log);

//Mach-O patch related functions
int macho_patch_first_byte(const char *path, uint8_t new_byte, FILE *log);

//Mach-O directory related functions
void macho_compare_dirs(const char *dir1, const char *dir2, FILE *log);
void macho_extract_dir(const char *dir, FILE *log);

//Mach-O probe related functions
void macho_deep_probe(const char *dir, FILE *log);
void macho_recursive_scan(const char *dir, FILE *log);
void macho_scan_dylibs(const char *dir, FILE *log);
void macho_scan_agx(const char *dir, FILE *log);

//Mach-O print related functions
void macho_print_load_commands(const char *path, FILE *log);
void macho_print_signature(const char *path, FILE *log);

//Mach-O list/architecture related functions
void macho_list_linked_libs(const char *path, FILE *log);
void macho_list_architectures(const char *path, FILE *log);

#endif

/*
TODO: - Additional scanning actions 
      - Tree mapping the Mach-O space
      - Race condition checking as we scan the Mach-O space
*/