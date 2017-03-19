#include <stdio.h>	// FILE, fopen, fclose, sprintf, printf, fwrite
#include <stdlib.h>	// malloc, free, NULL
#include <string.h>	// strcmp, strcpy
#include <unistd.h>	// F_OK, R_OK
//
// Input (arguments):
// 1) The channel IP
// 2) The channel port
// 3) Input file name
//
// Flow:
// 1) Create TCP socket to the channel
// 2) Calc hamming code (63,57)
// 3) Close write socket
// 4) Print the response
// 5) Free resources and exit
//
// Examples:
// > my_sender 132.66.16.33 6342 my_file
// received: 1260 bytes
// reconstructed: 1140 bytes
// corrected: 13 errors
// > my_sender 132.66.16.33 6342 my_file2
// received: 1890 bytes
// reconstructed: 1710 bytes
// corrected: 0 errors
//
int main (int argc, char *argv[]) {
	if (argc == 4) {
		printf("%s %s %s %s\n",argv[0],argv[1],argv[2],argv[3]);
	}
}
