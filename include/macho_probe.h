/*
Copyright: Ryan Wiseman

This code is free to use for the betterment of Mac hardware development
in the hopes that it provides a great starting point for additonal 
endeavors. I do not authorize this for usage for malicious purposes,
and if you do so (not listening to me), 
I do not authorize you to use this code.

The purpose of this code is to probe our Mach-O so we can find
GPU related information so we can find the resources to draw a direct
connection to the M2 GPU.

For more info, consult: https://blog.efiens.com/post/luibo/osx/macho/
*/

#ifndef MACHO_PROBE_H
#define MACHO_PROBE_H

#include <stdio.h>
#include <stdint.h>

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