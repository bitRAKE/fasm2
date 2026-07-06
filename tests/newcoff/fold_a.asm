; fold_a: defines tab (exactmatch) and a function returning tab[10] via its own reference
format MS64 NEWCOFF
public get_a
section '.rdata$tab' data readable comdat exactmatch align 16
public tab
tab db '0123456789abcdef'
section '.text$get_a' code readable executable comdat align 16
get_a:
	lea	rax, [tab]
	movzx	eax, byte [rax + 10]
	ret
