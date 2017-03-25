#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>

void printAsBin(char *input,int len) {
	if (input != NULL) {
		char *ptr = input;
		int i,j;
		for (j=0;j<len;j++,++ptr) {
			printf("(%c)", *ptr);
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
int main () {
	char str_ascii[] = "abcdefghi"; // Start len()=9 bytes=72 bits
	char str_bin[8*strlen(str_ascii)+1]; // Finish len()=10 bytes=80 bits // (padded with 8 additional bits)
	int hamming_from = 9; // In bits
	int hamming_to = 10; // In bits
	//
	str2bin(str_ascii,str_bin,strlen(str_ascii));
	printf("%s\n%s\n",str_ascii,str_bin);
	bin2str(str_bin,str_ascii,strlen(str_bin));
	printf("%s\n%s\n",str_ascii,str_bin);
}
int main1 () {
	char *str_in = "abcdefghi"; // Start len()=9 bytes=72 bits
	int hamming_from = 9; // In bits
	int hamming_to = 10; // In bits
	//
	char str_out[10]; // Finish len()=10 bytes=80 bits // (padded with 8 additional bits)
	char str_tmp[1]; // tmp char
	int indx = 0; // tmp loop var
	int indent = 0;	// How many bits been parsed already, from 0 to [8*len(str_in)]-hamming_from=63
	memset(str_out,'\0',sizeof(str_out));
	printAsBin(str_in,10);
	int t=0; // TODO XXX
	while (indent<strlen(str_in)*8) { // Run total of 8 times // padding/(hamming_to-hamming_from)=8
		printf("[1] indent %d=%d,%d str '%s' => '%s'\n",indent,indent/8,indent%8,&str_in[indent/8],str_out);
		printAsBin(str_out,10);
		do {
			if (indent%8 == 0) { // Copy complete byte
				str_out[indent/8] = str_in[indent/8];
				indent += 8;
			} else if (indent%hamming_from == 0) { // Add to already shifted byte
				
				
			} else { // Copy partial byte
				snprintf(str_tmp,1,"%d",(str_in[indent/8]) & (hamming_from%indent));
				printf("%d & %d = %s\n",str_in[indent/8],hamming_from%indent,str_tmp);
				str_out[indent/8] = str_tmp[0];
				//str_out[indent/8] = str_tmp;
				indent += hamming_from%indent;
			}
			printf("[2] indent %d=%d,%d str '%s' => '%s'\n",indent,indent/8,indent%8,&str_in[indent/8],str_out);
			printAsBin(str_out,10);
		} while (indent%hamming_from != 0);
		printf("[3] indent %d=%d,%d str '%s' => '%s'\n",indent,indent/8,indent%8,&str_in[indent/8],str_out);
		printAsBin(str_out,10);
		// Now we can add the padding // TODO - replace with the real hamming code
		for (indx=hamming_from;indx<hamming_to;indx++) {
			str_out[indent/8] <<= 1;
			//indent += 1;
		}
		printf("[4] indent %d=%d,%d str '%s' => '%s'\n",indent,indent/8,indent%8,&str_in[indent/8],str_out);
		printAsBin(str_out,10);
		t++; // TODO XXX
		if (t > 1) {return 0;} // TODO XXX
	}
}
