#include <stdio.h>	// sscanf
#include <string.h>	// strlen

#define HAMMING_FROM	9 // In bits // 57 TODO
#define HAMMING_TO	10 // In bits // 63 TODO
#define MAX_BUF_THEORY	20 // In bytes // 4095 TODO
#define MAX_BUF		(MAX_BUF_THEORY*HAMMING_FROM)/HAMMING_TO	// In bytes, =3705

// Define printing strings
#define IP_INVALID_MSG			"%s is not a valid IPv4 address\n"
#define PORT_INVALID_MSG		"The port must be a positive integer between 1 to 65535\n"
#define REPORT_CHANNEL			"sender: %s\nreceiver: %s\n%s bytes flipped %s bits\n"
#define REPORT_RECEIVER			"received: %d bytes\nwrote: %d bytes\ncorrected: %d errors\n"
#define REPORT_SENDER			"received: %s bytes\nreconstructed: %s bytes\ncorrected: %s errors\n"
#define USAGE_OPERANDS_MISSING_MSG	"Missing operands\nUsage: %s <IP> <PORT> <%s>\n"
#define USAGE_OPERANDS_SURPLUS_MSG	"Too many operands\nUsage: %s <IP> <PORT> <%s>\n"
#define ERROR_EXIT_MSG			"Exiting...\n"
#define F_ERROR_FUNCTION_LSEEK_MSG	"[Error] lseek() failed with an error: %s\n"
#define F_ERROR_FUNCTION_SPRINTF_MSG	"[Error] sprintf() failed with an error\n"
#define F_ERROR_FUNCTION_STRTOL_MSG	"[Error] strtol() failed with an error: %s\n"
#define F_ERROR_INPUT_CLOSE_MSG		"[Error] Close input file: %s\n"
#define F_ERROR_INPUT_FILE_MSG		"[Error] Input file '%s': %s\n"
#define F_ERROR_INPUT_IS_FOLDER_MSG	"[Error] Input file '%s': Is a directory\n"
#define F_ERROR_INPUT_OPEN_MSG		"[Error] Could not open input file '%s': %s\n"
#define F_ERROR_INPUT_READ_MSG		"[Error] Reading from input file %s: %s\n"
#define F_ERROR_OUTPUT_CLOSE_MSG	"[Error] Close output file: %s\n"
#define F_ERROR_OUTPUT_FILE_MSG		"[Error] Output file '%s': %s\n"
#define F_ERROR_OUTPUT_WRITE_MSG	"[Error] Writing to output file %s: %s\n"
#define F_ERROR_SOCKET_ACCEPT_MSG	"[Error] Accept socket: %s\n"
#define F_ERROR_SOCKET_BIND_MSG		"[Error] Socket bind failed: %s\n"
#define F_ERROR_SOCKET_CLOSE_MSG	"[Error] Close socket: %s\n"
#define F_ERROR_SOCKET_CONNECT_MSG	"[Error] Connect failed: %s\n"
#define F_ERROR_SOCKET_CREATE_MSG	"[Error] Could not create socket: %s\n"
#define F_ERROR_SOCKET_LISTEN_MSG	"[Error] Listen to socket failed: %s\n"
#define F_ERROR_SOCKET_READ_MSG		"[Error] Reading from socket: %s\n"
#define F_ERROR_SOCKET_SHUTDOWN_MSG	"[Error] Shutdown socket: %s\n"
#define F_ERROR_SOCKET_WRITE_MSG	"[Error] Writing to socket: %s\n"

#define DEBUG 1 // TODO XXX DEBUG

int validateIP4Dotted(const char *s) { // http://stackoverflow.com/questions/791982/determine-if-a-string-is-a-valid-ip-address-in-c#answer-14181669
	char tail[16];
	int c,i;
	int len = strlen(s);
	unsigned int d[4];
	if (len < 7 || 15 < len) {
		return -1;
	}
	tail[0] = 0;
	c = sscanf(s,"%3u.%3u.%3u.%3u%s",&d[0],&d[1],&d[2],&d[3],tail);
	if (c != 4 || tail[0]) {
		return -1;
	}
	for (i=0;i<4;i++) {
		if (d[i] > 255) {
			return -1;
		}
	}
	return 0;
}
void printAsBin(char *input,int len, int spaces) {
	if (input != NULL) {
		char *ptr = input;
		int i,j;
		for (j=0;j<len;j++,++ptr) {
			if (spaces == 1) {
				printf(" ");
			} else {
				printf("(%c)", *ptr);
			}
			for (i = 7;i>=0;--i) {
				(*ptr & 1 << i) ? putchar('1') : putchar('0');
			}
		}
		putchar('\n');
	}
}
void bin2str(char *input,char *output,int input_len) { // strlen(output) === input_len/8;!!!
	if ((input == NULL)||(input_len%8 != 0)) {
		strcpy(output,"\0");
	}
	char loop[9];
	char *ptr;
	int i,j;
	for (j=i=0;j<input_len;i++,j+=8) {
		snprintf(loop,9,"%.*s",8,input+j);
		output[i] = strtol(loop,&ptr,2);
	}
	output[input_len/8] = '\0';
}
void str2bin(char *input,char *output,int input_len) { // strlen(output) === 8*input_len;!!!
	if (input == NULL) {
		strcpy(output,"00000000");
	}
	char *ptr = input;
	int i,j;
	for (j=0;j<input_len;j++,++ptr) {
		for (i=7;i>=0;--i) {
			if (*ptr & 1 << i) {
				output[(8*j)+7-i] = '1';
			} else {
				output[(8*j)+7-i] = '0';
			}
		}
	}
	output[8*input_len] = '\0';
}
