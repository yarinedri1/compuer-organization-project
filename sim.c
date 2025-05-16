#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define BAD_ARG -1
#define BAD_FILE -2
#define NO_MEM -3
#define EOF -4
#define ONE_ROW_INSTRUCTION 1
#define TWO_ROW_INSTRUCTION 2	

// define file sizes
#define MEMORY_SIZE 4096        // Maximum number of lines in the memory
#define LINE_LENGTH 10        // Each line can hold up to 8 characters for instrucation + 2 for newline
#define DISK_ROWS 16384           // Number of rows in the disk => 128X128 each row is 32 bits
#define MAX_CYCLES (1024*4096)  // Maximum possible cycles needed to execute a program

// create program counter & clock
unsigned int CLK = 0;
unsigned int PC = 0;

// branch/jal & ISR indicators
int branch = 0;
int jal = 0;

// struct to reprensent register
typedef struct register_{
    char name[8];
	char hexcode[3]; // 1 hex charcter + 2
    int data; // 32 bits in 8 hex characters + 2 for newline
}register_;

typedef struct IOregister {
    char name[20];
	char hexcode[4]; // 2 hex charcter + 2
    int data;
} IOregister;

// register file
register_* registers[16]; //array of 16 registers of size 32 bits each

// initializng registers
void init_registers() {
	for (int i = 0; i < 16; i++) {
		registers[i] = (register_*)malloc(sizeof(register_));
		registers[i]->data=0;	
	}
	strcpy(registers[0]->name, "$zero"); strcpy(registers[0]->hexcode, "0");
	strcpy(registers[1]->name, "$imm"); strcpy(registers[1]->hexcode, "1");
	strcpy(registers[2]->name, "$v0"); strcpy(registers[2]->hexcode, "2");
	strcpy(registers[3]->name, "$a0"); strcpy(registers[3]->hexcode, "3");
	strcpy(registers[4]->name, "$a1"); strcpy(registers[4]->hexcode, "4");
	strcpy(registers[5]->name, "$a2"); strcpy(registers[5]->hexcode, "5");
	strcpy(registers[6]->name, "$a3"); strcpy(registers[6]->hexcode, "6");
	strcpy(registers[7]->name, "$t0"); strcpy(registers[7]->hexcode, "7");
	strcpy(registers[8]->name, "$t1"); strcpy(registers[8]->hexcode, "8");
	strcpy(registers[9]->name, "$t2"); strcpy(registers[9]->hexcode, "9");
	strcpy(registers[10]->name, "$s0"); strcpy(registers[10]->hexcode, "A");
	strcpy(registers[11]->name, "$s1"); strcpy(registers[11]->hexcode, "B");
	strcpy(registers[12]->name, "$s2"); strcpy(registers[12]->hexcode, "C");
	strcpy(registers[13]->name, "$gp"); strcpy(registers[13]->hexcode, "D");
	strcpy(registers[14]->name, "$sp"); strcpy(registers[14]->hexcode, "E");
	strcpy(registers[15]->name, "$ra"); strcpy(registers[15]->hexcode, "F");
}

// create IOregisters
 IOregister* IOregisters[23];

// initializing IOregisters
void init_IOregisters() {
	for (int i = 0; i < 23; i++) {
		IOregisters[i] = (IOregister*)malloc(sizeof(IOregister));
		IOregisters[i]->data = 0;
	}
	strcpy(IOregisters[0]->name, "irq0enable"); strcpy(IOregisters[0]->hexcode, "00");
	strcpy(IOregisters[1]->name, "irq1enable"); strcpy(IOregisters[1]->hexcode, "01");
	strcpy(IOregisters[2]->name, "irq2enable"); strcpy(IOregisters[2]->hexcode, "02");
	strcpy(IOregisters[3]->name, "irq0status"); strcpy(IOregisters[3]->hexcode, "03");
	strcpy(IOregisters[4]->name, "irq1status"); strcpy(IOregisters[4]->hexcode, "04");
	strcpy(IOregisters[5]->name, "irq2status"); strcpy(IOregisters[5]->hexcode, "05");
	strcpy(IOregisters[6]->name, "irqhandler"); strcpy(IOregisters[6]->hexcode, "06");
	strcpy(IOregisters[7]->name, "irqerturn");	strcpy(IOregisters[7]->hexcode, "07");
	strcpy(IOregisters[8]->name, "clks"); strcpy(IOregisters[8]->hexcode, "08");
	strcpy(IOregisters[9]->name, "leds"); strcpy(IOregisters[9]->hexcode, "09");
	strcpy(IOregisters[10]->name, "display7seg"); strcpy(IOregisters[10]->hexcode, "0A");
	strcpy(IOregisters[11]->name, "timerenable"); strcpy(IOregisters[11]->hexcode, "0B");
	strcpy(IOregisters[12]->name, "timercurrent"); strcpy(IOregisters[12]->hexcode, "0C");
	strcpy(IOregisters[13]->name, "timermax"); strcpy(IOregisters[13]->hexcode, "0D");
	strcpy(IOregisters[14]->name, "diskcmd"); strcpy(IOregisters[14]->hexcode, "0E");
	strcpy(IOregisters[15]->name, "disksector"); strcpy(IOregisters[15]->hexcode, "0F"); 
	strcpy(IOregisters[16]->name, "diskbuffer"); strcpy(IOregisters[16]->hexcode, "10"); 
	strcpy(IOregisters[17]->name, "diskstatus"); strcpy(IOregisters[17]->hexcode, "11");
	strcpy(IOregisters[18]->name, "reserved"); strcpy(IOregisters[18]->hexcode, "12");
	strcpy(IOregisters[19]->name, "reserved"); strcpy(IOregisters[19]->hexcode, "13");
	strcpy(IOregisters[20]->name, "monitoraddr"); strcpy(IOregisters[20]->hexcode, "14");
	strcpy(IOregisters[21]->name, "monitordata"); strcpy(IOregisters[21]->hexcode, "15");
	strcpy(IOregisters[22]->name, "monitorcmd"); strcpy(IOregisters[22]->hexcode, "16");
}
unsigned char monitor[256][256] = { 0 }; // every pixal represented by a byte initialed to black
// define instruction structure
typedef struct instruction {
	char op_code[3];       // 2 hex + 1 null terminator
	char rd[2];             // 1 hex + 1 null terminator
	char rs[2];             // 1 hex + 1 null terminator
	char rt[2];             // 1 hex + 1 null terminator
	char reservedBigimm[2];       // 1 hex + 1 null terminator
	char imm8[3];           // 2 hex + 1 null terminator
}instruction;

char* mainMemory[MEMORY_SIZE];  // main memory
char* disk[DISK_ROWS];          // hard drive

// initialize disk
void init_disk() {
	for (int i = 0; i < DISK_ROWS; i++) {
		disk[i] = (char*)malloc(sizeof(char)*LINE_LENGTH);
		strcpy(disk[i], "00000000");
	}
}
// initialize main memory
void init_memory() {
	for (int i = 0; i < MEMORY_SIZE; i++) {
		mainMemory[i] = (char*)malloc(sizeof(char) * LINE_LENGTH);
		strcpy(mainMemory[i], "00000000");
	}
}
// read line from file

int read_line_from_memin(FILE* memin, char* line_buffer, instruction* instr) {
	if (fgets(line_buffer, LINE_LENGTH, memin) == NULL) {
		return EOF; // EOF
	}
	strncpy(instr->op_code, line_buffer, 2); instr->op_code[2] = '\0';
	strncpy(instr->rd, line_buffer + 2, 1); instr->rd[1] = '\0';
	strncpy(instr->rs, line_buffer + 3, 1); instr->rs[1] = '\0';
	strncpy(instr->rt, line_buffer + 4, 1); instr->rt[1] = '\0';
	strncpy(instr->reservedBigimm, line_buffer + 5, 1); instr->reservedBigimm[1] = '\0';
	strncpy(instr->imm8, line_buffer + 6, 2); instr->imm8[2] = '\0';

	// check it is two rows instuction
	if (instr->reservedBigimm == 1)
		return TWO_ROW_INSTRUCTION;
	else
		return ONE_ROW_INSTRUCTION;

}
// read second line from file if bigimm = 1  

int read_second_line_from_memin(FILE* memin, char* second_line_buffer) {
	int bigNumAddress = 0;
	if (fgets(second_line_buffer, LINE_LENGTH, memin) == NULL) {
		return EOF; // EOF
	}
	bigNumAddress = strtol(second_line_buffer, NULL, 16);
	return bigNumAddress;
}

int decodeInstruction(instruction* instr, int bigimm) {
	int rd = 0, rs = 0, rt = 0;
	rd = strtol(instr->rd, NULL, 16);
	rs = strtol(instr->rs, NULL, 16);
	rt = strtol(instr->rt, NULL, 16);
	if (strcmp(instr->op_code, "00") == 0) { // add
		if (rd == 1 || rs == 1 || rt == 1) { //immidiate reg is used
			if (instr->reservedBigimm == 0)
				registers[rd]->data = registers[rs]->data + registers[rt]->data;
			else
				registers[rd]->data = registers[rs]->data + bigimm;
		}
	}
	else if (strcmp(instr->op_code, "01") == 0) { // sub
		// decode sub instruction
	}
	else if (strcmp(instr->op_code, "02") == 0) { // mul
		// decode mul instruction
	}
	else if (strcmp(instr->op_code, "03") == 0) { // and
		// decode and instruction
	}
	else if (strcmp(instr->op_code, "04") == 0) { // or
		// decode or instruction
	}
	else if (strcmp(instr->op_code, "05") == 0) { // xor
		// decode xor instruction
	}
	else if (strcmp(instr->op_code, "06") == 0) { // sll
		// decode sll instruction
	}
	else if (strcmp(instr->op_code, "07") == 0) { // sra
		// decode sra instruction
	}
	else if (strcmp(instr->op_code, "08") == 0) { // srl
		// decode srl instruction
	}
	else if (strcmp(instr->op_code, "09") == 0) { // beq
		// decode beq instruction
	}
	else if (strcmp(instr->op_code, "0A") == 0) { // bne
		// decode bne instruction
	}
	else if (strcmp(instr->op_code, "0B") == 0) { // blt
		// decode blt instruction
	}
	else if (strcmp(instr->op_code, "0C") == 0) { // bgt
		// decode bgt instruction
	}
	else if (strcmp(instr->op_code, "0D") == 0) { // ble
		// decode ble instruction
	}
	else if (strcmp(instr->op_code, "0E") == 0) { // bge
		// decode bge instruction
	}
	else if (strcmp(instr->op_code, "0F") == 0) { // jal
		// decode jal instruction
	}
	else if (strcmp(instr->op_code, "10") == 0) { // lw
		// decode lw instruction
	}
	else if (strcmp(instr->op_code, "11") == 0) { // sw
		// decode sw instruction
	}
	else if (strcmp(instr->op_code, "12") == 0) { // reti
		// decode reti instruction
	}
	else if (strcmp(instr->op_code, "13") == 0) { // in
		// decode in instruction
	}
	else if (strcmp(instr->op_code, "14") == 0) { // out
		// decode out instruction
	}
	else if (strcmp(instr->op_code, "15") == 0) { // halt
		// decode halt instruction

	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
	printf("simulator is running...\n");

	// check if the number of arguments is correct
	if(argc !=14)
		("Error! wrong number of command line argumets!\nNeed to be exectly 13");
	return -1;
	
	// open & check files

	FILE* memin = fopen(argv[1], "r");
	if (!memin) {
		printf("Error! cannot open file %s\n", argv[1]);
		fclose(memin);
		return BAD_FILE;
	}
	FILE* diskin = fopen(argv[2], "r");
	if (!diskin) {
		printf("Error! cannot open file %s\n", argv[2]);
		fclose(memin);
		return BAD_FILE;
	}
	FILE* irq2in = fopen(argv[3], "r");
	if (!irq2in) {
		printf("Error! cannot open file %s\n", argv[3]);
		fclose(memin);
		return BAD_FILE;
	}
	FILE* memout = fopen(argv[4], "w");
	if (!memout) {
		printf("Error! cannot open file %s\n", argv[4]);
		fclose(memin);
		return BAD_FILE;
	}
	FILE* reguot = fopen(argv[5], "w");
	if (!reguot) {
		printf("Error! cannot open file %s\n", argv[5]);
		fclose(memin);
		return BAD_FILE;
	}
	FILE* trace = fopen(argv[6], "w");
	if (!trace) {
		printf("Error! cannot open file %s\n", argv[6]);
		fclose(memin);
		return BAD_FILE;
	}
	FILE* hwregtrace = fopen(argv[7], "w");
	if (!hwregtrace) {
		printf("Error! cannot open file %s\n", argv[7]);
		fclose(memin);
		return BAD_FILE;
	}
	FILE* cycles = fopen(argv[8], "w");
	if (!cycles) {
		printf("Error! cannot open file %s\n", argv[8]);
		fclose(memin);
		return BAD_FILE;
	}
	FILE* leds = fopen(argv[9], "w");
	if (!leds) {
		printf("Error! cannot open file %s\n", argv[9]);
		fclose(memin);
		return BAD_FILE;
	}
	FILE* display7seg = fopen(argv[10], "w");
	if (!display7seg) {
		printf("Error! cannot open file %s\n", argv[10]);
		fclose(memin);
		return BAD_FILE;
	}
	FILE* diskout = fopen(argv[11], "w");
	if (!diskout) {
		printf("Error! cannot open file %s\n", argv[11]);
		fclose(memin);
		return BAD_FILE;
	}
	FILE* monitor = fopen(argv[12], "w");
	if (!monitor) {
		printf("Error! cannot open file %s\n", argv[12]);
		fclose(memin);
		return BAD_FILE;
	}
	FILE* monitoryuv = fopen(argv[13], "w");
	if (!monitoryuv) {
		printf("Error! cannot open file %s\n", argv[13]);
		fclose(memin);
		return BAD_FILE;
	}
	// $$$ INITIALIZING HARDWARE COMPUNENTS $$$ //
	
	init_registers();        // initialize CPU registers

	init_IOregisters();      // initialize IO registers

	init_memory();           // initialize MAIN MEMORY

	init_disk();             // initialize DISK

	// initialize struct instruction
	instruction* instr = (instruction*)malloc(sizeof(instruction));
	if (!instr)
		return NO_MEM;

	char line_buffer[LINE_LENGTH];
	char second_line_buffer[LINE_LENGTH];

	while (1) {
		int result = 0;
		int bigNumAddress = 0;

		// read line from memin
		result = read_line_from_memin(memin, line_buffer, instr);
		if (result == EOF) {
			printf("finished simulation by EOF\n");
			return EOF; // EOF
		}
		strcpy(mainMemory[PC], line_buffer);
		PC++;
		CLK++;
		if (result == TWO_ROW_INSTRUCTION) {

			bigNumAddress = read_second_line_from_memin(memin, second_line_buffer);
			if (bigNumAddress == EOF) {
				printf("finished simulation by EOF\n");
				return EOF; // EOF
			}
			strcpy(mainMemory[PC], second_line_buffer);
			PC++;
			CLK++;
		}
		


		


	

	}




}
