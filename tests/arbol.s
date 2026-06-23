_start:
	li sp, 0x10000
	li a0, 0x1000
	li t1, 6
	sw t1, 0(a0)
	li t1, 4
	sw t1, 4(a0)
	li t1, 4
	sw t1, 8(a0)
	li t1, 1
	sw t1, 12(a0)
	li t1, 2
	sw t1, 16(a0)
	li t1, 2
	sw t1, 20(a0)
	li t1, 1
	sw t1, 24(a0)
	li a1, 7
	call is_symmetric
_done:
	j _done
is_symmetric:
	bne a0, zero, no_early_return
	li a0, 1
	ret
no_early_return:
	li a2, 1
	li a3, 2
	j solve
solve:
	blt a2, a1, no_early_return_1
	li a0, 1
	ret
no_early_return_1:
	blt a3, a1, no_early_return_2
	li a0, 0
	ret
no_early_return_2:
	slli t0, a2, 2
	add t0, t0, a0
	lw t0, 0(t0)
	slli t1, a3, 2
	add t1, t1, a0
	lw t1, 0(t1)
	beq t0, t1, cond_1_true
	li a0, 0
	ret
cond_1_true:
	addi sp, sp, -32
	sw ra, 28(sp)
	sw s0, 24(sp)
	sw s1, 20(sp)
	sw s2, 16(sp)
	sw s3, 12(sp)
	add a2, a2, a2
	addi a2, a2, 1
	add a3, a3, a3
	addi a3, a3, 2
	mv s0, a0
	mv s1, a1
	mv s2, a2
	mv s3, a3
	call solve
	bne a0, zero, cond_2_true
	j epilogue
cond_2_true:
	mv a0, s0
	mv a1, s1
	addi a2, s2, 1
	addi a3, s3, -1
	call solve
epilogue:
	lw ra, 28(sp)
	lw s0, 24(sp)
	lw s1, 20(sp)
	lw s2, 16(sp)
	lw s3, 12(sp)
	addi sp, sp, 32
	ret
