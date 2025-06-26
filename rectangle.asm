	 #drawing a 6X11 rectangle
	.word 0x100 275					#vertex A at (1,20)
	.word 0x101 1555				#vertex B at (6,20)
	.word 0x102 1565				#vertex C at (6,30)
	.word 0x103 285					#vertex D at (1,30)
	lw $a0, $zero, $imm, 0x100		# Load vertex A into $a0
	lw $a1, $zero, $imm, 0x101		# Load vertex B into $a1
	lw $a2, $zero, $imm, 0x102		# Load vertex C into $a2
	lw $a3, $zero, $imm, 0x103		# Load vertex D into $a3
	add $sp, $zero, $imm, 500		# Initialize stack pointer to 500
	jal $ra, $imm, $zero, rectangle		# Call rectangle procedure
	halt $zero, $zero, $zero, 0		# Stop the program
rectangle:
	add $sp, $sp, $imm, -5			# Adjust stack for 4 items
	sw $ra, $sp, $imm, 4			# Save return address
	sw $a0, $sp, $imm, 3			# Save vertex A
	sw $a1, $sp, $imm, 2			# Save vertex B
	sw $a2, $sp, $imm, 1            # Save vertex C
	sw $a3, $sp, $imm, 0			# Save vertex D
	add $t0, $a0, $zero, 0		    # $t0 = i = vertex A
	sub $s0, $a3, $a0, 0		    # $s0 = vertex D - vertex A (length)
	add $s1, $t0, $zero, 0		    # $s1 = start of a line
	add $s2, $s0, $s1, 0		    # $s2 = end of a line
	add $t2, $zero, $imm, 255		# $t2 = 255, white color 
	add $t1, $zero, $imm, 1		    # $t1 = 1(write to monitor)
for1:
    bgt $imm, $t0, $s2, for2        # reached end of line, jump to for2
	out $t0, $zero, $imm, 20        # IOreg monitoraddr = pixel i 
	out $t2, $zero, $imm, 21        # IOreg monitordata = 255(white)
	out $t1, $zero, $imm, 22        # IOreg monitorcmd = 1 (write to monitor)
	add $t0, $t0, $imm, 1           # increment i (next pixel)
	beq $imm, $zero, $zero, for1	#jump to next interation (next pixel)
for2:
	bgt $imm, $t0, $a2, end         # reached end of rectangle, jump to end
	add $s1, $s1, $imm, 256         # $s1 = start of a line + 256 (next line)
	add $s2, $s1, $s0, 0			# $s2 = end of a line(next line)
	add $t0, $s1, $zero, 0		    # $t0 = i = start of next line
	beq $imm, $zero, $zero, for1	# jump to next iteration (next row)
end:
	lw $ra, $sp, $imm, 4			# Load return address
	lw $a0, $sp, $imm, 3			# Load vertex A
	lw $a1, $sp, $imm, 2			# Load vertex B
	lw $a2, $sp, $imm, 1			# Load vertex C
	lw $a3, $sp, $imm, 0			# Load vertex D
	add $sp, $sp, $imm, 5			# Free allocated memory
	beq $ra, $zero, $zero, 0		# Return to caller
    
	
	
