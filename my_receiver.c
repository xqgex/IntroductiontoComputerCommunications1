#include <errno.h>	// ERANGE, errno
#include <io.h>
#include <stdio.h>	// FILE, printf, fprintf, sprintf, stderr, sscanf, fopen, fclose, fwrite
#include <stdlib.h>	// EXIT_FAILURE, EXIT_SUCCESS, NULL, strtol, malloc, free
#include <string.h>	// strlen, strcmp, strcpy, strerror, memset
//#include <unistd.h>	// F_OK, R_OK, lseek, close, read, write
#include <limits.h>	// LONG_MAX, LONG_MIN
#include <sys/stat.h>	// S_ISDIR, stat
#include <fcntl.h>	// O_RDONLY, O_WRONLY, O_CREAT, O_TRUNC, open
//#include <arpa/inet.h> 	// AF_INET, SOCK_STREAM, socket, htons, inet_addr, connect, sockaddr_in
#include "winsock2.h"	// windows sockets include  
#include <stdint.h>
#include "my_library.h"	// validateIP4Dotted

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
	if ((0 < out_fd)&(_close(out_fd) == -1)) { // Upon successful completion, 0 shall be returned; otherwise, -1 shall be returned and errno set to indicate the error.
		strerror_s(errmsg, 255, errno);
		fprintf(stderr, F_ERROR_OUTPUT_CLOSE_MSG, errmsg);
		res = errno;
	}
	if ((0 < channel_fd)&&(_close(channel_fd) == -1)) { // Upon successful completion, 0 shall be returned; otherwise, -1 shall be returned and errno set to indicate the error.
		strerror_s(errmsg, 255, errno);
		fprintf(stderr, F_ERROR_SOCKET_CLOSE_MSG, errmsg);
		res = errno;
	}
	if ((error != 0)||(res != 0)) {
		fprintf(stderr,ERROR_EXIT_MSG);
		if (error != 0) { // If multiple error occurred, Print the error that called 'program_end' function.
			res = error;
		}
	}
	return res;
}
int main (int argc, char *argv[]) {
	// Function variables
	int input_port = 0;			// The channel port (type == int)
	int out_fd = 0;				// The output file 'file descriptor' (FD)
	int channel_fd = 0;			// The channel file descriptor (FD)
	int conn_fd = 0;			// The connection file 'file descriptor' (FD)
	int counter_client = 0;			// The number of bytes we read from the client
	int counter_dst = 0;			// The number of bytes we wrote to output
	int padding = 0;			// The amount of padding chars that been have added
	int report[] = {0,0,0};			// The 3 values for the report
	char errmsg[256];
	char input_port_char[6];		// The channel port (type == string)
	char read_buf[MAX_BUF_THEORY+1];	// The string buffer got from the channel
	char bin_from[8*MAX_BUF_THEORY+1];	// The binary representation of 'read_buf'
	char bin_to[8*MAX_BUF+1];		// The binary representation after hamming calculation
	char write_buf[MAX_BUF+1];		// The string buffer to be writen to the output
	char* endptr_PORT;			// strtol() for 'input_port'
	struct sockaddr_in serv_addr;		// The data structure for the channel
	// Init variables
	memset(bin_from,'0',sizeof(bin_from));
	memset(bin_to,'0',sizeof(bin_to));
	memset(read_buf,'0',sizeof(read_buf));
	memset(write_buf,'0',sizeof(write_buf));
	memset(&serv_addr,'0',sizeof(serv_addr));
	// Check correct call structure
	if (argc != 4) {
		if (argc < 4) {
			printf(USAGE_OPERANDS_MISSING_MSG,argv[0],"OUT");
		} else {
			printf(USAGE_OPERANDS_SURPLUS_MSG,argv[0],"OUT");
		}
		return EXIT_FAILURE;
	}
	// Check input IP
	if (validateIP4Dotted(argv[1]) == -1) { // The function return 0 if input string is a valid IPv4 address, Otherwise return -1
		printf(IP_INVALID_MSG,argv[1]);
		return EXIT_FAILURE;
	}
	// Check input port
	input_port = strtol(argv[2],&endptr_PORT,10); // If an underflow occurs. strtol() returns LONG_MIN. If an overflow occurs, strtol() returns LONG_MAX. In both cases, errno is set to ERANGE.
	if ((errno == ERANGE && (input_port == (int)LONG_MAX || input_port == (int)LONG_MIN)) || (errno != 0 && input_port == 0)) {
		strerror_s(errmsg, 255, errno);
		fprintf(stderr, F_ERROR_FUNCTION_STRTOL_MSG, errmsg);
		return errno;
	} else if ( (endptr_PORT == argv[2])||(input_port < 1)||(input_port > 65535) ) { // (Empty string) or (not in range [1,65535])
		printf(PORT_INVALID_MSG);
		return EXIT_FAILURE;
	} else if (sprintf(input_port_char,"%d",input_port) < 0) { // sprintf(), If an output error is encountered, a negative value is returned.
		fprintf(stderr,F_ERROR_FUNCTION_SPRINTF_MSG);
		return EXIT_FAILURE;
	} else if (strcmp(input_port_char,argv[2]) != 0) { // Contain invalid chars
		printf(PORT_INVALID_MSG);
		return EXIT_FAILURE;
	}
	// Check output file
	if ((out_fd = _open(argv[3],O_WRONLY | O_CREAT | O_TRUNC,0777)) == -1) { // Upon successful completion, ... return a non-negative integer .... Otherwise, -1 shall be returned and errno set to indicate the error.
		strerror_s(errmsg, 255, errno);
		fprintf(stderr, F_ERROR_OUTPUT_FILE_MSG, argv[3], errmsg);// The path to the OUT file must exist, otherwise output an error and exit. (i.e., no need to check the folder, just try to open/create the file).
		return program_end(errno,out_fd,channel_fd); // If OUT does not exist, the client should create it. If it exists, it should truncate it.
	}
	// Create a TCP socket that listens on PORT (use 10 as the parameter for listen).
	if (DEBUG) {printf("FLAG 0\n");} // TODO XXX DEBUG
	if((channel_fd = socket(AF_INET,SOCK_STREAM,0)) == -1) { // On success, a file descriptor for the new socket is returned. On error, -1 is returned, and errno is set appropriately.
		strerror_s(errmsg, 255, errno);
		fprintf(stderr, F_ERROR_SOCKET_CREATE_MSG, errmsg);
		return program_end(errno,out_fd,channel_fd);
	}
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(input_port);
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);//htonl(INADDR_ANY); // INADDR_ANY = any local machine address
	if(bind(channel_fd,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) == -1) { // On success, zero is returned. On error, -1 is returned, and errno is set appropriately.
		strerror_s(errmsg, 255, errno);
		fprintf(stderr, F_ERROR_SOCKET_BIND_MSG, errmsg);
		return program_end(errno,out_fd,channel_fd);
	}
	if (DEBUG) {printf("FLAG 1\n");} // TODO XXX DEBUG
	if(listen(channel_fd,10) == -1) { // On success, zero is returned. On error, -1 is returned, and errno is set appropriately.
		strerror_s(errmsg, 255, errno);
		fprintf(stderr, F_ERROR_SOCKET_LISTEN_MSG, errmsg);
		return program_end(errno,out_fd,channel_fd);
	}
	// Wait for connection
	if((conn_fd = accept(channel_fd,NULL,NULL)) == -1) { // On success, these system calls return a nonnegative integer that is a descriptor for the accepted socket. On error, -1 is returned, and errno is set appropriately.
		strerror_s(errmsg, 255, errno);
		fprintf(stderr, F_ERROR_SOCKET_ACCEPT_MSG, errmsg);
		return program_end(errno,out_fd,channel_fd);
	}
	write_buf[0] = '\0';
	while(1) {
		if (DEBUG) {printf("FLAG 2\n");} // TODO XXX DEBUG
		// b. Read data from the client until EOF.
		// You cannot assume anything regarding the overall size of the input.
		if ((counter_client = _read(conn_fd,read_buf,MAX_BUF_THEORY)) == -1) { // On success, the number of bytes read is returned (zero indicates end of file), .... On error, -1 is returned, and errno is set appropriately.
			strerror_s(errmsg, 255, errno);
			fprintf(stderr, F_ERROR_SOCKET_READ_MSG, errmsg);
			return program_end(errno,out_fd,channel_fd);
		} else if (counter_client == 0) {
			break;
		}
		if (DEBUG) {printf("FLAG 3 - Receive %d chars\n",counter_client);} // TODO XXX DEBUG
		// Decode hamming code (63,57) // TODO
		// len(read_buf)  = counter_client;
		// len(bin_from)  = counter_client*8;
		// len(bin_to)    = counter_client*8 - padding;
		// len(write_buf) = counter_client   - padding/8;
		str2bin(read_buf,bin_from,counter_client);
		padding = 0;
		while ((padding+1)*HAMMING_TO <= counter_client*8) {
			strncpy(bin_to+(padding*HAMMING_FROM),bin_from+(padding*HAMMING_TO),HAMMING_FROM);
			padding += 1;
		}
		padding *= HAMMING_TO-HAMMING_FROM;
		bin2str(bin_to,write_buf,counter_client*8-padding);
		// writing the result (the client's data) into the output file OUT.
		if ((counter_dst = _write(out_fd,write_buf,counter_client-padding/8)) == -1) { // On success, the number of bytes written is returned (zero indicates nothing was written). On error, -1 is returned, and errno is set appropriately.
			strerror_s(errmsg, 255, errno);
			fprintf(stderr, F_ERROR_OUTPUT_WRITE_MSG, argv[3], errmsg);
			return program_end(errno,out_fd,channel_fd);
		}
		report[0] += counter_client; // received
		report[1] += counter_dst; // reconstructed/wrote
		report[2] += 1; // corrected // TODO
		if (DEBUG) {printf("FLAG 4 - Write %d chars - %.*s (%d in total)\n",counter_dst,counter_dst,write_buf,report[1]);} // TODO XXX DEBUG
	}
	if (DEBUG) {printf("FLAG 5 - Received shutdown()\n");} // TODO XXX DEBUG
	// When 'read socket' closed, Write the report to the channel
	if (sprintf(write_buf,"%d:%d:%d%c",report[0],report[1],report[2],'\0') < 0) { // sprintf(), If an output error is encountered, a negative value is returned.
		fprintf(stderr,F_ERROR_FUNCTION_SPRINTF_MSG);
		return program_end(errno,out_fd,channel_fd);
	}
	if (DEBUG) {printf("FLAG 6 - %s ~ %ld\n",write_buf,strlen(write_buf));} // TODO XXX DEBUG
	if ((counter_dst = _write(conn_fd,write_buf,strlen(write_buf))) == -1) { // On success, the number of bytes written is returned (zero indicates nothing was written). On error, -1 is returned, and errno is set appropriately.
		strerror_s(errmsg, 255, errno);
		fprintf(stderr, F_ERROR_SOCKET_WRITE_MSG, errmsg);
		return program_end(errno,out_fd,channel_fd);
	}
	// Print the report
	fprintf(stderr,REPORT_RECEIVER,report[0],report[1],report[2]);
	// Once all data is sent, and the client finished reading and handling any data received from the channel,
	// The client closes the connection and finishes.
	_close(conn_fd); // TODO TODO TODO
	return program_end(EXIT_SUCCESS,out_fd,channel_fd);
}
