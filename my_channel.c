#include <stdio.h>	// FILE, fopen, fclose, sprintf, printf, fwrite
#include <stdlib.h>	// malloc, free, NULL
#include <string.h>	// strcmp, strcpy
#include <unistd.h>	// F_OK, R_OK
#include <errno.h>	// ERANGE, errno
#include <limits.h>	// LONG_MAX, LONG_MIN
#include <sys/stat.h>	// S_ISDIR, stat
#include <fcntl.h>	// O_RDONLY, O_WRONLY, O_CREAT, O_TRUNC, open
#include <arpa/inet.h> 	// AF_INET, SOCK_STREAM, socket, htons, inet_addr, connect, sockaddr_in
#include "my_library.h"	// validateIP4Dotted

//
// Input (arguments):
// 1) The sender port
// 2) The receiver port
// 3) Error probability
// 4) Random seed
//
//
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
// yuval: new clone test push.

int program_end(int error, int sender_fd, int reciver_fd, int sender_conn_fd, int reciver_conn_fd) {
	int res = 0;

	if ((0 < sender_fd)&&(close(sender_fd) == -1)) { // Upon successful completion, 0 shall be returned; otherwise, -1 shall be returned and errno set to indicate the error.
		fprintf(stderr,F_ERROR_SOCKET_CLOSE_MSG,strerror(errno));
		res = errno;
	}
	if ((0 < reciver_fd)&&(close(reciver_fd) == -1)) { // Upon successful completion, 0 shall be returned; otherwise, -1 shall be returned and errno set to indicate the error.
		fprintf(stderr,F_ERROR_SOCKET_CLOSE_MSG,strerror(errno));
		res = errno;
	}
	if ((0 < sender_conn_fd)&&(close(sender_conn_fd) == -1)) { // Upon successful completion, 0 shall be returned; otherwise, -1 shall be returned and errno set to indicate the error.
		fprintf(stderr,F_ERROR_SOCKET_CLOSE_MSG,strerror(errno));
		res = errno;
	}
	if ((0 < reciver_conn_fd)&&(close(reciver_conn_fd) == -1)) { // Upon successful completion, 0 shall be returned; otherwise, -1 shall be returned and errno set to indicate the error.
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

	/// Function variables///
	int sender_port = 0;				// The channels sender port (type == int)
	int reciver_port = 0;				// The channels reciver port (type == int)
	int sender_fd = 0;				// The sender file descriptor (FD)	
	int reciver_fd = 0;				// The reciver file descriptor (FD)
	struct sockaddr_in serv_addr_sender;		// The data structure for the sender
	struct sockaddr_out serv_addr_reciver;		// The data structure for the reciver

	// input variables (sender and reciver)
	int sender_conn_fd = 0;				// The sender connection file 'file descriptor' (FD)
	int counter_input_sender = 0;			// The number of bytes we read from the sender
	int counter_input_reciver = 0;			// The number of bytes we read from the reciver
	char sender_port_char[6];			// The input port (type == string)
	char sender_read_buf[MAX_BUF_THEORY+1];		// The string buffer got from the sender
	char reciver_read_buf[MAX_BUF_THEORY+1];	// The string buffer got from the reciver
	char sender_bin_from[8*MAX_BUF_THEORY+1];	// The binary representation of 'sender_read_buf'
	char reciver_bin_from[8*MAX_BUF_THEORY+1];	// The binary representation of 'reciver_read_buf'
	char* sender_endptr_PORT;			// strtol() for 'sender_port'
		
	// output variables (sender and reciver)
	int reciver_conn_fd = 0;			// The output connection file 'file descriptor' (FD)
	int counter_output_sender = 0;			// The number of bytes we send to the sender
	int counter_output_reciver = 0;			// The number of bytes we send to the reciver
	char reciver_port_char[6];			// The output port (type == string)
	char sender_write_buf[MAX_BUF_THEORY+1];	// The string buffer send to the sender
	char reciver_write_buf[MAX_BUF_THEORY+1];	// The string buffer send to the reciver
	char sender_bin_to[8*MAX_BUF_THEORY+1];		// The binary representation of 'sender_write_buf'
	char reciver_bin_to[8*MAX_BUF_THEORY+1];	// The binary representation of 'reciver_write_buf'
	char* reciver_endptr_PORT;			// strtol() for 'sender_input_port'
	
	// Init variables
	memset(sender_bin_from,'0',sizeof(sender_bin_from));
	memset(reciver_bin_from,'0',sizeof(reciver_bin_from));
	memset(sender_bin_to,'0',sizeof(sender_bin_to));
	memset(reciver_bin_to,'0',sizeof(reciver_bin_to));
	memset(sender_read_buf,'0',sizeof(sender_read_buf));
	memset(reciver_read_buf,'0',sizeof(reciver_read_buf));
	memset(sender_write_buf,'0',sizeof(sender_write_buf));
	memset(reciver_write_buf,'0',sizeof(reciver_write_buf));
	memset(sender_port_char,'0',sizeof(input_port_char));
	memset(reciver_port_char,'0',sizeof(output_port_char));
	
	// Check correct call structure
	if (argc != 4) {
		if (argc < 4) {
			printf(USAGE_OPERANDS_MISSING_MSG,argv[0],"OUT");
		} else {
			printf(USAGE_OPERANDS_SURPLUS_MSG,argv[0],"OUT");
		}
		return EXIT_FAILURE;
	}
	// Check sender port
	sender_port = strtol(argv[1],&sender_endptr_PORT,10); // If an underflow occurs. strtol() returns LONG_MIN. If an overflow occurs, strtol() returns LONG_MAX. In both cases, errno is set to ERANGE.
	if ((errno == ERANGE && (sender_port == (int)LONG_MAX || input_port == (int)LONG_MIN)) || (errno != 0 && input_port == 0)) {
		fprintf(stderr,F_ERROR_FUNCTION_STRTOL_MSG,strerror(errno));
		return errno;
	} else if ( (sender_endptr_PORT == argv[1])||(sender_port < 1)||(sender_port > 65535) ) { // (Empty string) or (not in range [1,65535])
		printf(PORT_INVALID_MSG);
		return EXIT_FAILURE;
	} else if (sprintf(sender_port_char,"%d",sender_port) < 0) { // sprintf(), If an output error is encountered, a negative value is returned.
		fprintf(stderr,F_ERROR_FUNCTION_SPRINTF_MSG);
		return EXIT_FAILURE;
	} else if (strcmp(sender_port_char,argv[1]) != 0) { // Contain invalid chars
		printf(PORT_INVALID_MSG);
		return EXIT_FAILURE;
	}
	
	
	// Check reciver port
	reciver_port = strtol(argv[2],&reciver_endptr_PORT,10); // If an underflow occurs. strtol() returns LONG_MIN. If an overflow occurs, strtol() returns LONG_MAX. In both cases, errno is set to ERANGE.
	if ((errno == ERANGE && (reciver_port == (int)LONG_MAX || output_port == (int)LONG_MIN)) || (errno != 0 && input_port == 0)) {
		fprintf(stderr,F_ERROR_FUNCTION_STRTOL_MSG,strerror(errno));
		return errno;
	} else if ( (endptr_PORT == argv[2])||(reciver_port < 1)||(reciver_port > 65535) ) { // (Empty string) or (not in range [1,65535])
		printf(PORT_INVALID_MSG);
		return EXIT_FAILURE;
	} else if (sprintf(reciver_port_char,"%d",reciver_port) < 0) { // sprintf(), If an output error is encountered, a negative value is returned.
		fprintf(stderr,F_ERROR_FUNCTION_SPRINTF_MSG);
		return EXIT_FAILURE;
	} else if (strcmp(reciver_port_char,argv[2]) != 0) { // Contain invalid chars
		printf(PORT_INVALID_MSG);
		return EXIT_FAILURE;
	}
	
	// Create a TCP socket that listens on sender PORT (use 10 as the parameter for listen).
	if (DEBUG) {printf("FLAG 0\n");} // TODO XXX DEBUG - and sender port number
	if((sender_fd = socket(AF_INET,SOCK_STREAM,0)) == -1) { // On success, a file descriptor for the new socket is returned. On error, -1 is returned, and errno is set appropriately.
		fprintf(stderr,F_ERROR_SOCKET_CREATE_MSG,strerror(errno));
		return program_end(errno,sender_fd,reciver_fd,sender_conn_fd,reciver_conn_fd);
	}
	
	// Create a TCP socket that listens on reciver PORT (use 10 as the parameter for listen).
	if (DEBUG) {printf("FLAG 0\n");} // TODO XXX DEBUG - and reciver port number
	if((reciver_fd = socket(AF_INET,SOCK_STREAM,0)) == -1) { // On success, a file descriptor for the new socket is returned. On error, -1 is returned, and errno is set appropriately.
		fprintf(stderr,F_ERROR_SOCKET_CREATE_MSG,strerror(errno));
		return program_end(errno,sender_fd,reciver_fd,sender_conn_fd,reciver_conn_fd);
	}
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(input_port);
	//serv_addr.sin_addr.s_addr = inet_addr(argv[1]);//htonl(INADDR_ANY); // INADDR_ANY = any local machine address
	
	//sender bind
	if(bind(sender_fd,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) == -1) { // On success, zero is returned. On error, -1 is returned, and errno is set appropriately.
		fprintf(stderr,F_ERROR_SOCKET_BIND_MSG,strerror(errno));
		return program_end(errno,sender_fd,reciver_fd,sender_conn_fd,reciver_conn_fd);
	}
	// reciver bind - //TODO is this the correct way to bind two sockets?
	if(bind(reciver_fd,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) == -1) { // On success, zero is returned. On error, -1 is returned, and errno is set appropriately.
		fprintf(stderr,F_ERROR_SOCKET_BIND_MSG,strerror(errno));
		return program_end(errno,sender_fd,reciver_fd,sender_conn_fd,reciver_conn_fd);
	}
	// sender listen
	if (DEBUG) {printf("FLAG 1\n");} // TODO XXX DEBUG
	if(listen(sender_fd,10) == -1) { // On success, zero is returned. On error, -1 is returned, and errno is set appropriately.
		fprintf(stderr,F_ERROR_SOCKET_LISTEN_MSG,strerror(errno));
		return program_end(errno,sender_fd,reciver_fd,sender_conn_fd,reciver_conn_fd);
	}
	// reciver listen
	if (DEBUG) {printf("FLAG 1\n");} // TODO XXX DEBUG
	if(listen(reciver_fd,10) == -1) { // On success, zero is returned. On error, -1 is returned, and errno is set appropriately.
		fprintf(stderr,F_ERROR_SOCKET_LISTEN_MSG,strerror(errno));
		return program_end(errno,sender_fd,reciver_fd,sender_conn_fd,reciver_conn_fd);
	}
	
	// Wait for connection - sender
	if((sender_conn_fd = accept(sender_fd,NULL,NULL)) == -1) { // On success, these system calls return a nonnegative integer that is a descriptor for the accepted socket. On error, -1 is returned, and errno is set appropriately.
		fprintf(stderr,F_ERROR_SOCKET_ACCEPT_MSG,strerror(errno));
		return program_end(errno,sender_fd,reciver_fd,sender_conn_fd,reciver_conn_fd);
	}
	
	// Wait for connection - reciver
	if((reciver_conn_fd = accept(reciver_fd,NULL,NULL)) == -1) { // On success, these system calls return a nonnegative integer that is a descriptor for the accepted socket. On error, -1 is returned, and errno is set appropriately.
		fprintf(stderr,F_ERROR_SOCKET_ACCEPT_MSG,strerror(errno));
		return program_end(errno,sender_fd,reciver_fd,sender_conn_fd,reciver_conn_fd);
	}
	
	///////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////
	///////////////////////                         ///////////////////////
	///////////////////////   channel's operation   ///////////////////////
	///////////////////////   (to be concluded..)   ///////////////////////
	///////////////////////                         ///////////////////////
	///////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////

	
//	if (argc == 5) {
//		printf("%s %s %s %s %s\n",argv[0],argv[1],argv[2],argv[3],argv[4]);
//	}
//}





