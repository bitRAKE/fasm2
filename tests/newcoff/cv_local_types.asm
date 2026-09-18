; Storage-width defaults are shared by PROC, LOCALS and untyped cvlocal.
; The runner supplies NEWCOFF.DEBUG=1 and 6; no executable behavior changes.
format binary as 'obj'
format MS64 NEWCOFF
include 'win64a.inc'
prologue@proc equ newcoff_debug_prologue
epilogue@proc equ static_rsp_epilogue
close@proc equ newcoff_debug_close
newcoff_debug_procs

section '.text' code readable executable align 16
public packed_homes
public manual_types
proc packed_homes uses rbx rsi, a:4,b:4,c:4,d:4,e:4,f:4,g:4,h:4,fifth
    locals
        local_dword dd ?
        local_qword dq ?
    endl
    assert h+4-(parmbase@proc) = 32
    assert fifth-(parmbase@proc) = 32
    ret
endp

manual_types:
cvproc manual_types
namespace manual_types
    virtual at rsp+8
        default_dword dd ?
        explicit_signed dd ?
        explicit_qword dd ?
        default_qword dq ?
        pointer dq ?
        legacy_dword dd ?
        legacy_qword dq ?
    end virtual
    cvlocal default_dword, default_qword
    cvlocal explicit_signed:CV_T_INT4, explicit_qword:CV_T_UQUAD
    cvlocal pointer:CV_T_64PVOID
end namespace
cv_proc_params manual_types, legacy_dword:4, legacy_qword
; Non-strict harvesting must continue to skip missing and non-RSP labels.
cvlocal_auto absent_local, 'absent_local', 0
cvlocal_auto manual_types, 'not_rsp', 0
    ret
cvendp
