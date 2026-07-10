MAX_PATH = 260

include 'win32a.inc'

format PE console 4.0 large NX
entry start

PLUG_CB_ASSEMBLE.addr = 0
PLUG_CB_ASSEMBLE.instruction = 4
PLUG_CB_ASSEMBLE.dest = 8
PLUG_CB_ASSEMBLE.destSize = 12
PLUG_CB_ASSEMBLE.size = 16
PLUG_CB_ASSEMBLE.error = 20
PLUG_CB_ASSEMBLE.errorSize = 24
PLUG_CB_ASSEMBLE.handled = 28
PLUG_CB_ASSEMBLE.success = 29
PLUG_CB_ASSEMBLE.bytes = 32

section '.text' code readable executable

start:
        invoke  LoadLibraryA,plugin_name
        test    eax,eax
        jz      fail_load
        mov     [plugin_module],eax
        invoke  GetProcAddress,eax,pluginit_name
        test    eax,eax
        jz      fail_export
        mov     [pluginit_address],eax
        invoke  GetProcAddress,[plugin_module],plugstop_name
        test    eax,eax
        jz      fail_export
        mov     [plugstop_address],eax
        invoke  GetProcAddress,[plugin_module],callback_name
        test    eax,eax
        jz      fail_export
        mov     [callback_address],eax

        push    init_info
        call    [pluginit_address]
        add     esp,4
        test    eax,eax
        jz      fail_init

        mov     dword [callback_info+PLUG_CB_ASSEMBLE.instruction],endbr32_text
        mov     dword [callback_info+PLUG_CB_ASSEMBLE.dest],output_bytes
        mov     dword [callback_info+PLUG_CB_ASSEMBLE.destSize],16
        mov     dword [callback_info+PLUG_CB_ASSEMBLE.error],error_text
        mov     dword [callback_info+PLUG_CB_ASSEMBLE.errorSize],256
        call    reset_result
        call    assemble
        cmp     byte [callback_info+PLUG_CB_ASSEMBLE.handled],1
        jne     fail_success
        cmp     byte [callback_info+PLUG_CB_ASSEMBLE.success],1
        jne     fail_success
        cmp     dword [callback_info+PLUG_CB_ASSEMBLE.size],4
        jne     fail_success
        cmp     dword [output_bytes],0FB1E0FF3h
        jne     fail_success

        mov     dword [callback_info+PLUG_CB_ASSEMBLE.addr],401000h
        mov     dword [callback_info+PLUG_CB_ASSEMBLE.instruction],call_text
        call    reset_result
        call    assemble
        cmp     byte [callback_info+PLUG_CB_ASSEMBLE.success],1
        jne     fail_relative
        cmp     dword [callback_info+PLUG_CB_ASSEMBLE.size],5
        jne     fail_relative
        cmp     byte [output_bytes],0E8h
        jne     fail_relative
        cmp     dword [output_bytes+1],1Bh
        jne     fail_relative

        mov     dword [callback_info+PLUG_CB_ASSEMBLE.instruction],endbr32_text
        mov     dword [callback_info+PLUG_CB_ASSEMBLE.destSize],2
        call    reset_result
        call    assemble
        cmp     byte [callback_info+PLUG_CB_ASSEMBLE.handled],1
        jne     fail_buffer
        cmp     byte [callback_info+PLUG_CB_ASSEMBLE.success],0
        jne     fail_buffer
        cmp     dword [callback_info+PLUG_CB_ASSEMBLE.size],4
        jne     fail_buffer

        mov     dword [callback_info+PLUG_CB_ASSEMBLE.instruction],invalid_text
        mov     dword [callback_info+PLUG_CB_ASSEMBLE.destSize],16
        call    reset_result
        call    assemble
        cmp     byte [callback_info+PLUG_CB_ASSEMBLE.handled],1
        jne     fail_invalid
        cmp     byte [callback_info+PLUG_CB_ASSEMBLE.success],0
        jne     fail_invalid
        cmp     byte [error_text],0
        je      fail_invalid

        mov     dword [callback_info+PLUG_CB_ASSEMBLE.size],123
        mov     byte [callback_info+PLUG_CB_ASSEMBLE.handled],1
        mov     byte [callback_info+PLUG_CB_ASSEMBLE.success],1
        call    assemble
        cmp     dword [callback_info+PLUG_CB_ASSEMBLE.size],123
        jne     fail_arbitration
        cmp     byte [callback_info+PLUG_CB_ASSEMBLE.success],1
        jne     fail_arbitration

        call    [plugstop_address]
        invoke  FreeLibrary,[plugin_module]
        invoke  ExitProcess,0

reset_result:
        and     dword [callback_info+PLUG_CB_ASSEMBLE.size],0
        mov     byte [callback_info+PLUG_CB_ASSEMBLE.handled],0
        mov     byte [callback_info+PLUG_CB_ASSEMBLE.success],0
        mov     byte [error_text],0
        retn

assemble:
        push    callback_info
        push    0
        call    [callback_address]
        add     esp,8
        retn

fail_load:
        mov     eax,1
        jmp     failed
fail_export:
        mov     eax,2
        jmp     failed
fail_init:
        mov     eax,3
        jmp     failed
fail_success:
        mov     eax,4
        jmp     failed
fail_buffer:
        mov     eax,5
        jmp     failed
fail_invalid:
        mov     eax,6
        jmp     failed
fail_relative:
        mov     eax,7
        jmp     failed
fail_arbitration:
        mov     eax,8
failed:
        invoke  ExitProcess,eax

section '.data' data readable writeable

plugin_module dd 0
pluginit_address dd 0
plugstop_address dd 0
callback_address dd 0
init_info rb 268
callback_info rb PLUG_CB_ASSEMBLE.bytes
output_bytes rb 16
error_text rb 256

plugin_name db 'fasm2.dp32',0
pluginit_name db 'pluginit',0
plugstop_name db 'plugstop',0
callback_name db 'CBASSEMBLE',0
endbr32_text db 'endbr32',0
call_text db 'call 401020h',0
invalid_text db 'definitely_not_an_instruction',0

section '.rdata' data readable

data import
        library kernel32,'KERNEL32.DLL'
        import kernel32,\
               ExitProcess,'ExitProcess',\
               FreeLibrary,'FreeLibrary',\
               GetProcAddress,'GetProcAddress',\
               LoadLibraryA,'LoadLibraryA'
end data

section '.reloc' fixups data readable discardable
