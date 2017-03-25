#include <stdio.h>	// FILE, fopen, fclose, sprintf, printf, fwrite
#include <stdlib.h>	// malloc, free, NULL
#include <string.h>	// strcmp, strcpy
#include <unistd.h>	// F_OK, R_OK
//
// Input (arguments):
// 1) The sender port
// 2) The receiver port
// 3) Error probability
// 4) Random seed
//
// yuval: test push simple add.  
//
//
// Flow:
// 1) Open two TCP sockets
// 2) When recieving data fron the sender
// 3) Flip bits with the input probability and pass it to the receiver
// 4) When the sender close the socket, Close the socket to the receiver
// 5) When recieve data from the receiver, Transfer it to the sender
// 6) Close sockets to the sender and the receiver
// 7) Print the statistics
// 8) Free resources and exit
//
// Examples:
// > my_channel 6342 6343 0.001 12345
// sender: 132.66.48.12
// receiver: 132.66.50.12
// 1457 bytes flipped 13 bits
//
int main (int argc, char *argv[]) {
	if (argc == 5) {
		printf("%s %s %s %s %s\n",argv[0],argv[1],argv[2],argv[3],argv[4]);
	}
}
