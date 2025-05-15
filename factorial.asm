
		  add $s0, $imm, $zero, 1        # initialize s0 to 1
factorial:
	      add $sp, $sp, $imm, -2         # adjust stack for 2 items
		  sw $ra, $sp, $imm, 1           # save return address
		  sw $a0, $sp, $imm, 0           # save argument
		  bne $imm, $a0, $s0, else       # if n!=1 jump to else
		  add $v0, $s0, $zero ,0         # v0 = 1
		  beq $imm, $zero, $zero, end    # jump to end
else:     
		  sub $a0, $a0, imm, 1           # n = n-1
          jal factorial                  # call factorial(n-1)
		  add $a0, $a0, imm, 1           # n = n+1
		  mul $v0, $v0, $a0, 0           # v0 = n * factorial(n-1)
		  beq $imm, $zero, $zero, end    # jump to end
end:	
		 lw $ra, $sp ,$imm, 1			 # load return address
		 lw $a0, $sp, $imm, 0            # load argument of caller
		 add $sp, $sp, $imm, 2			 # free allocated memroy
		 beq $ra, $zero, $zero,0         # return to caller

