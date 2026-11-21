; Every instruction the assembler has (usr/src/bin/as/i286), in each of
; the ways its operands may be written; and its directives.  tests/as
; assembles this with it and with NASM, which are to make the same of
; it: nothing here is meant to be run.

	bits	16
	cpu	286
	segment	_TEXT public align=2 class=CODE
	segment	_DATA public align=2 class=DATA
	segment	_BSS public align=2 class=BSS
	group	DGROUP _DATA _BSS

	extern	_ext, _fn, _far
	global	_start, _var, _buf
	common	_com 10:near

SIX	equ	6
MIN	equ	-3
BIG	equ	0x1234
SUM	equ	SIX*4+2

	segment	_TEXT
_start:
; no operands
	aaa
	aad
	aam
	aas
	cbw
	clc
	cld
	cli
	clts
	cmc
	cmpsb
	cmpsw
	cwd
	daa
	das
	hlt
	insb
	insw
	int3
	into
	iret
	lahf
	leave
	lodsb
	lodsw
	movsb
	movsw
	nop
	outsb
	outsw
	popa
	popf
	pusha
	pushf
	sahf
	scasb
	scasw
	stc
	std
	sti
	stosb
	stosw
	wait
	xlatb
	rep movsw
	repe cmpsb
	repne scasb
	repz cmpsw
	repnz scasw
	lock inc word [bx]

; add or adc sbb and sub xor cmp
	add	ax,bx
	add	bx,ax
	add	al,bl
	add	ch,dh
	add	ax,[bx]
	add	[bx],ax
	add	al,[si]
	add	[di],cl
	add	ax,1
	add	ax,-1
	add	ax,127
	add	ax,128
	add	ax,-128
	add	ax,-129
	add	ax,1000
	add	ax,0xFFFF
	add	ax,0xFF80
	add	ax,0xFF7F
	add	bx,1
	add	bx,1000
	add	al,5
	add	bl,5
	add	bl,-1
	add	byte [bx],5
	add	word [bx],5
	add	word [bx],500
	add	word [_var],5
	add	word [_var],500
	add	word [_var],_buf
	add	ax,_var
	add	bx,_var+4
	add	sp,8
	add	sp,200
	or	ax,cx
	or	word [bp-4],0x80
	or	byte [bp+4],0x80
	or	ah,0x0C
	adc	dx,0
	adc	word [bp-2],dx
	adc	dx,[si+2]
	sbb	dx,cx
	sbb	dx,237
	sbb	word [_var+2],0
	and	ax,255
	and	ax,-256
	and	ax,-2
	and	al,0xFE
	and	byte [di+_buf],127
	and	word [bx+si],SIX
	sub	ax,19968
	sub	si,ax
	sub	sp,ax
	sub	word [bp+di],MIN
	xor	ax,ax
	xor	ah,ah
	xor	si,1
	xor	byte [bx],32
	xor	word [bp-6],BIG
	cmp	ax,10
	cmp	ax,_var
	cmp	al,10
	cmp	dx,seg _var
	cmp	word [_var],0
	cmp	word [_var],-1
	cmp	word [_var],_buf+2
	cmp	byte [bx],0
	cmp	byte [si-1],'a'
	cmp	si,_buf+10
	cmp	ax,[bp+6]
	cmp	[bp+6],ax
	cmp	cx,dx
	cmp	byte [es:bx],1
	cmp	word [es:bx+4],1

; mov
	mov	ax,bx
	mov	si,di
	mov	bp,sp
	mov	al,ah
	mov	cl,bl
	mov	ax,1
	mov	ax,-1
	mov	bx,BIG
	mov	di,_buf
	mov	si,_buf+3
	mov	dx,seg _var
	mov	al,1
	mov	bh,255
	mov	ax,[_var]
	mov	[_var],ax
	mov	al,[_var]
	mov	[_var],al
	mov	ax,[es:_var]
	mov	ax,[_var+2]
	mov	bx,[_var]
	mov	[_var],bx
	mov	cl,[_var]
	mov	[_var],cl
	mov	ax,[bx]
	mov	ax,[bx+si]
	mov	ax,[bx+di]
	mov	ax,[bp+si]
	mov	ax,[bp+di]
	mov	ax,[si]
	mov	ax,[di]
	mov	ax,[bp]
	mov	ax,[bx+0]
	mov	ax,[bx+1]
	mov	ax,[bx-1]
	mov	ax,[bx+127]
	mov	ax,[bx+128]
	mov	ax,[bx-128]
	mov	ax,[bx-129]
	mov	ax,[bx+1000]
	mov	ax,[bx+_var]
	mov	ax,[bx+_var+2]
	mov	ax,[bx+si+_var-2]
	mov	ax,[bp+SIX]
	mov	ax,[bp+2+SIX+8]
	mov	ax,[bp-SIX*2]
	mov	ax,[si+_buf]
	mov	ax,[cs:bx]
	mov	ax,[ss:si+2]
	mov	ax,[ds:bp]
	mov	ax,word [bp-4]
	mov	word [bp-4],ax
	mov	[bp-4],si
	mov	[bx],al
	mov	byte [bx],0
	mov	word [bx],0
	mov	word [bx],_var
	mov	word [bp-8],1000
	mov	word [_var],0
	mov	byte [_var+1],7
	mov	word [es:bx],5
	mov	ax,cs
	mov	ax,ds
	mov	bx,es
	mov	dx,ss
	mov	ds,ax
	mov	es,bx
	mov	ss,dx
	mov	[bx+4],ds
	mov	[bx+6],cs
	mov	es,[bp+4]
	mov	ds,[_var]

; test, xchg, lea and the like
	test	ax,ax
	test	al,al
	test	si,di
	test	ax,1
	test	al,1
	test	bx,1
	test	bl,1
	test	si,-2
	test	word [bx],1
	test	byte [bx],1
	test	byte [bx+_buf+1],8
	test	word [_var],0x8000
	test	ax,[bp-4]
	test	[bp-4],ax
	test	cl,[si]
	xchg	ax,bx
	xchg	bx,ax
	xchg	ax,cx
	xchg	cx,dx
	xchg	al,ah
	xchg	dl,dh
	xchg	ax,[bx]
	xchg	[bx],ax
	xchg	cl,[bx]
	xchg	si,[bp-2]
	lea	ax,[bp-4]
	lea	si,[bx+_buf]
	lea	sp,[bp-4]
	lea	di,[bx+si+2]
	les	bx,[bp+6]
	lds	si,[_var]
	bound	ax,[bx]

; push and pop
	push	ax
	push	cx
	push	dx
	push	bx
	push	sp
	push	bp
	push	si
	push	di
	push	es
	push	cs
	push	ss
	push	ds
	push	0
	push	1
	push	-1
	push	127
	push	128
	push	-128
	push	-129
	push	1000
	push	_var
	push	_buf+2
	push	seg _var
	push	word 5
	push	word [bx]
	push	word [bp+4]
	push	word [_var]
	push	word [bx+_buf-2]
	pop	ax
	pop	cx
	pop	dx
	pop	bx
	pop	bp
	pop	si
	pop	di
	pop	es
	pop	ss
	pop	ds
	pop	word [bx]
	pop	word [_var]

; one operand
	inc	ax
	inc	di
	inc	al
	inc	byte [bx]
	inc	word [bx]
	inc	word [_var]
	inc	word [bp-2]
	dec	cx
	dec	bh
	dec	byte [_var]
	dec	word [si+4]
	not	ax
	not	bl
	not	word [bx]
	neg	dx
	neg	byte [bx]
	neg	word [bp-2]
	mul	bx
	mul	cl
	mul	word [bp]
	mul	word [bp+2]
	mul	byte [bx]
	imul	cx
	imul	word [_var]
	imul	ax,ax,10
	imul	ax,ax,-10
	imul	ax,bx,1000
	imul	si,word [bp-4],6
	imul	di,[_var],300
	imul	bx,12
	imul	bx,1200
	div	cx
	div	word [bp-2]
	div	bl
	idiv	cx
	idiv	word [_var+2]
	idiv	byte [si]

; shifts
	shl	ax,1
	shl	ax,2
	shl	ax,cl
	shl	bl,1
	shl	bl,3
	shl	bl,cl
	shl	word [bx],1
	shl	word [bx],4
	shl	byte [bx],cl
	sal	dx,1
	shr	ax,1
	shr	cx,8
	sar	dx,1
	sar	ax,cl
	rol	ax,1
	ror	bx,3
	rcl	dx,1
	rcr	ax,1
	rcr	word [bp-2],1
	align	2
after:	nop
	align	4
	nop

; jumps and calls
back:	jmp	back
	jmp	fwd
	jmp	short fwd
	jmp	near fwd
	je	back
	jne	fwd
	jz	fwd
	jnz	fwd
	ja	fwd
	jae	fwd
	jb	fwd
	jbe	fwd
	jc	fwd
	jnc	fwd
	jg	fwd
	jge	fwd
	jl	fwd
	jle	fwd
	jna	fwd
	jnae	fwd
	jnb	fwd
	jnbe	fwd
	jng	fwd
	jnge	fwd
	jnl	fwd
	jnle	fwd
	jo	fwd
	jno	fwd
	jp	fwd
	jnp	fwd
	jpe	fwd
	jpo	fwd
	js	fwd
	jns	fwd
	jcxz	fwd
	loop	back
	loope	back
	loopne	back
	loopz	fwd
	loopnz	fwd
fwd:	jmp	far2
	je	far2
	jne	back
	jmp	_ext
	je	_ext
	jmp	_ext+2
	jmp	ax
	jmp	bx
	jmp	[bx]
	jmp	word [bx+2]
	jmp	[cs:bx+table]
	jmp	far [bx]
	jmp	far [bp-4]
	jmp	far _far
	call	back
	call	fwd
	call	_fn
	call	_fn+1
	call	far _far
	call	far _start
	call	ax
	call	di
	call	[bx]
	call	word [bp+4]
	call	word [_var]
	call	far [bp+4]
	call	far [_var]
	ret
	ret	4
	retn
	retf
	retf	6
	int	0x30
	int	3
	in	al,dx
	in	ax,dx
	in	al,0x60
	in	ax,0x40
	out	dx,al
	out	dx,ax
	out	0x20,al
	out	0x40,ax
	enter	0,0
	enter	16,0
	enter	300,1
	times 130 nop
far2:	jmp	back
	jne	back
	je	fwd
.loc:	jmp	.loc
	je	.loc
	jmp	$
table:	dw	back, fwd, far2
	dw	far2.loc
; far to a segment that is a number, as a boot block does; and a times
; that fills to a place, after a jump that is made long
	jmp	0:back
	jmp	0x40:0x7C00
	call	0x1000:fwd
	call	0:0
pad:	jne	back
	times	16-($-pad) db 0x90
	jmp	pad

; the protected mode's
	lgdt	[bx]
	lidt	[bx+6]
	sgdt	[_var]
	sidt	[bp-6]
	lldt	ax
	lldt	[bx]
	ltr	bx
	sldt	cx
	str	dx
	lmsw	ax
	smsw	bx
	smsw	[bx]
	verr	ax
	verw	[bx]
	lar	ax,bx
	lsl	cx,[bx]
	arpl	ax,bx
	arpl	[bx],cx

; the 287's
	fld	dword [bx]
	fld	qword [bx]
	fld	tword [bx]
	fld	qword [_var]
	fld	qword [bp+2+SIX]
	fld	st0
	fld	st3
	fst	dword [bp-4]
	fst	qword [bx]
	fst	st2
	fstp	dword [bp-4]
	fstp	qword [bx]
	fstp	tword [bx]
	fstp	st1
	fild	word [bx]
	fild	dword [bx]
	fild	qword [bx]
	fist	word [bx]
	fist	dword [bp-4]
	fistp	word [bx]
	fistp	dword [bx]
	fistp	qword [bx]
	fadd	dword [bx]
	fadd	qword [_var]
	fadd	st1
	fadd	st0,st2
	fadd	st2,st0
	fmul	dword [bp-4]
	fmul	qword [bp+4]
	fmul	st0,st1
	fmul	st3,st0
	fcom	dword [bx]
	fcom	qword [bx]
	fcom	st1
	fcomp	dword [bp-4]
	fcomp	qword [bp+4]
	fcomp	st2
	fsub	dword [bx]
	fsub	qword [bx]
	fsub	st0,st1
	fsub	st1,st0
	fsubr	qword [bx]
	fsubr	st0,st1
	fsubr	st1,st0
	fdiv	dword [bx]
	fdiv	qword [_var]
	fdiv	st0,st1
	fdiv	st1,st0
	fdivr	qword [bx]
	fdivr	st0,st1
	fdivr	st1,st0
	faddp	st1
	faddp	st2,st0
	fmulp	st1
	fsubp	st1
	fsubrp	st1
	fdivp	st1
	fdivrp	st1
	fiadd	word [bx]
	fiadd	dword [bx]
	fimul	word [bx]
	ficom	word [bx]
	ficomp	dword [bx]
	fisub	word [bx]
	fisubr	dword [bx]
	fidiv	word [bx+2]
	fidiv	dword [bp-4]
	fidivr	word [bx]
	fxch
	fxch	st2
	ffree	st1
	fldcw	[bp-2]
	fstcw	[bp-2]
	fnstcw	[bp-4]
	fstsw	ax
	fnstsw	ax
	fstsw	[bp-2]
	fnstsw	[bx]
	fldenv	[bx]
	fstenv	[bx]
	fnstenv	[bx]
	fsave	[bx]
	fnsave	[bx]
	frstor	[bx]
	f2xm1
	fabs
	fchs
	fclex
	fnclex
	fcompp
	fdecstp
	fincstp
	finit
	fninit
	fld1
	fldl2e
	fldl2t
	fldlg2
	fldln2
	fldpi
	fldz
	fnop
	fpatan
	fprem
	fptan
	frndint
	fscale
	fsqrt
	ftst
	fwait
	fxam
	fxtract
	fyl2x
	fyl2xp1

; data
	segment	_DATA
_var:	dw	1, 2
	db	1
	align	2,db 0
	db	'a', "string", 0, 10
	db	255, -1, SIX, MIN
	dw	0, -1, 65535, BIG, SUM, 'ab'
	dw	_var, _var+2, _buf, _buf+4, _ext, _ext+6, _start, after
	dw	_start, seg _start
	dw	_far, seg _far
	dw	here-_var, $-_var, $-$$
here:	dd	0, 1, 0x12345678, -1
	times 7 db 0
	times 3 dw 0x55AA
	resb	3
	align	4,db 0
	dw	(1+2)*3, 100/7, 100 % 7, 1<<4, 0x100>>4, 0xF0|0x0F, 0xFF&0x3C, 5^1, ~0
	dw	0x10, 10h, 0FFh, 0xabcd
; the compiler's labels, which are kept by their numbers, and names like
; them that are not its
L7:	dw	L7, L007, LL7, L65536, L0, L65535
L007:	dw	1
LL7:	dw	2
L65536:	dw	3
L0:	dw	4
L65535:	dw	5

	segment	_BSS
_buf:	resb	10
	resw	4
	resd	2
	align	2,resb 1
last:	resb	1
