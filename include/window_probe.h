#ifndef WINDOW_PROBE_H
#define WINDOW_PROBE_H
#include <stdio.h>

//Generate window attempt
int window_probe_attempt(const char *device, int width, int height, FILE *log);

//Open window attempt
void window_probe_bruteforce(const char **devices, int num_devices, int width, int height, const char *logfile);

//Find the right files to open window attempt
int enumerate_dev_candidates(char candidates[][256], int max_candidates);
#endif