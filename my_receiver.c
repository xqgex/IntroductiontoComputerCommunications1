#include <errno.h>	// ERANGE, errno
#include <io.h>
#include <math.h>	// pow
#include <stdio.h>	// FILE, printf, fprintf, sprintf, stderr, sscanf, fopen, fclose, fwrite
#include <stdlib.h>	// EXIT_FAILURE, EXIT_SUCCESS, NULL, strtol, malloc, free
#include <string.h>	// strlen, strcmp, strcpy, strerror, memset
#include <limits.h>	// LONG_MAX, LONG_MIN
#include <sys/stat.h>	// S_ISDIR, stat
#include <fcntl.h>	// O_RDONLY, O_WRONLY, O_CREAT, O_TRUNC, open
#include "winsock2.h"	// windows sockets include  
#include <stdint.h>
#include "my_library.h"	// validateIP4Dotted
#pragma comment(lib, "Ws2_32.lib") // credit: http://stackoverflow.com/questions/16948064/unresolved-external-symbol-lnk2019

//
// Input (arguments):
// 1) The channel IP
// 2) The channel port
// 3) Output file name
//
// Flow:
// 1) Receive TCP request
// 2) Decode hamming code (63,57)
// 3) Write results to the output file
// 4) When read socket close, Write the report to the channel
// 5) Print the report
//
// Examples:
// > my_receiver 132.66.16.33 6343 rec_file
// received: 1260 bytes
// wrote: 1140 bytes
// corrected: 13 errors
//

int program_end(int error, int out_fd, int channel_fd) {
	char errmsg[256];
	int res = 0;
	if ((0 < out_fd)&&(fclose(out_fd) == -1)) { // Upon successful completion, 0 shall be returned; otherwise, -1 shall be returned and errno set to indicate the error.
		strerror_s(errmsg, 255, errno);
		fprintf(stderr, F_ERROR_OUTPUT_CLOSE_MSG, errmsg);
		res = errno;
	}
	if ((0 < channel_fd) && (closesocket(channel_fd) == -1)) { // Upon successful completion, 0 shall be returned; otherwise, -1 shall be returned and errno set to indicate the error.
		strerror_s(errmsg, 255, errno);
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
	int input_port = 0;					// The channel port (type == int)
	int indx = 0;						// Temporary loop index (traditionally 'i')
	int jndx = 0;						// Temporary loop index (traditionally 'j')
	int indxBinary[HAMMING_BINARY_LEN];	// The binary representation of 'indx' as array of ints
	int errorBinary[HAMMING_BINARY_LEN];
	int loop_count1 = 0;
	int loop_count2 = 0;
	int bin_to_count = 0;
	int counter_client = 0;				// The number of bytes we read from the client
	int counter_dst = 0;				// The number of bytes we wrote to output
	int padding = 0;					// The amount of padding chars that been have added
	int report[] = { 0,0,0 };			// The 3 values for the report
	char errmsg[256];
	char input_port_char[6];			// The channel port (type == string)
	char read_buf[MAX_BUF_THEORY + 1];	// The string buffer got from the channel
	char bin_from[8 * MAX_BUF_THEORY + 1];	// The binary representation of 'read_buf'
	char bin_to[8 * MAX_BUF + 1];		// The binary representation after hamming calculation
	char write_buf[MAX_BUF + 1];		// The string buffer to be writen to the output
	char* endptr_PORT;					// strtol() for 'input_port'
	FILE* out_fd = 0;					// The output file 'file descriptor' (FD)
	SOCKET channel_fd = 0;				// The channel file descriptor (FD)
	struct sockaddr_in serv_addr;		// The data structure for the channel
	WORD wVersionRequested;				// win' sockets variables.
	WSADATA wsaData;					// win' sockets variables.
	int err;							// win' sockets variables.
	// Init variables
	memset(bin_from, '0', sizeof(bin_from));
	memset(bin_to, '0', sizeof(bin_to));
	memset(read_buf, '0', sizeof(read_buf));
	memset(write_buf, '0', sizeof(write_buf));
	memset(&serv_addr, '0', sizeof(serv_addr));
	// Check correct call structure
	if (argc != 4) {
		if (argc < 4) {
			printf(USAGE_OPERANDS_MISSING_MSG, argv[0], " <IP>", " <PORT>", " <OUT>", "");
		} else {
			printf(USAGE_OPERANDS_SURPLUS_MSG, argv[0], " <IP>", " <PORT>", " <OUT>", "");
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
	// Check output file
	if ((out_fd = fopen(argv[3], "wb")) == -1) { // Upon successful completion, ... return a non-negative integer .... Otherwise, -1 shall be returned and errno set to indicate the error.
		strerror_s(errmsg, 255, errno);
		fprintf(stderr, F_ERROR_OUTPUT_FILE_MSG, argv[3], errmsg); // The path to the OUT file must exist, otherwise output an error and exit. (i.e., no need to check the folder, just try to open/create the file).
		return program_end(errno, out_fd, channel_fd); // If OUT does not exist, the client should create it. If it exists, it should truncate it.
	}
	// start up - windows sockets 
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
	/* Confirm that the WinSock DLL supports 2.2.        */
	/* Note that if the DLL supports versions greater    */
	/* than 2.2 in addition to 2.2, it will still return */
	/* 2.2 in wVersion since that is the version we      */
	/* requested.                                        */
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                  */
		printf("Could not find a usable version of Winsock.dll\n");
		WSACleanup();
		return 1;
	}
	// Create a TCP socket that listens on PORT (use 10 as the parameter for listen).
	if ((channel_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) { // On success, a file descriptor for the new socket is returned. On error, -1 is returned, and errno is set appropriately.
		strerror_s(errmsg, 255, errno);
		fprintf(stderr, F_ERROR_SOCKET_CREATE_MSG, errmsg);
		return program_end(errno, out_fd, channel_fd);
	}
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(input_port);
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);//htonl(INADDR_ANY); // INADDR_ANY = any local machine address
	if (connect(channel_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) { // On success, zero is returned. On error, -1 is returned, and errno is set appropriately.
		strerror_s(errmsg, 255, errno);
		fprintf(stderr, F_ERROR_SOCKET_BIND_MSG, errmsg);
		return program_end(errno, out_fd, channel_fd);
	}
	write_buf[0] = '\0';
	while (1) {
		// b. Read data from the client until EOF.
		// You cannot assume anything regarding the overall size of the input.
		if ((counter_client = recv(channel_fd, read_buf, MAX_BUF_THEORY, 0)) == SOCKET_ERROR) { // On success, the number of bytes read is returned (zero indicates end of file), .... On error, -1 is returned, and errno is set appropriately.
			strerror_s(errmsg, 255, errno);
			fprintf(stderr, F_ERROR_SOCKET_READ_MSG, errmsg);
			return program_end(errno, out_fd, channel_fd);
		} else if (counter_client == 0) {
			break;
		}
		// Decode hamming code (63,57)
		// len(read_buf)  = counter_client;
		// len(bin_from)  = counter_client*8;
		// len(bin_to)    = counter_client*8 - padding;
		// len(write_buf) = counter_client   - padding/8;
		str2bin(read_buf, bin_from, counter_client);
		padding = 0;
		while ((padding + 1)*HAMMING_TO <= counter_client * 8) {
			bin_to_count = 0;
			memset(errorBinary, 0, sizeof(errorBinary));
			for (indx = 0; indx<HAMMING_TO; indx++) {
				loop_count1 = decimalToBinary(indx + 1, indxBinary);
				if (bin_from[(padding*HAMMING_TO) + indx] == '1') { // If value is one
					for (jndx = 0; jndx<HAMMING_BINARY_LEN; jndx++) { // Foreach bit
						if (indxBinary[jndx] == 1) { // If it's one
							errorBinary[jndx] ^= 1; // XOR
						}
					}
				}
				if (loop_count1 != 1) { // Data bit - calculate XOR for relevants parity bits
					bin_to[(padding*HAMMING_FROM) + bin_to_count] = bin_from[(padding*HAMMING_TO) + indx];
					bin_to_count++;
				}
			}
			// Check for error
			loop_count1 = 0;
			loop_count2 = 0;
			for (jndx = 0; jndx<HAMMING_BINARY_LEN; jndx++) { // Foreach bit
				loop_count1 += errorBinary[jndx]*(int)pow(2, jndx);
			}
			for (jndx = 0; jndx<HAMMING_BINARY_LEN; jndx++) { // Foreach bit
				if ((int)pow(2, jndx) < loop_count1) {
					loop_count2 += 1;
				}
			}
			if (0 < loop_count1) { // We found error
				report[2] += 1; // corrected
				if (1 < loop_count1 - loop_count2) { // And it's not in one of the parity bits
					bin_to[padding*HAMMING_FROM + loop_count1 - loop_count2 - 1] ^= 1; // XOR
				}
			}
			padding += 1;
		}
		padding *= HAMMING_TO - HAMMING_FROM;
		bin2str(bin_to, write_buf, counter_client * 8 - padding);
		// writing the result (the client's data) into the output file OUT.
		if ((counter_dst = fwrite(write_buf, 1, counter_client - padding / 8, out_fd)) == -1) { // On success, the number of bytes written is returned (zero indicates nothing was written). On error, -1 is returned, and errno is set appropriately.
			strerror_s(errmsg, 255, errno);
			fprintf(stderr, F_ERROR_OUTPUT_WRITE_MSG, argv[3], errmsg);
			return program_end(errno, out_fd, channel_fd);
		}
		report[0] += counter_client; // received
		report[1] += counter_dst; // reconstructed/wrote
	}
	// When 'read socket' closed, Write the report to the channel
	if (sprintf(write_buf, "%d:%d:%d%c", report[0], report[1], report[2], '\0') < 0) { // sprintf(), If an output error is encountered, a negative value is returned.
		fprintf(stderr, F_ERROR_FUNCTION_SPRINTF_MSG);
		return program_end(errno, out_fd, channel_fd);
	}
	if ((counter_dst = send(channel_fd, write_buf, strlen(write_buf), 0)) == -1) { // On success, the number of bytes written is returned (zero indicates nothing was written). On error, -1 is returned, and errno is set appropriately.
		strerror_s(errmsg, 255, errno);
		fprintf(stderr, F_ERROR_SOCKET_WRITE_MSG, errmsg);
		return program_end(errno, out_fd, channel_fd);
	}
	// Print the report
	fprintf(stderr, REPORT_RECEIVER, report[0], report[1], report[2]);
	// Once all data is sent, and the client finished reading and handling any data received from the channel,
	// The client closes the connection and finishes.
	if (shutdown(channel_fd, SD_SEND) == -1) { // Upon successful completion, shutdown() shall return 0; otherwise, -1 shall be returned and errno set to indicate the error.
		strerror_s(errmsg, 255, errno);
		fprintf(stderr, F_ERROR_SOCKET_SHUTDOWN_MSG, errmsg);
		return program_end(errno, out_fd, channel_fd);
	}
	return program_end(EXIT_SUCCESS, out_fd, channel_fd);
}
