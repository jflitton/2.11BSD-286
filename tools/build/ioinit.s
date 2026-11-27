	bits	16
	cpu	286
	segment	_TEXT public align=2 class=CODE
	segment	_DATA public align=2 class=DATA
	segment	_BSS public align=2 class=BSS
	group	DGROUP _DATA _BSS
	global	_ioinit_
	segment	_TEXT
_ioinit_:
	enter	386,0
	push	si
	push	di
	lea	si,[bp+4]
	mov	bx,word [si]
	cmp	word [bx],0
	je	L15
	mov	word [_ioiflg_+2],1
	mov	al,byte [L16]
	jmp	LL2
L15:
	mov	word [_ioiflg_+2],0
	mov	al,byte [L18]
LL2:
	mov	byte [bp-9],al
	mov	bx,word [si+2]
	cmp	word [bx],0
	je	L19
	mov	word [_ioiflg_+4],1
	mov	al,byte [L20]
	jmp	LL4
L19:
	mov	word [_ioiflg_+4],0
	mov	al,byte [L22]
LL4:
	mov	byte [bp-10],al
	mov	word [bp-86],0
	mov	word [bp-84],0
	mov	word [bp-82],5
	mov	word [bp-80],0
	mov	word [bp-78],0
	mov	word [bp-66],0
	mov	word [bp-64],0
	mov	word [bp-72],0
	mov	word [bp-70],0
	lea	ax,[bp-9]
	mov	word [bp-68],ax
	lea	ax,[bp-10]
	mov	word [bp-62],ax
	lea	ax,[bp-86]
	push	ax
	call	_f_open
	pop	cx
	mov	word [bp-86],0
	mov	word [bp-84],0
	mov	word [bp-82],6
	mov	word [bp-80],0
	mov	word [bp-78],0
	mov	word [bp-66],0
	mov	word [bp-64],0
	mov	word [bp-72],0
	mov	word [bp-70],0
	lea	ax,[bp-9]
	mov	word [bp-68],ax
	lea	ax,[bp-10]
	mov	word [bp-62],ax
	lea	ax,[bp-86]
	push	ax
	call	_f_open
	pop	cx
	mov	bx,word [si+4]
	cmp	word [bx],0
	je	L23
	mov	word [_ioiflg_],1
	jmp	L24
L23:
	mov	word [_ioiflg_],0
L24:
	mov	word [bp-90],1
	mov	word [bp-94],0
	mov	word [bp-98],0
	push	word [si+12]
	push	word [si+10]
	push	word [si+6]
	call	_i_len
	add	sp,6
	mov	word [v.1],ax
	mov	word [v.1+2],dx
	cmp	dx,0
	jl	L25
	jg	L10000
	cmp	ax,0
	je	L25
L10000:
	cmp	dx,0
	jg	L25
	jl	L10001
	cmp	ax,30
	ja	L25
L10001:
	push	0
	push	1
	push	word [si+12]
	push	word [si+10]
	push	L26
	push	word [si+6]
	call	_s_cmp
	add	sp,12
	test	ax,ax
	je	L25
	mov	word [bp-98],1
	push	0
	push	1
	push	word [si+12]
	push	word [si+10]
	push	L26
	push	word [si+6]
	call	_i_indx
	add	sp,12
	mov	word [v.2],ax
	mov	word [v.2+2],dx
	or	ax,dx
	jne	L28
	mov	ax,word [v.1]
	mov	dx,word [v.1+2]
	add	ax,1
	adc	dx,0
	mov	word [v.2],ax
	mov	word [v.2+2],dx
L28:
	push	word [si+12]
	push	word [si+10]
	push	0
	push	32
	push	word [si+6]
	lea	ax,[bp-130]
	push	ax
	call	_s_copy
	add	sp,12
	mov	bx,word [si+8]
	cmp	word [bx],0
	je	L29
	mov	word [bp-86],0
	mov	word [bp-84],0
	mov	word [bp-82],0
	mov	word [bp-80],0
	mov	word [bp-78],0
	mov	word [bp-76],0
	mov	word [bp-74],L31
	lea	ax,[bp-86]
	push	ax
	call	_s_wsfe
	pop	cx
	mov	ax,word [v.2]
	mov	dx,word [v.2+2]
	add	ax,65535
	adc	dx,65535
	push	dx
	push	ax
	lea	ax,[bp-130]
	push	ax
	push	L32
	call	_do_fio
	add	sp,8
	call	_e_wsfe
L29:
	mov	word [v.3],0
	mov	word [v.3+2],0
L35:
	mov	word [bp-86],0
	mov	word [bp-84],0
	mov	word [bp-70],1
	mov	word [bp-68],0
	mov	ax,word [v.2]
	mov	dx,word [v.2+2]
	add	ax,65535
	adc	dx,65535
	mov	bx,ax
	mov	cx,dx
	mov	ax,32
	xor	dx,dx
	sub	ax,bx
	sbb	dx,cx
	mov	word [bp-74],ax
	mov	word [bp-72],dx
	mov	ax,bp
	add	ax,word [v.2]
	add	ax,-131
	mov	word [bp-82],ax
	mov	word [bp-80],0
	mov	word [bp-78],0
	mov	word [bp-76],L36
	lea	ax,[bp-86]
	push	ax
	call	_s_wsfi
	pop	cx
	push	0
	push	4
	push	v.3
	push	L32
	call	_do_fio
	add	sp,8
	call	_e_wsfi
	push	0
	push	256
	push	0
	push	32
	lea	ax,[bp-386]
	push	ax
	lea	ax,[bp-130]
	push	ax
	call	_getenv_
	add	sp,12
	push	0
	push	1
	push	0
	push	256
	push	L26
	lea	ax,[bp-386]
	push	ax
	call	_s_cmp
	add	sp,12
	test	ax,ax
	je	L33
	mov	word [bp-86],1
	mov	word [bp-84],0
	mov	ax,word [v.3]
	mov	dx,word [v.3+2]
	mov	word [bp-82],ax
	mov	word [bp-80],dx
	mov	word [bp-76],256
	mov	word [bp-74],0
	lea	ax,[bp-386]
	mov	word [bp-78],ax
	mov	word [bp-66],0
	mov	word [bp-64],0
	mov	word [bp-72],0
	mov	word [bp-70],L41
	mov	word [bp-68],L18
	mov	word [bp-62],0
	lea	ax,[bp-86]
	push	ax
	call	_f_open
	pop	cx
	test	ax,ax
	jne	L42
	mov	bx,word [si+8]
	cmp	word [bx],0
	je	L43
	mov	word [bp-86],0
	mov	word [bp-84],0
	mov	word [bp-82],0
	mov	word [bp-80],0
	mov	word [bp-78],0
	mov	word [bp-76],0
	mov	word [bp-74],L45
	lea	ax,[bp-86]
	push	ax
	call	_s_wsfe
	pop	cx
	push	0
	push	4
	push	v.3
	push	L32
	call	_do_fio
	add	sp,8
	push	0
	push	256
	lea	ax,[bp-386]
	push	ax
	call	_lnblnk_
	add	sp,6
	push	dx
	push	ax
	lea	ax,[bp-386]
	push	ax
	push	L32
	call	_do_fio
	add	sp,8
	call	_e_wsfe
L43:
	mov	word [bp-94],1
	jmp	L33
L42:
	mov	word [bp-86],0
	mov	word [bp-84],0
	mov	word [bp-82],0
	mov	word [bp-80],0
	mov	word [bp-78],0
	mov	word [bp-76],0
	mov	word [bp-74],L49
	lea	ax,[bp-86]
	push	ax
	call	_s_wsfe
	pop	cx
	mov	ax,word [v.2]
	mov	dx,word [v.2+2]
	add	ax,1
	adc	dx,0
	push	dx
	push	ax
	lea	ax,[bp-130]
	push	ax
	push	L32
	call	_do_fio
	add	sp,8
	call	_e_wsfe
	push	0
	push	256
	lea	ax,[bp-386]
	push	ax
	call	_lnblnk_
	add	sp,6
	push	dx
	push	ax
	lea	ax,[bp-386]
	push	ax
	call	_perror_
	add	sp,6
	mov	word [bp-90],0
L33:
	add	word [v.3],1
	adc	word [v.3+2],0
	mov	ax,word [v.3]
	mov	dx,word [v.3+2]
	cmp	dx,0
	jl	L35
	jg	L25
	cmp	ax,19
	jbe	L35
L25:
	mov	bx,word [si+8]
	cmp	word [bx],0
	je	L51
	cmp	word [bp-98],0
	je	L52
	cmp	word [bp-94],0
	jne	L52
	mov	word [bp-86],0
	mov	word [bp-84],0
	mov	word [bp-82],0
	mov	word [bp-80],0
	mov	word [bp-78],0
	mov	word [bp-76],0
	mov	word [bp-74],L54
	lea	ax,[bp-86]
	push	ax
	call	_s_wsfe
	pop	cx
	mov	ax,word [v.2]
	mov	dx,word [v.2+2]
	add	ax,65535
	adc	dx,65535
	push	dx
	push	ax
	lea	ax,[bp-130]
	push	ax
	push	L32
	call	_do_fio
	add	sp,8
	call	_e_wsfe
L52:
	mov	word [bp-86],0
	mov	word [bp-84],0
	mov	word [bp-82],0
	mov	word [bp-80],0
	mov	word [bp-78],0
	mov	word [bp-76],0
	mov	word [bp-74],L57
	lea	ax,[bp-86]
	push	ax
	call	_s_wsfe
	pop	cx
	push	0
	push	4
	push	word [si]
	push	L32
	call	_do_fio
	add	sp,8
	push	0
	push	4
	push	word [si+2]
	push	L32
	call	_do_fio
	add	sp,8
	push	0
	push	4
	push	word [si+4]
	push	L32
	call	_do_fio
	add	sp,8
	call	_e_wsfe
	push	L61
	call	_flush_
	pop	cx
L51:
	mov	ax,word [bp-90]
	mov	word [bp-8],ax
	pop	di
	pop	si
	leave
	ret
	segment	_DATA
	align	2,db 0
L16:
	db	112,0
	align	2,db 0
L18:
	db	102,0
	align	2,db 0
L20:
	db	122,0
	align	2,db 0
L22:
	db	110,0
	align	2,db 0
L26:
	db	32,0
	align	2,db 0
L32:
	dw	1,0
	align	2,db 0
L36:
	db	40,105
	db	50,46
	db	50,41
	db	0,0
	align	2,db 0
L41:
	db	115,0
	align	2,db 0
L61:
	dw	0,0
	align	2,db 0
L45:
	db	40,2
	db	105,111
	db	105,110
	db	105,116
	db	58,32
	db	108,111
	db	103,105
	db	99,97
	db	108,32
	db	117,110
	db	105,116
	db	32,2
	db	44,105
	db	50,44
	db	2,32
	db	111,112
	db	101,110
	db	101,100
	db	32,116
	db	111,32
	db	2,44
	db	97,41
	db	0,0
	align	2,db 0
L54:
	db	40,2
	db	105,111
	db	105,110
	db	105,116
	db	58,32
	db	110,111
	db	32,105
	db	110,105
	db	116,105
	db	97,108
	db	105,122
	db	97,116
	db	105,111
	db	110,32
	db	102,111
	db	117,110
	db	100,32
	db	102,111
	db	114,32
	db	2,44
	db	97,41
	db	0,0
	align	2,db 0
L31:
	db	40,2
	db	105,111
	db	105,110
	db	105,116
	db	58,32
	db	105,110
	db	105,116
	db	105,97
	db	108,105
	db	122,105
	db	110,103
	db	32,102
	db	114,111
	db	109,32
	db	2,44
	db	97,44
	db	2,110
	db	110,2
	db	41,0
	align	2,db 0
L49:
	db	40,2
	db	105,111
	db	105,110
	db	105,116
	db	58,32
	db	2,44
	db	97,44
	db	2,32
	db	2,44
	db	36,41
	db	0,0
	align	2,db 0
L57:
	db	40,2
	db	105,111
	db	105,110
	db	105,116
	db	58,32
	db	99,99
	db	116,108
	db	61,2
	db	44,108
	db	44,2
	db	44,32
	db	98,122
	db	114,111
	db	61,2
	db	44,108
	db	44,2
	db	44,32
	db	97,112
	db	110,100
	db	61,2
	db	44,108
	db	41,0
	segment	_BSS
v.2:
	resb	4
v.1:
	resb	4
v.3:
	resb	4
	segment	_DATA
	global	_ioinit_
	align	2,db 0
	extern	_flush_
	extern	_perror_
	extern	_lnblnk_
	extern	_getenv_
	extern	_e_wsfi
	extern	_s_wsfi
	extern	_e_wsfe
	extern	_do_fio
	extern	_s_wsfe
	extern	_s_copy
	extern	_i_indx
	extern	_s_cmp
	extern	_i_len
	extern	_f_open
	common	_ioiflg_ 6:near
