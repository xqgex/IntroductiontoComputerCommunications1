#define _GNU_SOURCE
#include <errno.h>	// ERANGE, errno
#include <stdio.h>	// FILE, printf, fprintf, sprintf, stderr, sscanf, fopen, fclose, fwrite
#include <stdlib.h>	// EXIT_FAILURE, EXIT_SUCCESS, NULL, strtol, malloc, free
#include <string.h>	// strlen, strcmp, strcpy, strerror, memset
#include <unistd.h>	// F_OK, R_OK, lseek, close, read, write
#include <limits.h>	// LONG_MAX, LONG_MIN
#include <sys/stat.h>	// S_ISDIR, stat
#include <fcntl.h>	// O_RDONLY, O_WRONLY, O_CREAT, O_TRUNC, open
#include <arpa/inet.h> 	// AF_INET, SOCK_STREAM, socket, htons, inet_addr, connect, sockaddr_in
#include "my_library.h"	// validateIP4Dotted

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
	int res = 0;
	if ((0 < in_fd)&(close(in_fd) == -1)) { // Upon successful completion, 0 shall be returned; otherwise, -1 shall be returned and errno set to indicate the error.
		fprintf(stderr,F_ERROR_INPUT_CLOSE_MSG,strerror(errno));
		res = errno;
	}
	if ((0 < sock_fd)&&(close(sock_fd) == -1)) { // Upon successful completion, 0 shall be returned; otherwise, -1 shall be returned and errno set to indicate the error.
		fprintf(stderr,F_ERROR_SOCKET_CLOSE_MSG,strerror(errno));
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
	int indx = 0;					// TODO XXX
	int input_port = 0;				// The channel port (type == int)
	int in_fd = 0;					// The input file 'file descriptor' (FD)
	int sock_fd = 0;				// The socket file descriptor (FD)
	int counter_dst = 0;				// The number of bytes we wrote to the channel
	int counter_src = 0;				// The number of bytes we read from input
	int padding = 0;				// The amount of padding chars that been have added
	char read_buf[MAX_BUF+1];			// The string buffer readed from the input
	char write_buf[MAX_BUF_THEORY+1];		// The string buffer to be sent to the channel
	char input_port_char[6];			// The channel port (type == string)
	char* endptr_PORT;				// strtol() for 'input_port'
	char report[][10] = {"","",""};			// The 3 values for the report
	char *report_str,*report_token,*report_saveptr;	// Variables to split the string {Ex. report = input.split(':',3)}
	struct sockaddr_in serv_addr;			// The data structure for the channel
	struct stat in_stat;				// The data structure for the input file
	// Init variables
	memset(read_buf,'0',sizeof(read_buf));
	memset(write_buf,'0',sizeof(write_buf));
	memset(&serv_addr,'0',sizeof(serv_addr));
	// Check correct call structure
	if (argc != 4) {
		if (argc < 4) {
			printf(USAGE_OPERANDS_MISSING_MSG,argv[0],"IN");
		} else {
			printf(USAGE_OPERANDS_SURPLUS_MSG,argv[0],"IN");
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
		fprintf(stderr,F_ERROR_FUNCTION_STRTOL_MSG,strerror(errno));
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
	// Check input file - ||We can not assume anything regarding the size of IN.||
	if ((in_fd = open(argv[3],O_RDONLY)) == -1) { // Upon successful completion, ... return a non-negative integer .... Otherwise, -1 shall be returned and errno set to indicate the error.
		fprintf(stderr,F_ERROR_INPUT_FILE_MSG,argv[3],strerror(errno)); // IN must exist, otherwise output an error and exit.
		return program_end(errno,in_fd,sock_fd);
	} else if (stat(argv[3],&in_stat) == -1) { // On success, zero is returned. On error, -1 is returned, and errno is set appropriately.
		fprintf(stderr,F_ERROR_INPUT_OPEN_MSG,argv[3],strerror(errno));
		return program_end(errno,in_fd,sock_fd);
	} else if (S_ISDIR(in_stat.st_mode)) {
		fprintf(stderr,F_ERROR_INPUT_IS_FOLDER_MSG,argv[3]);
		return program_end(-1,in_fd,sock_fd);
	} else if (lseek(in_fd,SEEK_SET,0) == -1) { // Upon successful completion, lseek() returns the resulting offset ... from the beginning of the file. On error, the value (off_t) -1 is returned and errno is set to indicate the error.
		fprintf(stderr,F_ERROR_FUNCTION_LSEEK_MSG,strerror(errno));
		return program_end(errno,in_fd,sock_fd);
	}
	// Open connection to the channel // Data should be sent via a TCP socket, to the channel specified by IP:PORT.
	if((sock_fd = socket(AF_INET,SOCK_STREAM,0)) == -1) { // On success, a file descriptor for the new socket is returned. On error, -1 is returned, and errno is set appropriately.
		fprintf(stderr,F_ERROR_SOCKET_CREATE_MSG,strerror(errno));
		return program_end(errno,in_fd,sock_fd);
	}
	if (DEBUG) {printf("FLAG 0\n");} // TODO XXX DEBUG
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(input_port);
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	if (connect(sock_fd,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) == -1) { // Upon successful completion, connect() shall return 0; otherwise, -1 shall be returned and errno set to indicate the error.
		fprintf(stderr,F_ERROR_SOCKET_CONNECT_MSG,strerror(errno));
		return program_end(errno,in_fd,sock_fd);
	}
	if (DEBUG) {printf("FLAG 1\n");} // TODO XXX DEBUG
	write_buf[0] = '\0';
	// All inputs variables are valid, Stsrt working
	while (1) {
		if (DEBUG) {printf("FLAG 2\n");} // TODO XXX DEBUG
		// Read input file IN
		if ((counter_src = read(in_fd,read_buf,MAX_BUF)) == -1) { // On success, the number of bytes read is returned (zero indicates end of file), .... On error, -1 is returned, and errno is set appropriately.
			fprintf(stderr,F_ERROR_INPUT_READ_MSG,argv[3],strerror(errno));
			return program_end(errno,in_fd,sock_fd);
		} else if (counter_src == 0) { // Received EOF, Exit the infinity loop
			break;
		}
		// Calc hamming code (63,57) // TODO
		padding = 0;
		while ((padding+1)*HAMMING_FROM <= counter_src) {
			strncpy(write_buf+(padding*HAMMING_TO),read_buf+(padding*HAMMING_FROM),HAMMING_FROM);
			for (indx=HAMMING_FROM;indx<HAMMING_TO;indx++) { // TODO
				write_buf[(padding*HAMMING_TO)+indx] = '0'; // TODO
			} // TODO
			padding += 1;
		}
		padding *= HAMMING_TO-HAMMING_FROM;
		if (DEBUG) {printf("FLAG 3 - Read %d chars - %.*s\n",counter_src,counter_src,read_buf);} // TODO XXX DEBUG
		// Sending data from the input file IN to the server,
		if ((counter_dst = write(sock_fd,write_buf,counter_src+padding)) == -1) { // On success, the number of bytes written is returned (zero indicates nothing was written). On error, -1 is returned, and errno is set appropriately.
			fprintf(stderr,F_ERROR_SOCKET_WRITE_MSG,strerror(errno));
			return program_end(errno,in_fd,sock_fd);
		} else if (counter_dst == 0) {
			break;
		}
		if (DEBUG) {printf("FLAG 4 - Send %d chars - %.*s\n",counter_dst,counter_dst,write_buf);} // TODO XXX DEBUG
	}
	// Close write socket // TODO TODO TODO
	if (shutdown(sock_fd,SHUT_WR) == -1) { // Upon successful completion, shutdown() shall return 0; otherwise, -1 shall be returned and errno set to indicate the error.
		fprintf(stderr,F_ERROR_SOCKET_SHUTDOWN_MSG,strerror(errno));
		return program_end(errno,in_fd,sock_fd);
	}
	// Wait and print the response
	strcpy(read_buf,"-1:-1:-1");
	if ((counter_dst = read(sock_fd,read_buf,MAX_BUF)) == -1) { // On success, the number of bytes read is returned (zero indicates end of file), .... On error, -1 is returned, and errno is set appropriately.
		fprintf(stderr,F_ERROR_SOCKET_READ_MSG,strerror(errno));
		return program_end(errno,in_fd,sock_fd);
	}
	for (report_str=read_buf,indx=0;indx<3;report_str=NULL,indx++) {
		report_token = strtok_r(report_str, ":", &report_saveptr);
		if (report_token == NULL) {
			break;
		}
		strcpy(report[indx],report_token);
	}
	fprintf(stderr,REPORT_SENDER,report[0],report[1],report[2]);
	// Free resources!!! and exit // TODO TODO TODO
	// Once all data is sent, and the client finished reading and handling any data received from the channel,
	// The client closes the connection and finishes.
	return program_end(EXIT_SUCCESS,in_fd,sock_fd);
}
