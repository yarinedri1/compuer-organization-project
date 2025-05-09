sort:
	 add $sp, $sp, imm, -5              #adjust stack for 5 items
	 sw $ra, $sp, imm, 4                #save return address
	 sw $s3, $sp, imm, 3                #save s3
	 sw $s2, $sp, imm, 2                #save s2
	 sw $s1, $sp, imm, 1                #save s1
	 sw $s0, $sp, imm, 0                #save s0
	 add $s2, $a0, $zero, 0             #$s2 = arr[0]
	 add $s3, $a1, $zero, 0             #$s3 = n
	 add $s0, $zero, $zero,0            #s0 = i = 0
for1:
	 bgt $imm, $s0, $s3, exit1          #if i > n, exit1
	 add $s1, $s0, imm, -1 	            #s1 = j = i - 1
for2:
         blt $imm, $s1, $zero, exit2        #if j < 0, exit2
	 add $t1, $s1, $zero, 0             #$t1 = j
	 add $t2 ,$t1, $s2, 0               #$t2 = arr + j
	 lw $t3, $t2, $imm, 0               #$t3 = arr[j]
	 lw $t4, $t2, $imm, 1               #$t4 = arr[j+1]
	 bge $imm, $t4, $t3, exit2          #if arr[j] >= arr[j+1], exit2
	 add $a0, $s2, $zero, 0             #$a0 = arr
	 add $a1, $s1, $zero, 0             #$a1 = j
	 jal swap                           #call swap procedure
	 add $s1, $s1, imm, -1              #j = j - 1
	 beq $imm, $zero, $zero, for2       #jump for2
exit2:
          add $t0, $zero, $imm , 1		#$t0 = 1
	  add, $s0, $s0, $imm, 1 		#$i = i + 1
	  beq $imm, $zero, $zero, for1		#jump for1

exit1:
	 lw $s0, $sp, imm, 0                #restore s0
	 lw $s1, $sp, imm, 1                #restore s1
	 lw $s2, $sp, imm, 2                #restore s2
	 lw $s3, $sp, imm, 3                #restore s3
	 lw $ra, $sp, imm, 4                #restore return address
	 add $sp, $sp, imm, 5               #restore stack pointer
	 jr $ra                             #return to caller

swap: 
	add $t0, $a0, $a1, 0		   #$t0 = $a0 + $a1 = arr[i]
	lw $t1, $t0, $imm, 0 	           #$t1 = arr[i] 				
	lw $t2, $t0, $imm, 1 	           #$t2 = arr[i+1] 
	sw $t2, $t0, $imm, 0 	           #arr[i] = arr[i+1]
	sw $t1, $t0, $imm, 1 	           #arr[i+1] = arr[i]
	jr $ra                             #return to caller


