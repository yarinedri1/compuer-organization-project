#define _CRT_SECURE_NO_WARNINGS
// Required header files for the assembler
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Definitions and size restrictions for the assembler
#define INPUT_MAX_LINE_SIZE 500
#define LABEL_MAX_SIZE 50
#define MAX_LABEL_AMOUNT 4096
#define MEMIN_MAX_LINE_NUM 4096
#define MAX_INSTRUCTION_AMOUNT 4096
#define BAD_OPCODE -1
#define BAD_REGISTER -2
#define BAD_LABEL -3
#define BAD_FILE -4
#define BAD_ARGS -5

// struct to represent a label with its name and memory address
typedef struct label_ {
	char name[LABEL_MAX_SIZE];
	int address; //hex number
}label;

// label table for first pass data collection
label label_table[MAX_LABEL_AMOUNT];

char* mem_data[MAX_INSTRUCTION_AMOUNT];

// Initialize memory data with default values
int initialize_mem_data() {
	for (int i = 0; i < MAX_INSTRUCTION_AMOUNT; i++) {
		mem_data[i] = (char*)malloc(9 * sizeof(char));
		if (mem_data[i] == NULL) {
			printf("Memory Error!");
			return 1;
		}
		strcpy(mem_data[i], "00000000");
	}
	return 0;
}

// map opcode to hex
void MapOpCode(char* opcode) {
	if (strcmp(opcode, "add") == 0) strcpy(opcode, "00");
	if (strcmp(opcode, "sub") == 0) strcpy(opcode, "01");
	if (strcmp(opcode, "mul") == 0) strcpy(opcode, "02");
	if (strcmp(opcode, "and") == 0) strcpy(opcode, "03");
	if (strcmp(opcode, "or") == 0) strcpy(opcode, "04");
	if (strcmp(opcode, "xor") == 0) strcpy(opcode, "05");
	if (strcmp(opcode, "sll") == 0) strcpy(opcode, "06");
	if (strcmp(opcode, "sra") == 0) strcpy(opcode, "07");
	if (strcmp(opcode, "srl") == 0) strcpy(opcode, "08");
	if (strcmp(opcode, "beq") == 0) strcpy(opcode, "09");
	if (strcmp(opcode, "bne") == 0) strcpy(opcode, "0A");
	if (strcmp(opcode, "blt") == 0) strcpy(opcode, "0B");
	if (strcmp(opcode, "bgt") == 0) strcpy(opcode, "0C");
	if (strcmp(opcode, "ble") == 0) strcpy(opcode, "0D");
	if (strcmp(opcode, "bge") == 0) strcpy(opcode, "0E");
	if (strcmp(opcode, "jal") == 0) strcpy(opcode, "0F");
	if (strcmp(opcode, "lw") == 0) strcpy(opcode, "10");
	if (strcmp(opcode, "sw") == 0) strcpy(opcode, "11");
	if (strcmp(opcode, "reti") == 0) strcpy(opcode, "12");
	if (strcmp(opcode, "in") == 0) strcpy(opcode, "13");
	if (strcmp(opcode, "out") == 0) strcpy(opcode, "14");
	if (strcmp(opcode, "halt") == 0) strcpy(opcode, "15");
}

// map register to hex
void MapRegister(char* reg, char* output) {
	if (strcmp(reg, "$zero") == 0) output[0] = '0';
	if (strcmp(reg, "$imm") == 0) output[0] = '1';
	if (strcmp(reg, "$v0") == 0) output[0] = '2';
	if (strcmp(reg, "$a0") == 0) output[0] = '3';
	if (strcmp(reg, "$a1") == 0) output[0] = '4';
	if (strcmp(reg, "$a2") == 0) output[0] = '5';
	if (strcmp(reg, "$t3") == 0) output[0] = '6';
	if (strcmp(reg, "$t0") == 0) output[0] = '7';
	if (strcmp(reg, "$t1") == 0) output[0] = '8';
	if (strcmp(reg, "$t2") == 0) output[0] = '9';
	if (strcmp(reg, "$s0") == 0) output[0] = 'A';
	if (strcmp(reg, "$s1") == 0) output[0] = 'B';
	if (strcmp(reg, "$s2") == 0) output[0] = 'C';
	if (strcmp(reg, "$gp") == 0) output[0] = 'D';
	if (strcmp(reg, "$sp") == 0) output[0] = 'E';
	if (strcmp(reg, "$ra") == 0) output[0] = 'F';
	output[1] = '\0'; // Null-terminate the string
}

// Function to add a label and its address to the labels table in the first pass
void add_label(char* name, int address, int label_counter, label label_table[]) {
	strcpy(label_table[label_counter].name, name);// Store label name
	label_table[label_counter].address = address;// Store label address
}

// Function to retrieve the address of a label by its name in the second pass
int get_label_address(char* name, int label_counter, label label_table[]) {
	for (int i = 0; i < label_counter; i++) {
		if (strcmp(label_table[i].name, name) == 0) {
			return label_table[i].address;// Return address if label is found
		}
	}
	return BAD_LABEL; // Label not found
}

// function to convert decimal number to hex string
void num_to_hex_string(int hex, char* str) {
	sprintf(str, "%X", hex);
}

// function to convert string (dec or hex) to num
int  string_to_num(char* str) {
	long num = 0;
	if (str[0] == '0' && str[1] == 'x') {
		num = strtoul(str, NULL, 16);
		if (num > 0x7FFFFFFF)
			num -= 0x100000000;  // Convert to signed 32-bit
	}
	else
		num = atoi(str);
	return num;
}

// function to remove trailing and leading whitespaces
void trim(char* str) {
	// Skip leading spaces
	char* start = str;
	while (isspace((unsigned char)*start)) start++;
	// Shift the content left if needed
	if (start != str)
		memmove(str, start, strlen(start) + 1);  // includes '\0'
	// Trim trailing spaces
	char* end = str + strlen(str) - 1;
	while (end > str && isspace((unsigned char)*end)) end--;
	*(end + 1) = '\0';  // Null-terminate
}

// Function to fully convert asm code to hex instructions
void AssemblyToHex(char* line, char* mem_data[], int* instruction_counter, int label_counter, label label_table[]) {
	char neg_immediate[INPUT_MAX_LINE_SIZE];
	char line_copy[INPUT_MAX_LINE_SIZE];
	char reg_buff[3] = "0";
	int address = 0;
	char data[INPUT_MAX_LINE_SIZE] = "\0";
	char out_put_data[INPUT_MAX_LINE_SIZE] = "\0";
	int data_len = 0;
	int number = 0;
	int immediate = 0;
	int len_hex_imm = 0;
	char output_line[20] = "\0";
	char second_line[20] = "\0";
	strcpy(line_copy, line);
	char opcode_asm_buff[INPUT_MAX_LINE_SIZE];
	char para1_asm_buff[INPUT_MAX_LINE_SIZE];
	char para2_asm_buff[INPUT_MAX_LINE_SIZE];
	char para3_asm_buff[INPUT_MAX_LINE_SIZE];
	char para4_asm_buff[INPUT_MAX_LINE_SIZE];
	sscanf(line, "%s", opcode_asm_buff);
	if (strcmp(opcode_asm_buff, ".word") == 0) { // .word command
		sscanf(line_copy, " %s %s %s", opcode_asm_buff, para1_asm_buff, para2_asm_buff);
		trim(opcode_asm_buff); trim(para1_asm_buff); trim(para2_asm_buff);
		printf("%s %s %s\n", opcode_asm_buff, para1_asm_buff, para2_asm_buff);
		address = string_to_num(para1_asm_buff);
		if (para2_asm_buff[0] == '0' && para2_asm_buff[1] == 'x') // handle data
			strcpy(data, para2_asm_buff + 2);
		else {
			number = atoi(para2_asm_buff);
			// Convert decimal string to hex string
			sprintf(data, "%X", number);
		}
		data_len = strlen(data);
		for (int i = 0; i < (8 - data_len); i++) { // add leading zeroes up to 8 bits to the data
			strcat(out_put_data, "0");
		}
		strcat(out_put_data, data);
		strcpy(mem_data[address], out_put_data); // store data
	}

	else { // Normal command
		sscanf(line, "%s %[^,] , %[^,] , %[^,] , %s", opcode_asm_buff, para1_asm_buff, para2_asm_buff, para3_asm_buff, para4_asm_buff); // break down instruction
		trim(opcode_asm_buff); trim(para1_asm_buff); trim(para2_asm_buff); trim(para3_asm_buff); trim(para4_asm_buff);
		printf("%s %s, %s, %s, %s\n", opcode_asm_buff, para1_asm_buff, para2_asm_buff, para3_asm_buff, para4_asm_buff);
		MapOpCode(opcode_asm_buff); strcat(output_line, opcode_asm_buff); // convert opcodes and registers to hex values
		MapRegister(para1_asm_buff, reg_buff); strcat(output_line, reg_buff);
		MapRegister(para2_asm_buff, reg_buff); strcat(output_line, reg_buff);
		MapRegister(para3_asm_buff, reg_buff); strcat(output_line, reg_buff);
		if (get_label_address(para4_asm_buff, label_counter, label_table) != BAD_LABEL) { // check if a symbolic label is used
			strcat(output_line, "100"); // bigimm = 1 (output_line = opcode + para1 + para2 + para3 + 0x100 = 32 bits)
			immediate = get_label_address(para4_asm_buff, label_counter, label_table);
			num_to_hex_string(immediate, para4_asm_buff); // get address as hex string
			len_hex_imm = strlen(para4_asm_buff);
			for (int i = 0; i < (8 - len_hex_imm); i++) { // add leading zeroes up to 8 bits to the address
				strcat(second_line, "0");
			}
			strcat(second_line, para4_asm_buff); // second_line contains the address of the label
		}

		else { // immediate is not a label
			immediate = string_to_num(para4_asm_buff);
			if (immediate < -128 || immediate > 127) { // a second line is needed for imm since 8 btyes can't represent it
				strcat(output_line, "100"); // bigimm = 1
				num_to_hex_string(immediate, para4_asm_buff);
				len_hex_imm = strlen(para4_asm_buff);
				for (int i = 0; i < (8 - len_hex_imm); i++) { //add leading zeroes up to 8 bits to the immediate
					strcat(second_line, "0");
				}
				strcat(second_line, para4_asm_buff); // second_line contains the value of the immediate represented by 32 bits
			}
			else { // imm8 is used, no second line needed
				strcat(output_line, "0"); // bigimm = 0
				num_to_hex_string(immediate, para4_asm_buff);
				if (strlen(para4_asm_buff) == 1)
					strcat(output_line, "0"); // leading zero if imm is 4 bits
				if (immediate < 0) {
					neg_immediate[0] = para4_asm_buff[6]; neg_immediate[1] = para4_asm_buff[7]; neg_immediate[2] = '\0'; // if immediate is neg, remove leading F
					strcat(output_line, neg_immediate);
				}
				if (immediate >= 0)
					strcat(output_line, para4_asm_buff);
			}
		}
		strcpy(mem_data[*instruction_counter], output_line); // store instruction in memory
		if (strcmp(second_line, "\0") != 0) { // if second_line is needed, increase PC by 2 instead of 1 
			*instruction_counter = *instruction_counter + 1;
			strcpy(mem_data[*instruction_counter], second_line);
		}
		(*instruction_counter)++;
	}
}

// first assembler pass
int first_pass(FILE* asm, label label_table[]) {
	int immediate = 0;
	char opcode_asm_buff[INPUT_MAX_LINE_SIZE];
	char para1_asm_buff[INPUT_MAX_LINE_SIZE];
	char para2_asm_buff[INPUT_MAX_LINE_SIZE];
	char para3_asm_buff[INPUT_MAX_LINE_SIZE];
	char para4_asm_buff[INPUT_MAX_LINE_SIZE];
	char rest[INPUT_MAX_LINE_SIZE];
	int addr = 0;
	int label_counter = 0;
	int ilc = 0;  // instruction location counter
	char* colon = NULL;
	char line_buff[INPUT_MAX_LINE_SIZE];
	while (!feof(asm)) {
		fgets(line_buff, INPUT_MAX_LINE_SIZE, asm); // filling label_table	
		trim(line_buff);  // Trim leading/trailing whitespace first	
		if (line_buff[0] == '\0' || line_buff[0] == '#') // Skip empty lines and comment-only lines
			continue;
		// get label
		colon = strchr(line_buff, ':'); // check the current line for a label
		if (colon) { // label found
			*colon = '\0';
			addr = ilc;
			trim(line_buff);
			printf("Label found: %s\n", line_buff);
			add_label(line_buff, addr, label_counter, label_table); // save the label
			label_counter++;
			// Move pointer past label+colon to the rest of the line
			char* rest = colon + 1;
			while (isspace(*rest)) rest++; // Skip spaces
			// If there's code after the label, treat it like a normal line
			if (*rest == '\0' || *rest == '\n' || *rest == '#')
				continue;
			else
				strcpy(line_buff, rest);
		}
		sscanf(line_buff, "%s %[^,] , %[^,] , %[^,] , %s", opcode_asm_buff, para1_asm_buff, para2_asm_buff, para3_asm_buff, para4_asm_buff);
		ilc++;
		if ((para4_asm_buff[0] >= 'A' && para4_asm_buff[0] <= 'Z') || (para4_asm_buff[0] >= 'a' && para4_asm_buff[0] <= 'z')) // increase ilc by two if a second line is used
			ilc++;
		else {
			immediate = string_to_num(para4_asm_buff);
			if (immediate < -128 || immediate > 127)
				ilc++;
		}
	}
	rewind(asm);
	return label_counter;
}

// second assembler pass
void second_pass(FILE* asm, FILE* memin, char* mem_data[], int label_counter, label label_table[]) {
	int instruction_counter = 0;
	char line_buffer[INPUT_MAX_LINE_SIZE];
	char* hashtag = NULL;
	char* colon = NULL;
	while (fgets(line_buffer, INPUT_MAX_LINE_SIZE, asm)) {
		trim(line_buffer);  // Trim leading/trailing whitespace first	
		if (line_buffer[0] == '\0' || line_buffer[0] == '#') // Skip empty lines and comment-only lines
			continue;
		hashtag = strchr(line_buffer, '#');
		if (hashtag)
			*hashtag = '\0'; // ignore comments in the asm file
		colon = strchr(line_buffer, ':');
		if (colon) {
			*colon = '\0'; // Terminate label part
			trim(line_buffer);
			char* rest = colon + 1;
			while (isspace(*rest)) rest++; // Skip leading whitespace
			if (*rest == '\0' || *rest == '\n') continue; // Label-only line
			strcpy(line_buffer, rest); // Shift rest of line into buffer
		}
		if (line_buffer[0] == '\0' || line_buffer[0] == '\n') continue; // ignore label-only lines
		AssemblyToHex(line_buffer, mem_data, &instruction_counter, label_counter, label_table);
	}
	for (int i = 0; i < MAX_INSTRUCTION_AMOUNT; i++) {
		fprintf(memin, "%s\n", mem_data[i]); // write the data to the output file
	}
}

int main(int argc, char* argv[]) {
	if (argc != 3) {
		printf("Error! NOT ENOUGH COMMAND LINE ARGUMENTS!\n");
		return BAD_ARGS;
	}

	FILE* asm_f = fopen(argv[1], "r");
	if (!asm_f) {
		printf("Error! COULD NOT OPEN program.asm FILE!\n");
		return BAD_FILE;
	}

	FILE* memin = fopen(argv[2], "w");
	if (!memin) {
		printf("Error! COULD NOT OPEN memin FILE!\n");
		return BAD_FILE;
	}

	int label_counter = 0;

	// Initialize memory data
	initialize_mem_data();

	// assembler first pass 
	label_counter = first_pass(asm_f, label_table);

	// assembler second pass
	second_pass(asm_f, memin, mem_data, label_counter, label_table);

	fclose(asm_f);
	fclose(memin);

	for (int i = 0; i < MAX_INSTRUCTION_AMOUNT; i++) {
		free(mem_data[i]);
	}
	return 0;
}









