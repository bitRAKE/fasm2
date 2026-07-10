  DllEntryPoint:
	mov	eax,[esp+8]
	cmp	eax,DLL_PROCESS_ATTACH
	je	.attach
	test	eax,eax
	jnz	.done
	invoke	DeleteCriticalSection,assembly_lock
	jmp	.done
  .attach:
	mov	eax,[esp+4]
	mov	[module_handle],eax
	invoke	InitializeCriticalSection,assembly_lock
	invoke	DisableThreadLibraryCalls,[module_handle]
  .done:
	mov	eax,1
	retn	12

  pluginit:
	push	ebx esi edi
	mov	ebx,[esp+16]
	test	ebx,ebx
	jz	.failed
	mov	dword [ebx+PLUG_INITSTRUCT.sdkVersion],1
	mov	dword [ebx+PLUG_INITSTRUCT.pluginVersion],2
	mov	dword [ebx+PLUG_INITSTRUCT.pluginName],'fasm'
	mov	word [ebx+PLUG_INITSTRUCT.pluginName+4],'2'

	invoke	GetModuleFileNameA,[module_handle],module_directory,MAX_PATH
	test	eax,eax
	jz	.failed
	cmp	eax,MAX_PATH
	jae	.failed
	lea	edi,[module_directory+eax]
  .find_separator:
	cmp	edi,module_directory
	jbe	.failed
	dec	edi
	cmp	byte [edi],'\'
	jne	.find_separator
	inc	edi
	mov	[path_tail],edi
	mov	byte [edi],0

	mov	esi,module_directory
	mov	edi,config_path
	call	copy_z
	dec	edi
	mov	esi,config_name
	call	copy_z

	invoke	GetPrivateProfileStringA,config_section,include_key,default_include,configured_include,CONFIG_CAPACITY,config_path
	test	eax,eax
	jz	.failed
	mov	[configured_include_length],eax
	call	resolve_include_path
	test	eax,eax
	jz	.failed

	invoke	GetPrivateProfileStringA,config_section,startup_key,empty_string,startup_source,CONFIG_CAPACITY,config_path
	invoke	GetPrivateProfileIntA,config_section,passes_key,100,config_path
	cmp	eax,1
	jae	.minimum_ok
	mov	eax,1
  .minimum_ok:
	cmp	eax,10000
	jbe	.maximum_ok
	mov	eax,10000
  .maximum_ok:
	mov	[configured_maximum_passes],eax
	mov	eax,1
	jmp	.return
  .failed:
	xor	eax,eax
  .return:
	pop	edi esi ebx
	retn

  resolve_include_path:
	mov	esi,configured_include
	mov	al,[esi]
	test	al,al
	jz	.invalid
	cmp	byte [esi+1],':'
	je	.absolute
	cmp	al,'\'
	je	.absolute
	cmp	al,'/'
	je	.absolute
	mov	eax,[path_tail]
	sub	eax,module_directory
	add	eax,[configured_include_length]
	add	eax,12
	cmp	eax,CONFIG_CAPACITY
	jae	.invalid
	mov	edi,include_path
	push	esi
	mov	esi,module_directory
	call	copy_z
	dec	edi
	pop	esi
	call	copy_z
	jmp	.append
  .absolute:
	mov	eax,[configured_include_length]
	add	eax,12
	cmp	eax,CONFIG_CAPACITY
	jae	.invalid
	mov	edi,include_path
	call	copy_z
  .append:
	dec	edi
	cmp	edi,include_path+CONFIG_CAPACITY-12
	jae	.invalid
	cmp	byte [edi-1],'\'
	je	.have_separator
	cmp	byte [edi-1],'/'
	je	.have_separator
	mov	byte [edi],'/'
	inc	edi
  .have_separator:
	mov	esi,fasm2_include
	call	copy_z
	mov	esi,include_path
  .normalize:
	lodsb
	test	al,al
	jz	.valid
	cmp	al,"'"
	je	.invalid
	cmp	al,'\'
	jne	.normalize
	mov	byte [esi-1],'/'
	jmp	.normalize
  .valid:
	mov	eax,1
	retn
  .invalid:
	xor	eax,eax
	retn

  plugstop:
	push	ebx
	invoke	EnterCriticalSection,assembly_lock
	mov	ebx,[plugin_output_region.address]
	test	ebx,ebx
	jz	.no_output
	invoke	VirtualFree,ebx,0,MEM_RELEASE
	and	dword [plugin_output_region.address],0
	and	dword [plugin_output_region.size],0
  .no_output:
	invoke	LeaveCriticalSection,assembly_lock
	pop	ebx
	mov	eax,1
	retn

  CBASSEMBLE:
	push	ebx esi edi
	mov	ebx,[esp+20]
	test	ebx,ebx
	jz	.return
	cmp	byte [ebx+PLUG_CB_ASSEMBLE.handled],0
	jne	.return
	mov	byte [ebx+PLUG_CB_ASSEMBLE.handled],1
	mov	byte [ebx+PLUG_CB_ASSEMBLE.success],0
	and	dword [ebx+PLUG_CB_ASSEMBLE.size],0
	call	clear_error

	invoke	EnterCriticalSection,assembly_lock
	mov	edi,source_buffer
	mov	esi,source_prefix
	call	copy_z_back
	mov	esi,include_path
	call	copy_z_back
	mov	esi,source_middle
	call	copy_z_back
	mov	esi,startup_source
	cmp	byte [esi],0
	je	.no_startup
	call	copy_z_back
	mov	byte [edi],10
	inc	edi
  .no_startup:
	mov	esi,source_format
	call	copy_z_back

	mov	eax,[ebx+PLUG_CB_ASSEMBLE.addr]
	mov	ecx,8
  .address_digit:
	rol	eax,4
	mov	edx,eax
	and	edx,0Fh
	cmp	dl,9
	jbe	.decimal_digit
	add	dl,'A'-10
	jmp	.store_digit
  .decimal_digit:
	add	dl,'0'
  .store_digit:
	mov	[edi],dl
	inc	edi
	loop	.address_digit
	mov	byte [edi],'h'
	inc	edi
	mov	byte [edi],10
	inc	edi

	mov	esi,[ebx+PLUG_CB_ASSEMBLE.instruction]
	test	esi,esi
	jz	.invalid_source
  .copy_instruction:
	cmp	edi,source_buffer+SOURCE_CAPACITY-2
	jae	.source_too_long
	lodsb
	test	al,al
	jz	.source_ready
	cmp	al,10
	je	.invalid_source
	cmp	al,13
	je	.invalid_source
	stosb
	jmp	.copy_instruction
  .source_ready:
	mov	byte [edi],10
	mov	byte [edi+1],0

	invoke	CreatePipe,pipe_read,pipe_write,0,ERROR_PIPE_CAPACITY
	test	eax,eax
	jz	.pipe_failed
	push	dword [pipe_write]
	push	0
	push	0
	push	plugin_output_region
	push	0
	push	source_buffer
	call	fasmg_Assemble
	mov	[assembly_status],eax
	invoke	CloseHandle,[pipe_write]
	and	dword [pipe_write],0
	call	read_diagnostic
	invoke	CloseHandle,[pipe_read]
	and	dword [pipe_read],0

	cmp	dword [assembly_status],0
	jne	.assembly_failed
	mov	ecx,[plugin_output_region.size]
	mov	[ebx+PLUG_CB_ASSEMBLE.size],ecx
	cmp	ecx,[ebx+PLUG_CB_ASSEMBLE.destSize]
	ja	.buffer_too_small
	mov	edi,[ebx+PLUG_CB_ASSEMBLE.dest]
	test	edi,edi
	jz	.assembled
	mov	esi,[plugin_output_region.address]
	rep	movsb
  .assembled:
	mov	byte [ebx+PLUG_CB_ASSEMBLE.success],1
	jmp	.leave
  .assembly_failed:
	mov	edi,[ebx+PLUG_CB_ASSEMBLE.error]
	test	edi,edi
	jz	.leave
	cmp	byte [edi],0
	jne	.leave
	mov	esi,error_assembly
	call	copy_error
	jmp	.leave
  .buffer_too_small:
	mov	esi,error_buffer
	call	copy_error
	jmp	.leave
  .invalid_source:
	mov	esi,error_source
	call	copy_error
	jmp	.leave
  .source_too_long:
	mov	esi,error_length
	call	copy_error
	jmp	.leave
  .pipe_failed:
	mov	esi,error_pipe
	call	copy_error
  .leave:
	invoke	LeaveCriticalSection,assembly_lock
  .return:
	pop	edi esi ebx
	retn

  copy_z:
	lodsb
	stosb
	test	al,al
	jnz	copy_z
	retn

  copy_z_back:
	lodsb
	test	al,al
	jz	.done
	stosb
	jmp	copy_z_back
  .done:
	retn

  clear_error:
	mov	edi,[ebx+PLUG_CB_ASSEMBLE.error]
	test	edi,edi
	jz	.done
	cmp	dword [ebx+PLUG_CB_ASSEMBLE.errorSize],0
	jle	.done
	mov	byte [edi],0
  .done:
	retn

  copy_error:
	mov	edi,[ebx+PLUG_CB_ASSEMBLE.error]
	test	edi,edi
	jz	.done
	mov	ecx,[ebx+PLUG_CB_ASSEMBLE.errorSize]
	test	ecx,ecx
	jle	.done
	dec	ecx
  .copy:
	lodsb
	test	al,al
	jz	.terminate
	test	ecx,ecx
	jz	.terminate
	stosb
	dec	ecx
	jmp	.copy
  .terminate:
	mov	byte [edi],0
  .done:
	retn

  read_diagnostic:
	mov	edi,[ebx+PLUG_CB_ASSEMBLE.error]
	test	edi,edi
	jz	.done
	mov	ecx,[ebx+PLUG_CB_ASSEMBLE.errorSize]
	cmp	ecx,1
	jbe	.done
	dec	ecx
	invoke	ReadFile,[pipe_read],edi,ecx,bytes_read,0
	mov	eax,[bytes_read]
	mov	byte [edi+eax],0
  .done:
	retn
