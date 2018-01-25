#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#define PIPERW 2
#define PIPEBUFSIZE 128

void input(int, int, int[]);
void output(int);
void translate(int, int);
