#include <errno.h>	// ERANGE, errno
#include <io.h>
#include <math.h>	// log
#include <stdio.h>	// FILE, printf, fprintf, sprintf, stderr, sscanf, fopen, fclose, fwrite
#include <stdlib.h>	// EXIT_FAILURE, EXIT_SUCCESS, NULL, strtol, malloc, free
#include <string.h>	// strlen, strcmp, strcpy, strerror, memset
//#include <unistd.h>	// F_OK, R_OK, lseek, close, read, write
#include <limits.h>	// LONG_MAX, LONG_MIN
#include <sys/stat.h>	// S_ISDIR, stat
#include <fcntl.h>	// O_RDONLY, O_WRONLY, O_CREAT, O_TRUNC, open
//#include <arpa/inet.h> 	// AF_INET, SOCK_STREAM, socket, htons, inet_addr, connect, sockaddr_in
#include "my_library.h"	// validateIP4Dotted
#include "winsock2.h"	// windows sockets include  
#include <stdint.h>
#pragma comment(lib, "Ws2_32.lib") // some kind of an include in order to compile and link REFERANCE: http://stackoverflow.com/questions/16948064/unresolved-external-symbol-lnk2019
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

int program_end(int error, int in_fd, int sock_fd) {
	char errmsg[256];
	int res = 0;
	if ((0 < in_fd)&(fclose(in_fd) == -1)) { // Upon successful completion, 0 shall be returned; otherwise, -1 shall be returned and errno set to indicate the error.
		fprintf(stderr, F_ERROR_INPUT_CLOSE_MSG, errmsg);
		res = errno;
	}

	if ((0 < sock_fd) && (closesocket(sock_fd) == -1)) { // Upon successful completion, 0 shall be returned; otherwise, -1 shall be returned and errno set to indicate the error.
		fprintf(stderr, F_ERROR_SOCKET_CLOSE_MSG, errmsg);
		res = errno;
	}

	if ((error != 0) || (res != 0)) {
		fprintf(stderr, ERROR_EXIT_MSG);
		if (error != 0) { // If multiple error occurred, Print the error that called 'program_end' function.
			res = error;
		}
	}
	return res;
}
int main(int argc, char *argv[]) {
	// Function variables
	int bin_from_count = 0;				// Point to the current cell at 'bin_from' (range from 0 to HAMMING_FROM)
	int indx = 0;					// Temporary loop index (traditionally 'i')
	int indxBinary[HAMMING_BINARY_LEN];		// The binary representation of 'indx' as array of ints
	int jndx = 0;					// Temporary loop index (traditionally 'j')
									//int indxOnes = 0;				// Number of '1' in index binary representation // TODO XXX
	int input_port = 0;				// The channel port (type == int)
	FILE* in_fd = 0;					// The input file
	SOCKET sock_fd = 0;				// The socket file descriptor (FD)
	int counter_dst = 0;				// The number of bytes we wrote to the channel
	int counter_src = 0;				// The number of bytes we read from input
	int padding = 0;				// The amount of padding chars that been have added
	char errmsg[256];
	char read_buf[MAX_BUF + 1];			// The string buffer readed from the input
	char bin_from[8 * MAX_BUF + 1];			// The binary representation of 'read_buf'
	char bin_to[8 * MAX_BUF_THEORY + 1];		// The binary representation after hamming calculation
	char write_buf[MAX_BUF_THEORY + 1];		// The string buffer to be sent to the channel
	char input_port_char[6];			// The channel port (type == string)
	char* endptr_PORT;				// strtol() for 'input_port'
	char report[][36] = { "","","" };			// The 3 values for the report // 36 == 3*len("–2147483648")+2*len(":")+1
	char *report_str, *report_token, *report_saveptr;	// Variables to split the string {Ex. report = input.split(':',3)}
	struct sockaddr_in serv_addr;			// The data structure for the channel
	struct stat in_stat;				// The data structure for the input file
	WORD wVersionRequested;					//win' sockets variables. 
	WSADATA wsaData;						//win' sockets variables. 
	int err;								//win' sockets variables. 


											// Init variables
	memset(bin_from, '0', sizeof(bin_from));
	memset(read_buf, '0', sizeof(read_buf));
	memset(write_buf, '0', sizeof(write_buf));
	memset(&serv_addr, '0', sizeof(serv_addr));
	// Check correct call structure
	if (argc != 4) {
		if (argc < 4) {
			printf(USAGE_OPERANDS_MISSING_MSG, argv[0], "IN");
		}
		else {
			printf(USAGE_OPERANDS_SURPLUS_MSG, argv[0], "IN");
		}
		return EXIT_FAILURE;
	}
	// Check input IP
	if (validateIP4Dotted(argv[1]) == -1) { // The function return 0 if input string is a valid IPv4 address, Otherwise return -1
		printf(IP_INVALID_MSG, argv[1]);
		return EXIT_FAILURE;
	}
	// Check input port
	input_port = strtol(argv[2], &endptr_PORT, 10); // If an underflow occurs. strtol() returns LONG_MIN. If an overflow occurs, strtol() returns LONG_MAX. In both cases, errno is set to ERANGE.
	if ((errno == ERANGE && (input_port == (int)LONG_MAX || input_port == (int)LONG_MIN)) || (errno != 0 && input_port == 0)) {
		strerror_s(errmsg, 255, errno);
		fprintf(stderr, F_ERROR_FUNCTION_STRTOL_MSG, errmsg);
		return errno;
	}
	else if ((endptr_PORT == argv[2]) || (input_port < 1) || (input_port > 65535)) { // (Empty string) or (not in range [1,65535])
		printf(PORT_INVALID_MSG);
		return EXIT_FAILURE;
	}
	else if (sprintf(input_port_char, "%d", input_port) < 0) { // sprintf(), If an output error is encountered, a negative value is returned.
		fprintf(stderr, F_ERROR_FUNCTION_SPRINTF_MSG);
		return EXIT_FAILURE;
	}
	else if (strcmp(input_port_char, argv[2]) != 0) { // Contain invalid chars
		printf(PORT_INVALID_MSG);
		return EXIT_FAILURE;
	}
	// Check input file - ||We can not assume anything regarding the size of IN.||
	if ((in_fd = fopen(argv[3], "rb")) == NULL) { // Upon successful completion, ... return a non-negative integer .... Otherwise, -1 shall be returned and errno set to indicate the error.
		fprintf(stderr, F_ERROR_INPUT_FILE_MSG, argv[3], errmsg); // IN must exist, otherwise output an error and exit.
		return program_end(errno, in_fd, sock_fd);
	}

	/// start up - windows sockets 
	// the next few lines regarding the windows sockets initiation
	// is based on the code from the offital microsoft documentation. 
	// source - https://msdn.microsoft.com/en-us/library/windows/desktop/ms742213(v=vs.85).aspx

	/* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */

	wVersionRequested = MAKEWORD(2, 2);

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		printf("WSAStartup failed with error: %d\n", err);
		printf(F_ERROR_SOCKET_CREATE_MSG);
		return EXIT_FAILURE;
	}

	/* Confirm that the WinSock DLL supports 2.2.*/
	/* Note that if the DLL supports versions greater    */
	/* than 2.2 in addition to 2.2, it will still return */
	/* 2.2 in wVersion since that is the version we      */
	/* requested.                                        */

	//if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
	//	/* Tell the user that we could not find a usable */
	//	/* WinSock DLL.                                  */
	//	printf("Could not find a usable version of Winsock.dll\n");
	//	WSACleanup();
	//	return 1;
	//}

	// Open connection to the channel // Data should be sent via a TCP socket, to the channel specified by IP:PORT.
	if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) { // On success, a file descriptor for the new socket is returned. On error, -1 is returned, and errno is set appropriately.
		strerror_s(errmsg, 255, errno);
		fprintf(stderr, F_ERROR_SOCKET_CREATE_MSG, errmsg);
		return program_end(errno, in_fd, sock_fd);
	}
	if (DEBUG) { printf("FLAG 0\n"); }
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(input_port);
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	if (connect(sock_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) { // Upon successful completion, connect() shall return 0; otherwise, -1 shall be returned and errno set to indicate the error.
		strerror_s(errmsg, 255, errno);
		fprintf(stderr, F_ERROR_SOCKET_CONNECT_MSG, errmsg);
		return program_end(errno, in_fd, sock_fd);
	}
	if (DEBUG) { printf("FLAG 1\n"); } 
	write_buf[0] = '\0';
	// All inputs variables are valid, Stsrt working
	while (1) {
		memset(bin_to, '0', sizeof(bin_to));
		if (DEBUG) { printf("FLAG 2\n"); } 
										   // Read input file IN
										   
		if ((counter_src = fread(read_buf, 1, MAX_BUF, in_fd)) == -1) { // On success, the number of bytes read is returned (zero indicates end of file), .... On error, -1 is returned, and errno is set appropriately.
			fprintf(stderr, F_ERROR_INPUT_READ_MSG, argv[3], errmsg);
			return program_end(errno, in_fd, sock_fd);
		}
		else if (counter_src == 0) { // Received EOF, Exit the infinity loop
			if (DEBUG) { printf("finished writing, now moving to reading. \n"); }
			break;
		} // TODO - is is? 
		// Calc hamming code (63,57) 
		// len(read_buf)  = counter_src;
		// len(bin_from)  = counter_src*8;
		// len(bin_to)    = counter_src*8 + padding;
		// len(write_buf) = counter_src   + padding/8;
		str2bin(read_buf, bin_from, counter_src);
		padding = 0;
		while ((padding + 1)*HAMMING_FROM <= counter_src * 8) { // We read up-to MAX_BUF chars, and process HAMMING_FROM bits at a time.
			bin_from_count = 0;
			for (indx = 0; indx<HAMMING_TO; indx++) {
				if (decimalToBinary(indx + 1, indxBinary) != 1) { // Data bit - calculate XOR for relevants parity bits
					if (bin_from[(padding*HAMMING_FROM) + bin_from_count] == '1') { // If value is one
						for (jndx = 0; jndx<HAMMING_BINARY_LEN; jndx++) { // Foreach bit
							if (indxBinary[jndx] == 1) {			  // If it's one
								bin_to[(padding*HAMMING_TO) + (int)pow(2, jndx) - 1] ^= 1; // XOR
	
							}
						}
					}
					bin_to[(padding*HAMMING_TO) + indx] = bin_from[(padding*HAMMING_FROM) + bin_from_count];
					//if (DEBUG) {printf("b[%d]=b[%d];\n",(padding*HAMMING_TO)+indx,(padding*HAMMING_FROM)+bin_from_count);} // TODO XXX DEBUG
					bin_from_count++;
				}
			}
			padding += 1;
		}
		padding *= HAMMING_TO - HAMMING_FROM;
		bin2str(bin_to, write_buf, counter_src * 8 + padding);
		if (DEBUG) { printf("FLAG 3 - Read %d chars - %.*s\n", counter_src, counter_src, read_buf); } 
																									  // Sending data from the input file IN to the server,
		if ((counter_dst = send(sock_fd, write_buf, counter_src + padding / 8, 0)) < 0) { // On success, the number of bytes written is returned (zero indicates nothing was written). On error, -1 is returned, and errno is set appropriately.
			fprintf(stderr, F_ERROR_SOCKET_WRITE_MSG, errmsg);
			return program_end(errno, in_fd, sock_fd);
		}
		else if (counter_dst == 0) {
			break;
		}
		if (DEBUG) { printf("FLAG 4 - Send %d chars\n", counter_dst); } 
	} // end of while. 
	  // Close write socket
	if (shutdown(sock_fd, SD_SEND) == -1) { // Upon successful completion, shutdown() shall return 0; otherwise, -1 shall be returned and errno set to indicate the error.
		fprintf(stderr, F_ERROR_SOCKET_SHUTDOWN_MSG, errmsg);
		return program_end(errno, in_fd, sock_fd);
	}
	// Wait and print the response
	//strncpy(read_buf,"0:0:0",5);
	if ((counter_dst = recv(sock_fd, read_buf, MAX_BUF, 0)) == -1) { // On success, the number of bytes read is returned (zero indicates end of file), .... On error, -1 is returned, and errno is set appropriately.
		fprintf(stderr, F_ERROR_SOCKET_READ_MSG, errmsg);
		return program_end(errno, in_fd, sock_fd);
	}
	read_buf[counter_dst] = '\0';
	if (DEBUG) { printf("FLAG 5 - %s ~ %d\n", read_buf, counter_dst); } 
	for (report_str = read_buf, indx = 0; indx<3; report_str = NULL, indx++) {
		report_token = strtok_s(report_str, ":", &report_saveptr);
		if (report_token == NULL) {
			break;
		}
		strcpy(report[indx], report_token);
	}
	fprintf(stderr, REPORT_SENDER, report[0], report[1], report[2]);
	// TODO TODO TODO
	// Free resources!!! and exit 
	// Once all data is sent, and the client finished reading and handling any data received from the channel,
	// The client closes the connection and finishes.
	return program_end(EXIT_SUCCESS, in_fd, sock_fd);
}
