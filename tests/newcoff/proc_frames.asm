; Backend-only regression: multiple USES registers, locals, and nested frames.
; The runner supplies NEWCOFF.DEBUG=1 and 6 separately.
format binary as 'obj'
format MS64 NEWCOFF
include 'win64a.inc'
prologue@proc equ newcoff_debug_prologue
epilogue@proc equ static_rsp_epilogue
close@proc equ newcoff_debug_close
newcoff_debug_procs

extrn frame_probe
public frame_fixture
section '.text' code readable executable align 16
proc frame_fixture uses rbx rsi rdi r12, seed
    locals
        saved_seed dq ?
        scratch rb 256
    endl
    mov [saved_seed],rcx
    mov rbx,11
    mov rsi,22
    mov rdi,33
    mov r12,44
    fastcall frame_probe
    mov rax,[saved_seed]
    add rax,rbx
    add rax,rsi
    add rax,rdi
    add rax,r12
    ret
endp

