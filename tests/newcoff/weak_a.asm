; weak_a: the real implementation
format MS64 NEWCOFF
section '.text$impl' code readable executable comdat align 16
public real_get
real_get:
	mov	eax, 97
	ret
