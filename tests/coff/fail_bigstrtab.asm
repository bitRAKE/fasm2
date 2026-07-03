; Push the string table past 9,999,999 bytes so a long section name's
; "/nnnnnnnn" header reference needs 8 digits + '/' = 9 characters,
; which cannot fit the 8-byte section Name field.

format MS64 COFF

repeat 172000
    repeat 1, idx:%
	public 0 as 'a_long_padding_symbol_name_qqqqqqqqqqqqqqqqqqqqqqqq_' bappend `idx
    end repeat
end repeat

section '.rdata$longname' data readable comdat
public marker
marker dd 42

section '.text$main' code readable executable
public mainCRTStartup
mainCRTStartup:
	mov	eax,[marker]
	ret
