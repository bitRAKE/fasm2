; EXACT_MATCH CheckSum cross-validation vectors.
;
; Each section below reproduces, byte for byte, a COMDAT section that
; clang-cl (LLVM 22, /Gy /O2 /GS-) emitted from:
;
;   int f0(void){return 42;}
;   int f1(int x){return x*x+7;}
;   long long f2(long long a,long long b){return a*b-3;}
;   __declspec(selectany) int  g_pool[5] = {1,2,3,4,5};
;   __declspec(selectany) char g_str[13] = "hello, world";
;
; The comment on each section is the aux CheckSum clang wrote for it,
; read back with `llvm-readobj --symbols`. Assembling this file and
; dumping the symbols must show the identical CheckSum on each fasm2
; section - confirming the CRC-32 parameters (reflected poly 0xEDB88320,
; seed 0, no final inversion) match LLVM's, independent of any generation
; detail, since the raw bytes are identical.
;
;   llvm-readobj --symbols crc_vectors.obj
;
; This is a positive run-42 test too: main calls f0 (returns 42).
;
; The .data$ sections are READABLE WRITEABLE, as clang emits selectany
; data - and because the linker's '.data' output group is read+write, so
; a read-only contribution draws LNK4078 (attribute mismatch). The
; checksum covers section CONTENTS only; attributes do not affect it.

format MS64 NEWCOFF

section '.text$f0' code readable executable comdat exactmatch	; clang CheckSum 0x7F8535B7
public f0
f0:	db 0B8h,02Ah,000h,000h,000h,0C3h		; mov eax,42 / ret

section '.text$f1' code readable executable comdat exactmatch	; clang CheckSum 0xD940D5A4
public f1
f1:	db 00Fh,0AFh,0C9h,08Dh,041h,007h,0C3h

section '.text$f2' code readable executable comdat exactmatch	; clang CheckSum 0x186DBB85
public f2
f2:	db 048h,00Fh,0AFh,0CAh,048h,08Dh,041h,0FDh,0C3h

section '.data$pool' data readable writeable comdat exactmatch		; clang CheckSum 0x26CD321D
public pool
pool:	dd 1,2,3,4,5

section '.data$str' data readable writeable comdat exactmatch		; clang CheckSum 0x1B85DBCF
public strvec
strvec:	db 'hello, world',0

section '.text$main' code readable executable
public mainCRTStartup
mainCRTStartup:
	sub	rsp,40
	call	f0
	add	rsp,40
	ret
