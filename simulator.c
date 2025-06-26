#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define BAD_ARG -1
#define BAD_FILE -2
#define BAD_MEM -3
#define EOF -4
#define ONE_ROW_INSTRUCTION 1
#define TWO_ROW_INSTRUCTION 2
#define HALT 3

// define file sizes
#define MEMORY_SIZE 4096        // Maximum number of lines in the memory
#define LINE_LENGTH 10        // Each line can hold up to 8 characters for instrucation + 2 for newline
#define DISK_ROWS 16384           // Number of rows in the disk => 128X128 each row is 32 bits
#define MAX_CYCLES (1024*4096)  // Maximum possible cycles needed to execute a program

// create program counter & clock
unsigned int CLK = 0;
unsigned int PC = 0;
unsigned int disk_counter = 0;

// struct to reprensent register
typedef struct register_ {
	char name[8];
	char hexcode[3]; // 1 hex charcter + 2
	unsigned int data; // 32 bits in 8 hex characters + 2 for newline
}register_;

typedef struct IOregister {
	char name[20];
	char hexcode[4]; // 2 hex charcter + 2
	unsigned int data;
} IOregister;

// register file
register_* registers[16]; //array of 16 registers of size 32 bits each

// initializng registers
int init_registers() {
	for (int i = 0; i < 16; i++) {
		registers[i] = (register_*)malloc(sizeof(register_));
		if (registers[i] == NULL) {
			printf("Memory Error");
			return BAD_MEM;
		}
		registers[i]->data = 0;
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
	return 0;
}

// create IOregisters
IOregister* IOregisters[23];

// initializing IOregisters
int init_IOregisters() {
	for (int i = 0; i < 23; i++) {
		IOregisters[i] = (IOregister*)malloc(sizeof(IOregister));
		if (IOregisters[i] == NULL) {
			printf("Memory Error");
			return BAD_MEM;
		}
		IOregisters[i]->data = 0;
	}
	strcpy(IOregisters[0]->name, "irq0enable"); strcpy(IOregisters[0]->hexcode, "00");
	strcpy(IOregisters[1]->name, "irq1enable"); strcpy(IOregisters[1]->hexcode, "01");
	strcpy(IOregisters[2]->name, "irq2enable"); strcpy(IOregisters[2]->hexcode, "02");
	strcpy(IOregisters[3]->name, "irq0status"); strcpy(IOregisters[3]->hexcode, "03");
	strcpy(IOregisters[4]->name, "irq1status"); strcpy(IOregisters[4]->hexcode, "04");
	strcpy(IOregisters[5]->name, "irq2status"); strcpy(IOregisters[5]->hexcode, "05");
	strcpy(IOregisters[6]->name, "irqhandler"); strcpy(IOregisters[6]->hexcode, "06");
	strcpy(IOregisters[7]->name, "irqreturn");	strcpy(IOregisters[7]->hexcode, "07");
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
	return 0;
}

unsigned char monitor[256][256] = { 0 }; // every pixel represented by a byte initialized to black

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
int init_disk(FILE* diskin, char* line_buffer) {
	for (int i = 0; i < DISK_ROWS; i++) {
		disk[i] = (char*)malloc(sizeof(char) * LINE_LENGTH);
		if (disk[i] == NULL) {
			printf("Memory Error");
			return BAD_MEM;
		}
		if (fgets(line_buffer, LINE_LENGTH, diskin) == NULL)
			strcpy(disk[i], "00000000");
		else {
			line_buffer[strcspn(line_buffer, "\n")] = '\0';
			strcpy(disk[i], line_buffer);
		}
	}
	return 0;
}

// initialize main memory
int init_memory(FILE* memin, char* line_buffer) {
	for (int i = 0; i < MEMORY_SIZE; i++) {
		mainMemory[i] = (char*)malloc(sizeof(char) * LINE_LENGTH);
		if (mainMemory[i] == NULL) {
			printf("Memory Error");
			return BAD_MEM;
		}
		if (fgets(line_buffer, LINE_LENGTH, memin) == NULL)
			strcpy(mainMemory[i], "00000000");
		else {
			line_buffer[strcspn(line_buffer, "\n")] = '\0';
			strcpy(mainMemory[i], line_buffer);
		}
	}
	return 0;
}

// read line from memory and populate instruction
int read_line_from_memory(char* line_buffer, instruction* instr) {
	strcpy(line_buffer, mainMemory[PC]);
	strncpy(instr->op_code, line_buffer, 2); instr->op_code[2] = '\0';
	strncpy(instr->rd, line_buffer + 2, 1); instr->rd[1] = '\0';
	strncpy(instr->rs, line_buffer + 3, 1); instr->rs[1] = '\0';
	strncpy(instr->rt, line_buffer + 4, 1); instr->rt[1] = '\0';
	strncpy(instr->reservedBigimm, line_buffer + 5, 1); instr->reservedBigimm[1] = '\0';
	strncpy(instr->imm8, line_buffer + 6, 2); instr->imm8[2] = '\0';
	// check it is two rows instuction
	if (instr->reservedBigimm[0] == '1') // if bigimm = 1 then imm32 is used
		return TWO_ROW_INSTRUCTION;
	else
		return ONE_ROW_INSTRUCTION;
}

// read second line from memory if bigimm = 1  
int read_second_line_from_memory(char* second_line_buffer) {
	int bigNumAddress = 0;
	strcpy(second_line_buffer, mainMemory[PC]);
	bigNumAddress = strtol(second_line_buffer, NULL, 16);
	return bigNumAddress;
}

// function to write to memout.txt
void write_memory_data(FILE* memout) {
	for (int i = 0; i < MEMORY_SIZE; i++) {
		fprintf(memout, "%s\n", mainMemory[i]); // write the data to the output file
	}
}

// write data of registers 2-15 to output file
void write_register_data(FILE* regout) {
	for (int i = 2; i < 16; i++) {
		fprintf(regout, "%08X\n", registers[i]->data); // write the data to the output file
	}
}

// write data line to output trace file
void write_trace_data_line(FILE* trace, char* instr, int pc_flag) {
	fprintf(trace, "%08X ", CLK - 1); // write the cycle to the output file
	if (pc_flag == TWO_ROW_INSTRUCTION)
		fprintf(trace, "%03X ", PC - 2); // write the pc to the output file
	else
		fprintf(trace, "%03X ", PC - 1);
	fprintf(trace, "%s ", instr); // write the data to the output file
	for (int i = 0; i < 15; i++) {
		fprintf(trace, "%08X ", registers[i]->data); // write the register data to the output file
	}
	fprintf(trace, "%08X", registers[15]->data);
	fprintf(trace, "%c", '\n');
}

// write data line to output hwregtrace file
void write_hwregtrace_data_line(FILE* hwregtrace, int status, int reg_num) { // status = 0: read, status = 1: write
	//int reg_num = strtol(reg, NULL, 16);
	fprintf(hwregtrace, "%08X ", CLK - 1); // write the cycle to the output file
	if (status)
		fprintf(hwregtrace, "%s ", "WRITE");
	else
		fprintf(hwregtrace, "%s ", "READ");
	fprintf(hwregtrace, "%s ", IOregisters[reg_num]->name); // write the register name to the output file
	fprintf(hwregtrace, "%08X\n", IOregisters[reg_num]->data); // write the register data to the output file
}

// write data to output cycles file
void write_cycles_data(FILE* cycles, unsigned int clk) {
	fprintf(cycles, "%08X\n", CLK); // write the cycles to the output file
}

// write data line to output leds file
void write_leds_data_line(FILE* leds) {
	fprintf(leds, "%08X ", CLK - 1);
	fprintf(leds, "%08X\n", IOregisters[9]->data);
}

// write data line to output display7seg file
void write_display7seg_data_line(FILE* display7seg) {
	fprintf(display7seg, "%08X ", CLK - 1);
	fprintf(display7seg, "%08X\n", IOregisters[10]->data);
}

// write data to output diskout file
void write_disk_data(FILE* diskout) {
	for (int i = 0; i < DISK_ROWS; i++) {
		fprintf(diskout, "%s\n", disk[i]); // write the data to the output file
	}
}

// write data to output monitor file
void write_monitor_data(FILE* fmonitor) {
	for (int i = 0; i < 256; i++) {
		for (int j = 0; j < 256; j++) {
			fprintf(fmonitor, "%02X\n", monitor[i][j]);
		}
	}
}

// write data to output monitoryuv file
void write_monitoryuv_data(FILE* monitoryuv) {
	for (int i = 0; i < 256; i++) {
		fwrite(&monitor[i], sizeof(unsigned char), 256, monitoryuv);
	}
}

// every cycle, check for disk write/read commands, and execute operation if 1024 cycles have passed
void check_disk_status() {
	disk_counter++;
	if (IOregisters[14]->data == 1 || IOregisters[14]->data == 2) { // read/write command
		if (IOregisters[17]->data == 0) { // disk not busy
			disk_counter = CLK;
			IOregisters[17]->data = 1;
		}
	}

	if (CLK >= disk_counter + 1024 && IOregisters[17]->data == 1) {// 1024 cycles have passed since read/write operation started
		if (IOregisters[14]->data == 1) {
			for (int i = 0; i < 128; i++) {
				strcpy(mainMemory[IOregisters[16]->data + i], disk[IOregisters[15]->data * 128 + i]); // write data from disk to memory (read from disk)
			}
		}
		else if (IOregisters[14]->data == 2) {
			for (int i = 0; i < 128; i++) {
				strcpy(disk[IOregisters[15]->data * 128 + i], mainMemory[IOregisters[16]->data + i]); // write data from memory to disk (write to disk)
			}
		}
		IOregisters[17]->data = 0; // clear disk status
		IOregisters[14]->data = 0; // clear disk cmd
		IOregisters[4]->data = 1; // raise irq1status
	}
}

// every cycle, check for write command to monitor and execute accordingly
void check_monitor_status() {
	if (IOregisters[22]->data == 1) {
		int address = IOregisters[20]->data;
		int row = address / 256;
		int col = address % 256;
		monitor[row][col] = IOregisters[21]->data;
		IOregisters[22]->data = 0;
	}
}

// every cycle, check if irq2status is needed to be raised according to irq2in file
void check_irq2_status(FILE* irq2in, int* irq2_flag, char* line_buffer, int* irq2_cycle) {
	if (*irq2_flag) {
		if (fgets(line_buffer, LINE_LENGTH, irq2in)) {
			*irq2_cycle = strtol(line_buffer, NULL, 10);
			*irq2_flag = 0;
		}
	}
	if (CLK >= *irq2_cycle && *irq2_flag == 0) { // handle irq2
		*irq2_flag = 1;
		IOregisters[5]->data = 1;
		printf("triggered irq2 in cycle %d\n", CLK);
	}
}

// decode and execute instruction. imm is either imm8 or imm32.
int decodeInstruction(instruction* instr, int imm, FILE* hwregtrace, FILE* leds, FILE* display7seg) {
	int rd = 0, rs = 0, rt = 0;
	rd = strtol(instr->rd, NULL, 16);
	rs = strtol(instr->rs, NULL, 16);
	rt = strtol(instr->rt, NULL, 16);

	if (strcmp(instr->op_code, "00") == 0 && (rd != 0 && rd != 1)) { // add
		int src1 = (rs == 1) ? imm : registers[rs]->data; // handle case which immediate reg is used
		int src2 = (rt == 1) ? imm : registers[rt]->data;
		registers[rd]->data = src1 + src2;
		return 0;
	}

	else if (strcmp(instr->op_code, "01") == 0 && (rd != 0 && rd != 1)) { // sub
		int src1 = (rs == 1) ? imm : registers[rs]->data; // handle case which immediate reg is used
		int src2 = (rt == 1) ? imm : registers[rt]->data;
		registers[rd]->data = src1 - src2;
		return 0;
	}

	else if (strcmp(instr->op_code, "02") == 0 && (rd != 0 && rd != 1)) { // mul
		int src1 = (rs == 1) ? imm : registers[rs]->data; // handle case which immediate reg is used
		int src2 = (rt == 1) ? imm : registers[rt]->data;
		registers[rd]->data = src1 * src2;
		return 0;
	}

	else if (strcmp(instr->op_code, "03") == 0 && (rd != 0 && rd != 1)) { // and
		int src1 = (rs == 1) ? imm : registers[rs]->data; // handle case which immediate reg is used
		int src2 = (rt == 1) ? imm : registers[rt]->data;
		registers[rd]->data = src1 & src2;
		return 0;
	}

	else if (strcmp(instr->op_code, "04") == 0 && (rd != 0 && rd != 1)) { // or
		int src1 = (rs == 1) ? imm : registers[rs]->data; // handle case which immediate reg is used
		int src2 = (rt == 1) ? imm : registers[rt]->data;
		registers[rd]->data = src1 | src2;
		return 0;
	}

	else if (strcmp(instr->op_code, "05") == 0 && (rd != 0 && rd != 1)) { // xor
		int src1 = (rs == 1) ? imm : registers[rs]->data; // handle case which immediate reg is used
		int src2 = (rt == 1) ? imm : registers[rt]->data;
		registers[rd]->data = src1 ^ src2;
		return 0;
	}

	else if (strcmp(instr->op_code, "06") == 0 && (rd != 0 && rd != 1)) { // sll
		int src1 = (rs == 1) ? imm : registers[rs]->data; // handle case which immediate reg is used
		int src2 = (rt == 1) ? imm : registers[rt]->data;
		registers[rd]->data = src1 << src2;
		return 0;
	}

	else if (strcmp(instr->op_code, "07") == 0 && (rd != 0 && rd != 1)) { // sra
		int src1 = (rs == 1) ? imm : registers[rs]->data; // handle case which immediate reg is used
		int src2 = (rt == 1) ? imm : registers[rt]->data;
		registers[rd]->data = src1 >> src2;
		return 0;
	}

	else if (strcmp(instr->op_code, "08") == 0 && (rd != 0 && rd != 1)) { // srl
		unsigned int src1 = (rs == 1) ? (unsigned int)imm : (unsigned int)registers[rs]->data; // handle case which immediate reg is used
		int src2 = (rt == 1) ? imm : registers[rt]->data;
		registers[rd]->data = src1 >> src2;
		return 0;
	}

	else if (strcmp(instr->op_code, "09") == 0) { // beq
		int src1 = (rs == 1) ? imm : registers[rs]->data; // handle case which immediate reg is used
		int src2 = (rt == 1) ? imm : registers[rt]->data;
		int dest = (rd == 1) ? imm : registers[rd]->data;
		if (src1 == src2) {
			PC = dest;
		}
		return 0;
	}

	else if (strcmp(instr->op_code, "0A") == 0) { // bne
		int src1 = (rs == 1) ? imm : registers[rs]->data; // handle case which immediate reg is used
		int src2 = (rt == 1) ? imm : registers[rt]->data;
		int dest = (rd == 1) ? imm : registers[rd]->data;
		if (src1 != src2) {
			PC = dest;
		}
		return 0;
	}

	else if (strcmp(instr->op_code, "0B") == 0) { // blt
		int src1 = (rs == 1) ? imm : registers[rs]->data; // handle case which immediate reg is used
		int src2 = (rt == 1) ? imm : registers[rt]->data;
		int dest = (rd == 1) ? imm : registers[rd]->data;
		if (src1 < src2) {
			PC = dest;
		}
		return 0;
	}

	else if (strcmp(instr->op_code, "0C") == 0) { // bgt
		int src1 = (rs == 1) ? imm : registers[rs]->data; // handle case which immediate reg is used
		int src2 = (rt == 1) ? imm : registers[rt]->data;
		int dest = (rd == 1) ? imm : registers[rd]->data;
		if (src1 > src2) {
			PC = dest;
		}
		return 0;
	}

	else if (strcmp(instr->op_code, "0D") == 0) { // ble
		int src1 = (rs == 1) ? imm : registers[rs]->data; // handle case which immediate reg is used
		int src2 = (rt == 1) ? imm : registers[rt]->data;
		int dest = (rd == 1) ? imm : registers[rd]->data;
		if (src1 <= src2) {
			PC = dest;
		}
		return 0;
	}

	else if (strcmp(instr->op_code, "0E") == 0) { // bge
		int src1 = (rs == 1) ? imm : registers[rs]->data; // handle case which immediate reg is used
		int src2 = (rt == 1) ? imm : registers[rt]->data;
		int dest = (rd == 1) ? imm : registers[rd]->data;
		if (src1 >= src2) {
			PC = dest;
		}
		return 0;
	}

	else if (strcmp(instr->op_code, "0F") == 0 && (rd != 0 && rd != 1)) { // jal
		int src1 = (rs == 1) ? imm : registers[rs]->data; // handle case which immediate reg is used
		int src2 = (rt == 1) ? imm : registers[rt]->data;
		registers[rd]->data = PC;
		PC = src1;
		return 0;
	}

	else if (strcmp(instr->op_code, "10") == 0 && (rd != 0 && rd != 1)) { // lw
		int src1 = (rs == 1) ? imm : registers[rs]->data; // handle case which immediate reg is used
		int src2 = (rt == 1) ? imm : registers[rt]->data;
		registers[rd]->data = strtol(mainMemory[src1 + src2], NULL, 16);
		return 0;
	}

	else if (strcmp(instr->op_code, "11") == 0) { // sw
		int src1 = (rs == 1) ? imm : registers[rs]->data; // handle case which immediate reg is used
		int src2 = (rt == 1) ? imm : registers[rt]->data;
		int dest = (rd == 1) ? imm : registers[rd]->data;
		sprintf(mainMemory[src1 + src2], "%08X", dest);
		return 0;
	}

	else if (strcmp(instr->op_code, "12") == 0) { // reti
		PC = IOregisters[7]->data;
		IOregisters[7]->data = 0;
		return 0;
	}

	else if (strcmp(instr->op_code, "13") == 0 && (rd != 0 && rd != 1)) { // in
		int src1 = (rs == 1) ? imm : registers[rs]->data; // handle case which immediate reg is used
		int src2 = (rt == 1) ? imm : registers[rt]->data;
		registers[rd]->data = IOregisters[src1 + src2]->data;
		write_hwregtrace_data_line(hwregtrace, 0, src1 + src2);
		return 0;
	}

	else if (strcmp(instr->op_code, "14") == 0) { // out
		int src1 = (rs == 1) ? imm : registers[rs]->data; // handle case which immediate reg is used
		int src2 = (rt == 1) ? imm : registers[rt]->data;
		int dest = (rd == 1) ? imm : registers[rd]->data;
		int temp_data = IOregisters[src1 + src2]->data;
		IOregisters[src1 + src2]->data = dest;
		write_hwregtrace_data_line(hwregtrace, 1, src1 + src2);
		if (src1 + src2 == 9 && temp_data != dest)  // write to leds file as well if leds register is written to
			write_leds_data_line(leds);
		if (src1 + src2 == 10 && temp_data != dest) { // write to display7seg file as well if display7seg register is written to
			write_display7seg_data_line(display7seg);
		}
		return 0;
	}

	else if (strcmp(instr->op_code, "15") == 0) { // halt
		return HALT;
	}
}

// cleanup function to close all files
void close_files(FILE* memin, FILE* diskin, FILE* irq2in, FILE* memout, FILE* regout, FILE* trace, FILE* hwregtrace, FILE* cycles, FILE* leds, FILE* display7seg, FILE* diskout, FILE* fmonitor, FILE* monitoryuv) {
	if (memin != NULL)
		fclose(memin);
	if (diskin != NULL)
		fclose(diskin);
	if (irq2in != NULL)
		fclose(irq2in);
	if (memout != NULL)
		fclose(memout);
	if (regout != NULL)
		fclose(regout);
	if (trace != NULL)
		fclose(trace);
	if (hwregtrace != NULL)
		fclose(hwregtrace);
	if (cycles != NULL)
		fclose(cycles);
	if (leds != NULL)
		fclose(leds);
	if (display7seg != NULL)
		fclose(display7seg);
	if (diskout != NULL)
		fclose(diskout);
	if (fmonitor != NULL)
		fclose(fmonitor);
	if (monitoryuv != NULL)
		fclose(monitoryuv);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
	printf("simulator is running...\n");

	// check if the number of arguments is correct
	if (argc != 14) {
		printf("Error! wrong number of command line arguments!\nNeed to be exectly 13");
		return BAD_ARG;
	}

	// open & check files
	FILE* memin = fopen(argv[1], "r");
	if (!memin) {
		printf("Error! cannot open file %s\n", argv[1]);
		close_files(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
		return BAD_FILE;
	}
	FILE* diskin = fopen(argv[2], "r");
	if (!diskin) {
		printf("Error! cannot open file %s\n", argv[2]);
		close_files(memin, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
		return BAD_FILE;
	}
	FILE* irq2in = fopen(argv[3], "r");
	if (!irq2in) {
		printf("Error! cannot open file %s\n", argv[3]);
		close_files(memin, diskin, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
		return BAD_FILE;
	}
	FILE* memout = fopen(argv[4], "w");
	if (!memout) {
		printf("Error! cannot open file %s\n", argv[4]);
		close_files(memin, diskin, irq2in, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
		return BAD_FILE;
	}
	FILE* regout = fopen(argv[5], "w");
	if (!regout) {
		printf("Error! cannot open file %s\n", argv[5]);
		close_files(memin, diskin, irq2in, memout, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
		return BAD_FILE;
	}
	FILE* trace = fopen(argv[6], "w");
	if (!trace) {
		printf("Error! cannot open file %s\n", argv[6]);
		close_files(memin, diskin, irq2in, memout, regout, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
		return BAD_FILE;
	}
	FILE* hwregtrace = fopen(argv[7], "w");
	if (!hwregtrace) {
		printf("Error! cannot open file %s\n", argv[7]);
		close_files(memin, diskin, irq2in, memout, regout, trace, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
		return BAD_FILE;
	}
	FILE* cycles = fopen(argv[8], "w");
	if (!cycles) {
		printf("Error! cannot open file %s\n", argv[8]);
		close_files(memin, diskin, irq2in, memout, regout, trace, hwregtrace, NULL, NULL, NULL, NULL, NULL, NULL);
		return BAD_FILE;
	}
	FILE* leds = fopen(argv[9], "w");
	if (!leds) {
		printf("Error! cannot open file %s\n", argv[9]);
		close_files(memin, diskin, irq2in, memout, regout, trace, hwregtrace, cycles, NULL, NULL, NULL, NULL, NULL);
		return BAD_FILE;
	}
	FILE* display7seg = fopen(argv[10], "w");
	if (!display7seg) {
		printf("Error! cannot open file %s\n", argv[10]);
		close_files(memin, diskin, irq2in, memout, regout, trace, hwregtrace, cycles, leds, NULL, NULL, NULL, NULL);
		return BAD_FILE;
	}
	FILE* diskout = fopen(argv[11], "w");
	if (!diskout) {
		printf("Error! cannot open file %s\n", argv[11]);
		close_files(memin, diskin, irq2in, memout, regout, trace, hwregtrace, cycles, leds, display7seg, NULL, NULL, NULL);
		return BAD_FILE;
	}
	FILE* fmonitor = fopen(argv[12], "w");
	if (!fmonitor) {
		printf("Error! cannot open file %s\n", argv[12]);
		close_files(memin, diskin, irq2in, memout, regout, trace, hwregtrace, cycles, leds, display7seg, diskout, NULL, NULL);
		return BAD_FILE;
	}
	FILE* monitoryuv = fopen(argv[13], "w");
	if (!monitoryuv) {
		printf("Error! cannot open file %s\n", argv[13]);
		close_files(memin, diskin, irq2in, memout, regout, trace, hwregtrace, cycles, leds, display7seg, diskout, fmonitor, NULL);
		return BAD_FILE;
	}

	// initialize struct instruction
	instruction* instr = (instruction*)malloc(sizeof(instruction));
	if (!instr)
		return BAD_MEM;

	char line_buffer[LINE_LENGTH];
	char second_line_buffer[LINE_LENGTH];
	unsigned int irq2_cycle = 0;
	int decode_res = 0;
	int read_result = 0;
	int immediate = 0;
	int irq2_amount = 0;

	// $$$ INITIALIZING HARDWARE COMPONENTS $$$ //

	read_result = init_registers();        // initialize CPU registers
	if (read_result == BAD_MEM) {
		close_files(memin, diskin, irq2in, memout, regout, trace, hwregtrace, cycles, leds, display7seg, diskout, fmonitor, monitoryuv);
		return BAD_MEM;
	}

	read_result = init_IOregisters();      // initialize IO registers
	if (read_result == BAD_MEM) {
		close_files(memin, diskin, irq2in, memout, regout, trace, hwregtrace, cycles, leds, display7seg, diskout, fmonitor, monitoryuv);
		return BAD_MEM;
	}

	read_result = init_memory(memin, line_buffer);           // initialize MAIN MEMORY
	if (read_result == BAD_MEM) {
		close_files(memin, diskin, irq2in, memout, regout, trace, hwregtrace, cycles, leds, display7seg, diskout, fmonitor, monitoryuv);
		return BAD_MEM;
	}

	read_result = init_disk(diskin, line_buffer);   // initialize DISK
	if (read_result == BAD_MEM) {
		close_files(memin, diskin, irq2in, memout, regout, trace, hwregtrace, cycles, leds, display7seg, diskout, fmonitor, monitoryuv);
		return BAD_MEM;
	}

	int irq = 0; // interrupts indicator

	int irq2_flag = 1;

	while (1) { // fetch-decode-execute loop
		irq = ((IOregisters[0]->data & IOregisters[3]->data) | (IOregisters[1]->data & IOregisters[4]->data) | (IOregisters[2]->data & IOregisters[5]->data));
		if (irq == 1 && IOregisters[7]->data == 0) {
			IOregisters[7]->data = PC; // irqreturn = PC, to be used with reti command
			PC = IOregisters[6]->data; // PC = irqhandler
		}

		if (IOregisters[11]->data) { // handle timer and irq0
			if (IOregisters[12]->data == IOregisters[13]->data) { // reset timer and raise irq0status if timer = timermax
				IOregisters[3]->data = 1;
				IOregisters[12]->data = 0;
			}
			(IOregisters[12]->data)++;
		}

		check_irq2_status(irq2in, &irq2_flag, line_buffer, &irq2_cycle);

		// read line from memory
		read_result = read_line_from_memory(line_buffer, instr); // result is eof if reading is done or indicator of one or two lines instruction. instr is populated.
		printf("CYCLE is %d, PC now is %d, instr is %s%s%s%s%s%s, ", CLK, PC, instr->op_code, instr->rd, instr->rs, instr->rt, instr->reservedBigimm, instr->imm8);

		PC++;
		CLK++;

		if (read_result == TWO_ROW_INSTRUCTION) {

			check_irq2_status(irq2in, &irq2_flag, line_buffer, &irq2_cycle);

			immediate = read_second_line_from_memory(second_line_buffer); // imm will be the whole second line = bigimm32
			PC++; // increase pc and clk again because its a two row instruction
			CLK++;
		}
		else { // use imm8 if instruction is one line
			immediate = strtol(instr->imm8, NULL, 16);
			if (immediate & 0x80) { // Check if the 8th bit is 1 (negative number)
				immediate = immediate | 0xFFFFFF00; // Sign extend to 32 bits	
			}
		}
		registers[1]->data = immediate; // update $imm register

		write_trace_data_line(trace, line_buffer, read_result); // write current data to trace.txt

		printf("Immediate is %X\n", immediate);

		decode_res = decodeInstruction(instr, immediate, hwregtrace, leds, display7seg); //decode and execute instruction

		if (decode_res == HALT) {
			printf("Reached end of asm program");
			break;
		}

		check_disk_status(); // handle disk operation

		check_monitor_status(); // handle monitor operation

		IOregisters[8]->data = CLK; // update clks IOregister
	}

	write_memory_data(memout);

	write_register_data(regout);

	write_cycles_data(cycles, CLK);

	write_disk_data(diskout);

	write_monitor_data(fmonitor);

	write_monitoryuv_data(monitoryuv);

	close_files(memin, diskin, irq2in, memout, regout, trace, hwregtrace, cycles, leds, display7seg, diskout, fmonitor, monitoryuv);

	free(instr);
	for (int i = 0; i < 16; i++) {
		free(registers[i]);
	}
	for (int i = 0; i < 23; i++) {
		free(IOregisters[i]);
	}
	for (int i = 0; i < DISK_ROWS; i++) {
		free(disk[i]);
	}
	for (int i = 0; i < MEMORY_SIZE; i++) {
		free(mainMemory[i]);
	}

	return 0;
}
