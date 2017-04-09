#include <stdio.h>		// FILE, fopen, fclose, sprintf, printf, fwrite
#include <stdlib.h>		// malloc, free, NULL
#include <string.h>		// strcmp, strcpy
#include <errno.h>		// ERANGE, errno
#include <limits.h>		// LONG_MAX, LONG_MIN
#include <sys/stat.h>	// S_ISDIR, stat
#include <fcntl.h>		// O_RDONLY, O_WRONLY, O_CREAT, O_TRUNC, open
#include "my_library.h"	// validateIP4Dotted
#include <winsock2.h>	// windows sockets include
#include <stdint.h>
#define SEED_INV "seed parsing faild due to bad input or diffrent error \n"
#define ERRPR_P_MSG "error probebility parsing faild due to bad input or diffrent error \n"
#pragma comment(lib, "Ws2_32.lib") // some kind of an include in order to compile and link REFERANCE: http://stackoverflow.com/questions/16948064/unresolved-external-symbol-lnk2019

//
// Input (arguments):
// 1) The sender port
// 2) The receiver port
// 3) Error probability (float)
// 4) Random seed
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

int program_end(int error, int sender_fd, int reciver_fd, int sender_conn_fd, int reciver_conn_fd) {
	int res = 0;
	if ((0 < sender_fd) && (closesocket(sender_fd) == -1)) { // Upon successful completion, 0 shall be returned; otherwise, -1 shall be returned and errno set to indicate the error.
		fprintf(stderr, F_ERROR_SOCKET_CLOSE_MSG, strerror(errno));
		res = errno;
	}
	if ((0 < reciver_fd) && (closesocket(reciver_fd) == -1)) { // Upon successful completion, 0 shall be returned; otherwise, -1 shall be returned and errno set to indicate the error.
		fprintf(stderr, F_ERROR_SOCKET_CLOSE_MSG, strerror(errno));
		res = errno;
	}
	if ((0 < sender_conn_fd) && (closesocket(sender_conn_fd) == -1)) { // Upon successful completion, 0 shall be returned; otherwise, -1 shall be returned and errno set to indicate the error.
		fprintf(stderr, F_ERROR_SOCKET_CLOSE_MSG, strerror(errno));
		res = errno;
	}
	if ((0 < reciver_conn_fd) && (closesocket(reciver_conn_fd) == -1)) { // Upon successful completion, 0 shall be returned; otherwise, -1 shall be returned and errno set to indicate the error.
		fprintf(stderr, F_ERROR_SOCKET_CLOSE_MSG, strerror(errno));
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
	int i, check = 0;								// loop var, and errno check;					
	int send_to_recv = 1;							// sender to recver falg (type == int)
	int recv_to_send = 1;							// recver to sender falg (type == int)
	int sender_port = 0;							// The channels sender port (type == int)
	int reciver_port = 0;							// The channels reciver port (type == int)
	SOCKET sender_fd;								// The sender file descriptor (FD)
	SOCKET reciver_fd;								// The reciver file descriptor (FD)
	int Rand_seed = 0;								// The input randon seed (type == int)
	double random_var = 0;							// The random variable created by srand fuction (type == int)
	int num_bits_fliped = 0;						// count the number of flipped bits.
	float error_p = 0.0;							// The given error probabilty (type == float)
	struct sockaddr_in serv_addr_sender;			// The data structure for the sender // TODO set back to sender servadd
	struct sockaddr_in serv_addr_reciver;			// TODO COMPILATION ERROR - storage size of ‘serv_addr_reciver’ isn’t known	// The data structure for the reciver
	WORD wVersionRequested;							// win' sockets variables.
	WSADATA wsaData;								// win' sockets variables.
	int err;										// win' sockets variables.
	SOCKET sender_conn_fd = 0;						// The sender connection file 'file descriptor' (FD)
	int counter_input_sender = 0;					// The number of bytes we read from the sender
	int counter_input_reciver = 0;					// The number of bytes we read from the reciver
	char sender_port_char[6];						// The input port (type == string)
	char sender_read_buf[MAX_BUF_THEORY + 1];		// The string buffer got from the sender
	char reciver_read_buf[MAX_BUF_THEORY + 1];		// The string buffer got from the reciver
	char sender_bin_from[8 * MAX_BUF_THEORY + 1];	// The binary representation of 'sender_read_buf'
	char reciver_bin_from[8 * MAX_BUF_THEORY + 1];	// The binary representation of 'reciver_read_buf'
	char* sender_endptr_PORT;						// strtol() for 'sender_port'
	char senderIP[1024] = { 0 };
	char reciverIP[1024] = { 0 };
	// output variables (sender and reciver)
	SOCKET reciver_conn_fd = 0;						// The output connection file 'file descriptor' (FD)
	int counter_output_sender = 0;					// The number of bytes we send to the sender
	int counter_output_reciver = 0;					// The number of bytes we send to the reciver
	int total_counter = 0;							// The number of bytes we send in total.
	char reciver_port_char[6];						// The output port (type == string)
	char sender_write_buf[MAX_BUF_THEORY + 1];		// The string buffer send to the sender
	char reciver_write_buf[MAX_BUF_THEORY + 1];		// The string buffer send to the reciver
	char sender_bin_to[8 * MAX_BUF_THEORY + 1];		// The binary representation of 'sender_write_buf'
	char reciver_bin_to[8 * MAX_BUF_THEORY + 1];	// The binary representation of 'reciver_write_buf'
	char* reciver_endptr_PORT;						// strtol() for 'sender_input_port'
	// Init variables
	memset(sender_bin_from, '0', sizeof(sender_bin_from));
	memset(reciver_bin_from, '0', sizeof(reciver_bin_from));
	memset(sender_bin_to, '0', sizeof(sender_bin_to));
	memset(reciver_bin_to, '0', sizeof(reciver_bin_to));
	memset(sender_read_buf, '0', sizeof(sender_read_buf));
	memset(reciver_read_buf, '0', sizeof(reciver_read_buf));
	memset(sender_write_buf, '0', sizeof(sender_write_buf));
	memset(reciver_write_buf, '0', sizeof(reciver_write_buf));
	memset(sender_port_char, '0', sizeof(sender_port_char));
	memset(reciver_port_char, '0', sizeof(reciver_port_char));
	// Check correct call structure
	if (argc != 5) {
		if (argc < 5) {
			printf(USAGE_OPERANDS_MISSING_MSG, argv[0], " <SENDER PORT>", " <RECIVER PORT>", " <PROBABILITY>", " <SEED>");
		} else {
			printf(USAGE_OPERANDS_SURPLUS_MSG, argv[0], " <SENDER PORT>", " <RECIVER PORT>", " <PROBABILITY>", " <SEED>");
		}
		return EXIT_FAILURE;
	}
	// Check sender port
	sender_port = strtol(argv[1], &sender_endptr_PORT, 10); // If an underflow occurs. strtol() returns LONG_MIN. If an overflow occurs, strtol() returns LONG_MAX. In both cases, errno is set to ERANGE.
	if ((errno == ERANGE && (sender_port == (int)LONG_MAX || sender_port == (int)LONG_MIN)) || (errno != 0 && sender_port == 0)) {
		fprintf(stderr, F_ERROR_FUNCTION_STRTOL_MSG, strerror(errno));
		return errno;
	} else if ((sender_endptr_PORT == argv[1]) || (sender_port < 1) || (sender_port > 65535)) { // (Empty string) or (not in range [1,65535])
		printf(PORT_INVALID_MSG);
		return EXIT_FAILURE;
	} else if (sprintf(sender_port_char, "%d", sender_port) < 0) { // sprintf(), If an output error is encountered, a negative value is returned.
		fprintf(stderr, F_ERROR_FUNCTION_SPRINTF_MSG);
		return EXIT_FAILURE;
	} else if (strcmp(sender_port_char, argv[1]) != 0) { // Contain invalid chars
		printf(PORT_INVALID_MSG);
		return EXIT_FAILURE;
	}
	// Check reciver port
	reciver_port = strtol(argv[2], &reciver_endptr_PORT, 10); // If an underflow occurs. strtol() returns LONG_MIN. If an overflow occurs, strtol() returns LONG_MAX. In both cases, errno is set to ERANGE.
	if ((errno == ERANGE && (reciver_port == (int)LONG_MAX || reciver_port == (int)LONG_MIN)) || (errno != 0 && reciver_port == 0)) {
		fprintf(stderr, F_ERROR_FUNCTION_STRTOL_MSG, strerror(errno));
		return errno;
	} else if ((reciver_endptr_PORT == argv[2]) || (reciver_port < 1) || (reciver_port > 65535)) { // (Empty string) or (not in range [1,65535])
		printf(PORT_INVALID_MSG);
		return EXIT_FAILURE;
	} else if (sprintf(reciver_port_char, "%d", reciver_port) < 0) { // sprintf(), If an output error is encountered, a negative value is returned.
		fprintf(stderr, F_ERROR_FUNCTION_SPRINTF_MSG);
		return EXIT_FAILURE;
	} else if (strcmp(reciver_port_char, argv[2]) != 0) { // Contain invalid chars
		printf(PORT_INVALID_MSG);
		return EXIT_FAILURE;
	}
	// get seed and ratio from argv.
	Rand_seed = strtol(argv[4], NULL, 10);
	if (Rand_seed < 1) { // random seed value parsing fail
		printf(SEED_INV);
		return EXIT_FAILURE;
	}
	if (sscanf(argv[3], "%f", &error_p) != 1) { // parse error probabilty. 
		printf(ERRPR_P_MSG);
		return EXIT_FAILURE;
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
	/* Confirm that the WinSock DLL supports 2.2.*/
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
	// Create a TCP socket that listens on sender PORT (use 10 as the parameter for listen).
	if ((sender_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) { // On success, a file descriptor for the new socket is returned. On error, -1 is returned, and errno is set appropriately.
		fprintf(stderr, F_ERROR_SOCKET_CREATE_MSG, strerror(errno));
		return program_end(errno, sender_fd, NULL, sender_conn_fd, reciver_conn_fd);
	}
	// Create a TCP socket that listens on reciver PORT (use 10 as the parameter for listen).
	if ((reciver_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) { // On success, a file descriptor for the new socket is returned. On error, -1 is returned, and errno is set appropriately.
		fprintf(stderr, F_ERROR_SOCKET_CREATE_MSG, strerror(errno));
		return program_end(errno, sender_fd, reciver_fd, sender_conn_fd, reciver_conn_fd);
	}
	serv_addr_sender.sin_family = AF_INET;
	serv_addr_sender.sin_port = htons(sender_port);
	serv_addr_sender.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr_reciver.sin_family = AF_INET;
	serv_addr_reciver.sin_port = htons(reciver_port);
	serv_addr_reciver.sin_addr.s_addr = htonl(INADDR_ANY);
	//serv_addr.sin_addr.s_addr = inet_addr(argv[1]);//htonl(INADDR_ANY); // INADDR_ANY = any local machine address
	//sender bind
	if (bind(sender_fd, (struct sockaddr*)&serv_addr_sender, sizeof(serv_addr_sender)) == -1) { // On success, zero is returned. On error, -1 is returned, and errno is set appropriately.
		fprintf(stderr, F_ERROR_SOCKET_BIND_MSG, strerror(errno));
		return program_end(errno, sender_fd, reciver_fd, sender_conn_fd, reciver_conn_fd);
	}
	// reciver bind - //TODO is this the correct way to bind two sockets? YES!
	if (bind(reciver_fd, (struct sockaddr*)&serv_addr_reciver, sizeof(serv_addr_reciver)) == -1) { // On success, zero is returned. On error, -1 is returned, and errno is set appropriately.
		fprintf(stderr, F_ERROR_SOCKET_BIND_MSG, strerror(errno));
		return program_end(errno, sender_fd, reciver_fd, sender_conn_fd, reciver_conn_fd);
	}
	// sender listen
	fflush(NULL);
	if (listen(sender_fd, 10) == -1) { // On success, zero is returned. On error, -1 is returned, and errno is set appropriately.
		fprintf(stderr, F_ERROR_SOCKET_LISTEN_MSG, strerror(errno));
		return program_end(errno, sender_fd, reciver_fd, sender_conn_fd, reciver_conn_fd);
	}
	// reciver listen
	if (listen(reciver_fd, 10) == -1) { // On success, zero is returned. On error, -1 is returned, and errno is set appropriately.
		fprintf(stderr, F_ERROR_SOCKET_LISTEN_MSG, strerror(errno));
		return program_end(errno, sender_fd, reciver_fd, sender_conn_fd, reciver_conn_fd);
	}
	// TODO - move up to place 
	struct sockaddr_in tmp_reciver;
	struct sockaddr_in tmp_sender;
	// Wait for connection - reciver
	if ((reciver_conn_fd = accept(reciver_fd, (SOCKADDR*)&tmp_reciver, NULL)) == -1) { // On success, these system calls return a nonnegative integer that is a descriptor for the accepted socket. On error, -1 is returned, and errno is set appropriately.
		fprintf(stderr, F_ERROR_SOCKET_ACCEPT_MSG, ""/*TODO*/);
		return program_end(errno, sender_fd, reciver_fd, sender_conn_fd, reciver_conn_fd);
	}
	// Wait for connection - sender
	if ((sender_conn_fd = accept(sender_fd, (SOCKADDR*)&tmp_sender, NULL)) == -1) { // On success, these system calls return a nonnegative integer that is a descriptor for the accepted socket. On error, -1 is returned, and errno is set appropriately.
		fprintf(stderr, F_ERROR_SOCKET_ACCEPT_MSG, ""/*TODO*/);
		return program_end(errno, sender_fd, reciver_fd, sender_conn_fd, reciver_conn_fd);
	}
										 ///////////////////////////////////////////////////////////////////////
										 ///////////////////////////////////////////////////////////////////////
										 ///////////////////////                         ///////////////////////
										 ///////////////////////   channel's operation   ///////////////////////
										 ///////////////////////                         ///////////////////////
										 ///////////////////////////////////////////////////////////////////////
										 ///////////////////////////////////////////////////////////////////////

										 ////////////////////////////////////////////
										 //////////                        //////////
										 //////////   sender ---> reciver  //////////
										 //////////                        //////////
										 ////////////////////////////////////////////
	srand(Rand_seed);
	num_bits_fliped = 0;
	while (1) { // while sending.
		if ((counter_input_sender = recv(sender_conn_fd, sender_read_buf, MAX_BUF_THEORY, 0)) == -1) { // On success, the number of bytes read is returned (zero indicates end of file), .... On error, -1 is returned, and errno is set appropriately.
			fprintf(stderr, F_ERROR_SOCKET_READ_MSG, strerror(errno));
			return program_end(errno, sender_fd, reciver_fd, sender_conn_fd, reciver_conn_fd);
		} else if (counter_input_sender == 0) { // recived EOF. preper to land.
			if ((check = shutdown(reciver_conn_fd, SD_SEND)) == -1) { // close the connection with reciver to send direction only.  
				fprintf(stderr, F_ERROR_SOCKET_CLOSE_MSG, strerror(errno));
				return program_end(errno, sender_fd, reciver_fd, sender_conn_fd, reciver_conn_fd);
			}
			// leave reading loop
			break;
		}
		total_counter += counter_input_sender;
		sender_read_buf[counter_input_sender] = '\0'; // put an end to it. 
		// to bin
		str2bin(sender_read_buf, sender_bin_from, counter_input_sender);
		// turn over bytes in given ratio.
		// set random seed.
		// turn
		for (i = 0; i < counter_input_sender * 8; i++) {
			random_var = ((double)rand() / RAND_MAX);
			if (random_var <= error_p) {
				reciver_bin_to[i] = (sender_bin_from[i] ^ 0x1);
				num_bits_fliped++;
			} else {
				reciver_bin_to[i] = sender_bin_from[i];
			}
		}
		// back from bin
		bin2str(reciver_bin_to, reciver_write_buf, counter_input_sender * 8);
		// writing the result to reciver.
		if ((counter_output_reciver = send(reciver_conn_fd, reciver_write_buf, counter_input_sender, 0)) == -1) { // On success, the number of bytes written is returned (zero indicates nothing was written). On error, -1 is returned, and errno is set appropriately.
			fprintf(stderr, F_ERROR_SOCKET_WRITE_MSG, strerror(errno));
			return program_end(errno, sender_fd, reciver_fd, sender_conn_fd, reciver_conn_fd);
		}
	} // end of first while. 
	  ////////////////////////////////////////////
	  //////////                        //////////
	  //////////   sender <--- reciver  //////////
	  //////////                        //////////
	  ////////////////////////////////////////////
	if ((counter_input_reciver = recv(reciver_conn_fd, reciver_read_buf, MAX_BUF_THEORY, 0)) == -1) { // On success, the number of bytes read is returned (zero indicates end of file), .... On error, -1 is returned, and errno is set appropriately.
		fprintf(stderr, F_ERROR_SOCKET_READ_MSG, strerror(errno));
		return program_end(errno, sender_fd, reciver_fd, sender_conn_fd, reciver_conn_fd);
	}
	total_counter += counter_input_reciver;
	// to bin
	str2bin(reciver_read_buf, reciver_bin_from, counter_input_reciver);
	// turn over bytes in given ratio.
	// turn
	for (i = 0; i < counter_input_reciver * 8; i++) {
		sender_bin_to[i] = reciver_bin_from[i]; // in this diraction, no filiping gets done.
	}
	// back from bin
	bin2str(sender_bin_to, sender_write_buf, counter_input_reciver * 8);
	// writing the result to reciver.
	if ((counter_output_sender = send(sender_conn_fd, sender_write_buf, counter_input_reciver, 0)) == -1) { // On success, the number of bytes written is returned (zero indicates nothing was written). On error, -1 is returned, and errno is set appropriately.
		fprintf(stderr, F_ERROR_SOCKET_WRITE_MSG, strerror(errno));
		return program_end(errno, sender_fd, reciver_fd, sender_conn_fd, reciver_conn_fd);
	}
	//print the messege info 
	inet_ntop(AF_INET, &tmp_sender.sin_addr, senderIP, 1024);
	inet_ntop(AF_INET, &tmp_reciver.sin_addr, reciverIP, 1024);
	printf("sender: %s \n", senderIP);
	printf("reciver: %s \n",reciverIP);
	printf("%d bytes fliped %d bits\n", total_counter, num_bits_fliped);
	//close rammaing fd and so on.
	return program_end(EXIT_SUCCESS, sender_fd, reciver_fd, sender_conn_fd, reciver_conn_fd);;
}
