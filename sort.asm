 #setting a 16 numbered array at address range 0x100 to 0x10f
	.word 0x100 5						  # MEM[0x100] = 5 
	.word 0x101 12                        # MEM[0x101] = 12 
	.word 0x102 6                         # MEM[0x102] = 6 
	.word 0x103 9                         # MEM[0x103] = 9 
	.word 0x104 1                         # MEM[0x104] = 1 
	.word 0x105 14                        # MEM[0x105] = 14 
	.word 0x106 3                         # MEM[0x106] = 3 
	.word 0x107 10                        # MEM[0x107] = 10 
	.word 0x108 11                        # MEM[0x108] = 11 
	.word 0x109 2                         # MEM[0x109] = 2 
	.word 0x10A 4                         # MEM[0x10A] = 4 
	.word 0x10B 13                        # MEM[0x10B] = 13 
	.word 0x10C 15                        # MEM[0x10C] = 15 
	.word 0x10D 7                         # MEM[0x10E] = 7 
	.word 0x10E 8                         # MEM[0x10D] = 8 
	.word 0x10F 16                        # MEM[0x10F] = 16 
	add $a0, $zero, $imm, 0x100          #$a0 = base address of the array
	add $a1, $zero, $imm, 16             #$a1 = number of elements in the array (n = 16)
	add $sp, $zero, $imm, 200            #initialize stack pointer to 0x800
	jal $ra, $imm, $zero, sort           #call sort procedure
	halt $zero, $zero, $zero, 0          #stop the program 
sort:
	 add $sp, $sp, $imm, -3              #adjust stack for 3 items
	 sw $ra, $sp, $imm, 2                #save return address
	 sw $a0, $sp, $imm, 1                #save a0
	 sw $a1, $sp, $imm, 0                #save a1
	 add $s2, $a0, $zero, 0              #$s2 = arr
	 add $t0, $a1, $zero, 0              #$t0 = n (number of elements in the array)
	 add $a1, $a1, $imm, -1              #$a1 = n - 1 (last index of the array)
	 add $s0, $zero, $zero, 0            #s0 = i = 0
for1:
	 bge $imm, $s0, $a1, end          #if i >= n-1, end
	 add $s1, $zero, $zero, 0 	            #s1 = j = 0
	 sub $t0, $a1, $s0, 0                #$t0 = n - i - 1 (last index to compare with) 
for2:
     bge $imm, $s1, $t0, exit2          #if j >= n-i-1, exit2
	 add $s2, $s1, $s2, 0               #$s2 = arr + j
	 lw $t1, $s2, $imm, 0               #$t1 = arr[j]
	 lw $t2, $s2, $imm, 1               #$t2 = arr[j+1]
	 bge $imm, $t2, $t1, exit1          #if arr[j+1] >= arr[j], exit2
	 sw $t1, $s2, $imm, 1 				#arr[j+1] = arr[j]
	 sw $t2, $s2, $imm, 0 				#arr[j] = arr[j+1]
exit1:
	 add $s1, $s1, $imm, 1              #j = j + 1
	 add $s2, $a0, $zero, 0             #reset $s2 to base address of the array
	 beq $imm, $zero, $zero, for2       #jump for2
exit2:
	  add $s0, $s0, $imm, 1 		    #$i = i + 1
	  beq $imm, $zero, $zero, for1		#jump for1
end:
	 lw $a1, $sp, $imm, 0                #restore s0
	 lw $a0, $sp, $imm, 1                #restore s1
	 lw $ra, $sp, $imm, 2                #restore return address
	 beq $ra, $zero, $zero, 0           #return to caller
