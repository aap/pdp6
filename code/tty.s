TTY=120

AC0==0
AC1==1
AC2==2
PDP==17

INTERNAL PUTC,PUTS

PUTC:
	CONSZ	TTY,20		; wait until not busy
	 JRST	.-1
	DATAO	TTY,AC1		; transfer character
	POPJ	PDP,

;	CAIE	AC1,12		; return unless LF
;	 POPJ	PDP,
;	MOVEI	AC1,15		; put CR and two DEL
;	PUSHJ	PDP,PUTC
;	MOVEI	AC1,177
;	PUSHJ	PDP,PUTC
;	PUSHJ	PDP,PUTC
;	POPJ	PDP,

PUTS:
	ILDB	AC1,AC2
	SKIPN	AC1
	 POPJ	PDP,
	PUSHJ	PDP,PUTC
	JRST	PUTS

INTERNAL GETCH,GETC

GETCH:
	CONSO	TTY,40		; wait for flag
	 JRST	.-1
	DATAI	TTY,AC1		; get character
	ANDI	AC1,177
	POPJ	PDP,

GETC:
	PUSHJ	PDP,GETCH
	PUSHJ	PDP,PUTC
	POPJ	PDP,