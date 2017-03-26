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


//MSG def // TODO move to headr in time
#define SEED_INV "seed parsing faild due to bad input or diffrent error"
#define ERRPR_P_MSG "error probebility parsing faild due to bad input or diffrent error"

//
// Input (arguments):
// 1) The sender port
// 2) The receiver port
// 3) Error probability (float)
// 4) Random seed
//
//
//
//
// Flow:
// 1) Open two TCP sockets								// not ready
// 2) When recieving data fron the sender						// ready
// 3) Flip bits with the input probability and pass it to the receiver			// ready
// 4) When the sender close the socket, Close the socket to the receiver		// not ready
// 5) When recieve data from the receiver, Transfer it to the sender			// ready
// 6) Close sockets to the sender and the receiver					// not ready
// 7) Print the statistics								// not ready
// 8) Free resources and exit								// not ready
//
// Examples:										// not ready
// > my_channel 6342 6343 0.001 12345
// sender: 132.66.48.12
// receiver: 132.66.50.12
// 1457 bytes flipped 13 bits
//

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
int main(int argc, char *argv[]) {
	/// Function variables///
	int i;
	int send_to_recv = 1;			// sender to recver falg (type == int)
	int recv_to_send = 1;			// recver to sender falg (type == int)
	int sender_port = 0;			// The channels sender port (type == int)
	int reciver_port = 0;			// The channels reciver port (type == int)
	int sender_fd = 0;			// The sender file descriptor (FD)
	int reciver_fd = 0;			// The reciver file descriptor (FD)
	int Rand_seed = 0;			// The input randon seed (type == int)
	int random_var = 0;			// The random variable created by srand fuction (type == int)
	int num_bits_fliped = 0;		// count the number of flipped bits.
	float error_p = 0.0;			// The given error probabilty (type == float)
	struct sockaddr_in serv_addr;		// The data structure for the sender // TODO set back to sender servadd
	struct sockaddr_out serv_addr_reciver;	// TODO COMPILATION ERROR - storage size of ‘serv_addr_reciver’ isn’t known	// The data structure for the reciver
	// input variables (sender and reciver)
	int sender_conn_fd = 0;				// The sender connection file 'file descriptor' (FD)
	int counter_input_sender = 0;			// The number of bytes we read from the sender
	int counter_input_reciver = 0;			// The number of bytes we read from the reciver
	char sender_port_char[6];			// The input port (type == string)
	char sender_read_buf[MAX_BUF_THEORY + 1];	// The string buffer got from the sender
	char reciver_read_buf[MAX_BUF_THEORY + 1];	// The string buffer got from the reciver
	char sender_bin_from[8 * MAX_BUF_THEORY + 1];	// The binary representation of 'sender_read_buf'
	char reciver_bin_from[8 * MAX_BUF_THEORY + 1];	// The binary representation of 'reciver_read_buf'
	char* sender_endptr_PORT;			// strtol() for 'sender_port'
	// output variables (sender and reciver)
	int reciver_conn_fd = 0;			// The output connection file 'file descriptor' (FD)
	int counter_output_sender = 0;			// The number of bytes we send to the sender
	int counter_output_reciver = 0;			// The number of bytes we send to the reciver
	char reciver_port_char[6];			// The output port (type == string)
	char sender_write_buf[MAX_BUF_THEORY + 1];	// The string buffer send to the sender
	char reciver_write_buf[MAX_BUF_THEORY + 1];	// The string buffer send to the reciver
	char sender_bin_to[8 * MAX_BUF_THEORY + 1];	// The binary representation of 'sender_write_buf'
	char reciver_bin_to[8 * MAX_BUF_THEORY + 1];	// The binary representation of 'reciver_write_buf'
	char* reciver_endptr_PORT;			// strtol() for 'sender_input_port'
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
	if (argc != 4) {
		if (argc < 4) {
			printf(USAGE_OPERANDS_MISSING_MSG, argv[0], "OUT");
		} else {
			printf(USAGE_OPERANDS_SURPLUS_MSG, argv[0], "OUT");
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
	Rand_seed = strtol(argv[4], NULL , 10);
	if (Rand_seed < 1) { // random seed value parsing fail
		printf(SEED_INV);
		return EXIT_FAILURE;
	}
	error_p = strtof(argv[3], NULL, 10); // TODO COMPILATION ERROR - too many arguments to function ‘strtof’
	if (error_p < 1) { // random seed value parsing fail
		printf(ERRPR_P_MSG);
		return EXIT_FAILURE;
	}
	// Create a TCP socket that listens on sender PORT (use 10 as the parameter for listen).
	if (DEBUG) { printf("FLAG 0\n"); } // TODO XXX DEBUG - and sender port number
	if ((sender_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) { // On success, a file descriptor for the new socket is returned. On error, -1 is returned, and errno is set appropriately.
		fprintf(stderr, F_ERROR_SOCKET_CREATE_MSG, strerror(errno));
		return program_end(errno, sender_fd, reciver_fd, sender_conn_fd, reciver_conn_fd);
	}
	// Create a TCP socket that listens on reciver PORT (use 10 as the parameter for listen).
	if (DEBUG) { printf("FLAG 1\n"); } // TODO XXX DEBUG - and reciver port number
	if ((reciver_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) { // On success, a file descriptor for the new socket is returned. On error, -1 is returned, and errno is set appropriately.
		fprintf(stderr, F_ERROR_SOCKET_CREATE_MSG, strerror(errno));
		return program_end(errno, sender_fd, reciver_fd, sender_conn_fd, reciver_conn_fd);
	}
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(sender_port);
	serv_addr_reciver.sin_family = AF_INET;
	serv_addr_reciver.sin_port = htons(reciver_port);
	//serv_addr.sin_addr.s_addr = inet_addr(argv[1]);//htonl(INADDR_ANY); // INADDR_ANY = any local machine address
	//sender bind
	if (bind(sender_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) { // On success, zero is returned. On error, -1 is returned, and errno is set appropriately.
		fprintf(stderr, F_ERROR_SOCKET_BIND_MSG, strerror(errno));
		return program_end(errno, sender_fd, reciver_fd, sender_conn_fd, reciver_conn_fd);
	}
	// reciver bind - //TODO is this the correct way to bind two sockets?
	if (bind(reciver_fd, (struct sockaddr*)&serv_addr_reciver, sizeof(serv_addr_reciver)) == -1) { // On success, zero is returned. On error, -1 is returned, and errno is set appropriately.
		fprintf(stderr, F_ERROR_SOCKET_BIND_MSG, strerror(errno));
		return program_end(errno, sender_fd, reciver_fd, sender_conn_fd, reciver_conn_fd);
	}
	// sender listen
	if (DEBUG) { printf("FLAG 2\n"); } // TODO XXX DEBUG
	if (listen(sender_fd, 10) == -1) { // On success, zero is returned. On error, -1 is returned, and errno is set appropriately.
		fprintf(stderr, F_ERROR_SOCKET_LISTEN_MSG, strerror(errno));
		return program_end(errno, sender_fd, reciver_fd, sender_conn_fd, reciver_conn_fd);
	}
	// reciver listen
	if (DEBUG) { printf("FLAG 3\n"); } // TODO XXX DEBUG
	if (listen(reciver_fd, 10) == -1) { // On success, zero is returned. On error, -1 is returned, and errno is set appropriately.
		fprintf(stderr, F_ERROR_SOCKET_LISTEN_MSG, strerror(errno));
		return program_end(errno, sender_fd, reciver_fd, sender_conn_fd, reciver_conn_fd);
	}
	// Wait for connection - sender
	if ((sender_conn_fd = accept(sender_fd, NULL, NULL)) == -1) { // On success, these system calls return a nonnegative integer that is a descriptor for the accepted socket. On error, -1 is returned, and errno is set appropriately.
		fprintf(stderr, F_ERROR_SOCKET_ACCEPT_MSG, strerror(errno));
		return program_end(errno, sender_fd, reciver_fd, sender_conn_fd, reciver_conn_fd);
	}
	// Wait for connection - reciver
	if ((reciver_conn_fd = accept(reciver_fd, NULL, NULL)) == -1) { // On success, these system calls return a nonnegative integer that is a descriptor for the accepted socket. On error, -1 is returned, and errno is set appropriately.
		fprintf(stderr, F_ERROR_SOCKET_ACCEPT_MSG, strerror(errno));
		return program_end(errno, sender_fd, reciver_fd, sender_conn_fd, reciver_conn_fd);
	}
	///////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////
	///////////////////////                         ///////////////////////
	///////////////////////   channel's operation   ///////////////////////
	///////////////////////   (to be concluded..)   ///////////////////////
	///////////////////////                         ///////////////////////
	///////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////
	while (recv_to_send || send_to_recv) { // while at list one of the flags is up
		////////////////////////////////////////////
		//////////                        //////////
		//////////   sender ---> reciver  //////////
		//////////                        //////////
		////////////////////////////////////////////
		if (send_to_recv) {
			if (DEBUG) { printf(">My_channel %s %s %s %s %s\n", argv[0], argv[1], argv[2], argv[3], argv[4]); } // TODO XXX DEBUG
			if (DEBUG) { printf("sender: %s ", sender_ip ); } // TODO COMPILATION ERROR - ‘sender_ip’ undeclared (first use in this function) // TODO XXX DEBUG
			if (DEBUG) { printf("reciver: %s ", reciver_ip); } // TODO COMPILATION ERROR - ‘reciver_ip’ undeclared (first use in this function) // TODO XXX DEBUG
			// b. Read data from the client until EOF.
			// You cannot assume anything regarding the overall size of the input.
			if ((counter_input_sender = read(sender_conn_fd, sender_read_buf, MAX_BUF_THEORY)) == -1) { // On success, the number of bytes read is returned (zero indicates end of file), .... On error, -1 is returned, and errno is set appropriately.
				fprintf(stderr, F_ERROR_SOCKET_READ_MSG, strerror(errno));
				return program_end(errno, sender_fd, reciver_fd, sender_conn_fd, reciver_conn_fd);
			} else if (counter_input_sender == 0) { // recived EOF. preper to land.
				close_one_way; // TODO COMPILATION ERROR - ‘close_one_way’ undeclared (first use in this function) // TODO XXX
				send_to_recv = 0; // close read loop.
				continue;
			}
			// to bin
			str2bin(sender_read_buf, sender_bin_from, counter_input_sender);
			// turn over bytes in given ratio.
			// set random seed.
			srand(Rand_seed);
			// turn
			num_bits_fliped = 0;
			for (i = 0; i < counter_input_sender; i++) {
				random_var = (rand() % 1000);
				if (random_var <= (int)(error_p * 1000)) {
					reciver_bin_to[i] = !sender_bin_from[i];
					num_bits_fliped++;
				} else {
					reciver_bin_to[i] = sender_bin_from[i];
				}
			}
			// back from bin
			bin2str(reciver_bin_to, reciver_write_buf, counter_input_sender);
			// writing the result to reciver.
			if ((counter_output_reciver = write(reciver_conn_fd, reciver_write_buf, counter_input_sender)) == -1) { // On success, the number of bytes written is returned (zero indicates nothing was written). On error, -1 is returned, and errno is set appropriately.
				fprintf(stderr, F_ERROR_SOCKET_WRITE_MSG, strerror(errno));
				return program_end(errno, sender_fd, reciver_fd, sender_conn_fd, reciver_conn_fd);
			}
			if (DEBUG) { printf("FLAG 4 - messege length %d bytes\n", counter_output_reciver); } // TODO XXX DEBUG
			if (DEBUG) { printf("FLAG 5 - fliped %d bits\n", num_bits_fliped); } // TODO XXX DEBUG
		}
		////////////////////////////////////////////
		//////////                        //////////
		//////////   sender <--- reciver  //////////
		//////////                        //////////
		////////////////////////////////////////////
		if (recv_to_send) {
			if (DEBUG) { printf(">My_channel %s %s %s %s %s\n", argv[0], argv[1], argv[2], argv[3], argv[4]); } // TODO XXX DEBUG
			if (DEBUG) { printf("sender: %s ", reciver_ip); } // TODO XXX DEBUG
			if (DEBUG) { printf("reciver: %s ", sender_ip); } // TODO XXX DEBUG
			// b. Read data from the client until EOF.
			// You cannot assume anything regarding the overall size of the input.
			if ((counter_input_reciver = read(reciver_conn_fd, reciver_read_buf, MAX_BUF_THEORY)) == -1) { // On success, the number of bytes read is returned (zero indicates end of file), .... On error, -1 is returned, and errno is set appropriately.
				fprintf(stderr, F_ERROR_SOCKET_READ_MSG, strerror(errno));
				return program_end(errno, sender_fd, reciver_fd, sender_conn_fd, reciver_conn_fd);
			} else if (counter_input_reciver == 0) { // recived EOF. preper to land.
				close_second_way; // TODO COMPILATION ERROR - ‘close_second_way’ undeclared (first use in this function) // TODO XXX
				recv_to_send = 0; // close read loop.
				continue;
			}
			// to bin
			str2bin(reciver_read_buf, reciver_bin_from, counter_input_reciver);
			// turn over bytes in given ratio.
			// turn
			num_bits_fliped = 0;
			for (i = 0; i < counter_input_reciver; i++) {
				sender_bin_to[i] = reciver_bin_from[i]; // in this diraction, no filiping gets done.
			}
			// back from bin
			bin2str(sender_bin_to, sender_write_buf, counter_input_reciver);
			// writing the result to reciver.
			if ((counter_output_sender = write(sender_conn_fd, sender_write_buf, counter_input_reciver)) == -1) { // On success, the number of bytes written is returned (zero indicates nothing was written). On error, -1 is returned, and errno is set appropriately.
				fprintf(stderr, F_ERROR_SOCKET_WRITE_MSG, strerror(errno));
				return program_end(errno, sender_fd, reciver_fd, sender_conn_fd, reciver_conn_fd);
			}
			if (DEBUG) { printf("FLAG 6 - messege length %d bytes\n", counter_output_sender); } // TODO XXX DEBUG
			if (DEBUG) { printf("FLAG 7 - fliped %d bits\n", num_bits_fliped); } // TODO XXX DEBUG
		}
	} // end of while 1
	//TODO - close ramming fd and so on.
	return 0;
}
