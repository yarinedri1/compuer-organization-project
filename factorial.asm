# Set n = 5 at memory address 0x100
		.word 0x100 5                # MEM[0x100] = 5 (input n)

		lw   $a0, $zero, $imm, 0x100     # Load n from MEM[0x100] into $a0
		add $sp, $zero, $imm, 200      # set sp as 200 - large, safe value
		add  $s0, $imm, $zero, 1         # $s0 = 1
		jal  $ra, $imm, $zero, factorial # Call factorial
		sw   $v0, $zero, $imm, 0x101     # Store result in MEM[0x101]
		halt $zero, $zero, $zero, 0      # Stop
factorial:
	    add $sp, $sp, $imm, -2         # adjust stack for 2 items
		sw $ra, $sp, $imm, 1           # save return address
		sw $a0, $sp, $imm, 0           # save argument n
		bne $imm, $a0, $s0, else       # if n != 1 jump to else
		add $v0, $s0, $zero ,0         # v0 = 1 (return 1, base case)
		beq $imm, $zero, $zero, end    # jump to end
else:     
		sub $a0, $a0, $imm, 1           # n = n-1
        jal  $ra, $imm, $zero, factorial   # call factorial(n-1)
		add $a0, $a0, $imm, 1           # n = n+1 (restore n)
		mul $v0, $v0, $a0, 0           # v0 = n * factorial(n-1)
		beq $imm, $zero, $zero, end    # jump to end
end:	
		lw $ra, $sp ,$imm, 1			# load return address
		lw $a0, $sp, $imm, 0            # load argument of caller
		add $sp, $sp, $imm, 2			# free allocated memroy
		beq $ra, $zero, $zero, 0         # return to caller
