#include <stdio.h>	// FILE, fopen, fclose, sprintf, printf, fwrite
#include <stdlib.h>	// malloc, free, NULL
#include <string.h>	// strcmp, strcpy
#include <unistd.h>	// F_OK, R_OK
//
// Input (arguments):
// 1) The channel IP
// 2) The channel port
// 3) Output file name
//
// Flow:
// 1) Recieve TCP request
// 2) Decode hamming code (63,57)
// 3) Write results to the output file
// 4) When read socket close, Write the statistics to the channel
// 5) Print the statistics
//
// Examples:
// > my_receiver 132.66.16.33 6343 rec_file
// received: 1260 bytes
// wrote: 1140 bytes
// corrected: 13 errors
//
int main (int argc, char *argv[]) {
	if (argc == 4) {
		printf("%s %s %s %s\n",argv[0],argv[1],argv[2],argv[3]);
	}
}
