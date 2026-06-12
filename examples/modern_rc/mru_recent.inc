; mru_recent.inc - a single binary MRU instance for recent files.
;
; Identity is the lower-case FNV-1a hash plus a full lstrcmpiW path check.
; Updates use find -> del -> add so changed payload is actually written.

struct RecentFileBlob
	path_hash	dq ?
	sel_start	dd ?
	sel_end		dd ?
	path		rw MAX_PATH
ends

define __GLOBAL_DATA__ mru_recent_data
define __GLOBAL_BSS__ mru_recent_bss

macro mru_recent_data
	recent_subkey du APP_REGISTRY_ROOT,'\Recent',0

	align 8
	recent_info MRUINFO cbSize: sizeof.MRUINFO,\
		uMax: ID_RECENT_COUNT,\
		fFlags: MRU_BINARY or MRU_CACHEWRITE,\
		hKey: HKEY_CURRENT_USER,\
		lpszSubKey: recent_subkey,\
		lpfnCompare: RecentCompare

	recent_files MRUDESC info_ptr: recent_info,\
		handle: 0,\
		blob_size: sizeof.RecentFileBlob,\
		is_wide: 1
purge mru_recent_data
end macro

macro mru_recent_bss
	align 8
	recent_tmp	RecentFileBlob
	recent_slot	dd ?
purge mru_recent_bss
end macro

proc RecentCompare uses rbx rsi
	mov	rbx,rcx
	mov	rsi,rdx
	mov	rax,[rbx+RecentFileBlob.path_hash]
	cmp	rax,[rsi+RecentFileBlob.path_hash]
	jb	.less
	ja	.greater
	lea	rcx,[rbx+RecentFileBlob.path]
	lea	rdx,[rsi+RecentFileBlob.path]
	invoke	lstrcmpiW,rcx,rdx
	ret
  .less:
	mov	eax,-1
	ret
  .greater:
	mov	eax,1
	ret
endp

proc SaveCurrentToRecent uses rdi
    locals
	blob RecentFileBlob
	slot dd ?
    endl

	cmp	qword [recent_files.handle],0
	je	.done
	fastcall wcs_is_empty,current_path
	test	eax,eax
	jnz	.done

	lea	rdi,[blob]
	mov	ecx,sizeof.RecentFileBlob shr 3
	xor	eax,eax
	rep	stosq

	fastcall fnv1a_w,current_path
	mov	[blob.path_hash],rax
	fastcall wcscpy_n,addr blob.path,current_path,MAX_PATH

	invoke	SendMessage,[hEdit],EM_GETSEL,addr blob.sel_start,addr blob.sel_end

	invoke	pMRU_FindMRUData,[recent_files.handle],addr blob,\
		sizeof.RecentFileBlob,addr slot
	cmp	eax,0
	jl	.add
	invoke	pMRU_DelMRUString,[recent_files.handle],eax
  .add:
	invoke	pMRU_AddMRUData,[recent_files.handle],addr blob,\
		sizeof.RecentFileBlob
  .done:
	xor	eax,eax
	ret
endp

proc PopulateRecentMenu uses rbx
    locals
	blob RecentFileBlob
    endl

	cmp	qword [hRecentMenu],0
	je	.done
	fastcall ClearMenuItems,[hRecentMenu],0

	cmp	qword [recent_files.handle],0
	je	.empty

	xor	ebx,ebx
  .loop:
	cmp	ebx,ID_RECENT_COUNT
	jge	.after_loop
	invoke	pMRU_EnumMRUListW,[recent_files.handle],ebx,\
		addr blob,sizeof.RecentFileBlob
	cmp	eax,sizeof.RecentFileBlob
	jne	.after_loop
	mov	eax,ebx
	inc	eax
	invoke	wsprintf,menu_label,'&%u %ls',eax,addr blob.path
	mov	r10d,ID_RECENT_BASE
	add	r10d,ebx
	invoke	AppendMenu,[hRecentMenu],MF_STRING,r10,menu_label
	inc	ebx
	jmp	.loop

  .after_loop:
	test	ebx,ebx
	jnz	.refresh
  .empty:
	invoke	AppendMenu,[hRecentMenu],MF_STRING or MF_GRAYED,ID_RECENT_END,recent_empty_label
  .refresh:
	cmp	qword [hMain],0
	je	.done
	invoke	DrawMenuBar,[hMain]
  .done:
	xor	eax,eax
	ret
endp

proc OpenRecentById uses rbx, command_id
    locals
	blob RecentFileBlob
    endl

	mov	[command_id],rcx
	cmp	qword [recent_files.handle],0
	je	.fail
	mov	ebx,ecx
	sub	ebx,ID_RECENT_BASE
	cmp	ebx,ID_RECENT_COUNT
	jae	.fail
	invoke	pMRU_EnumMRUListW,[recent_files.handle],ebx,\
		addr blob,sizeof.RecentFileBlob
	cmp	eax,sizeof.RecentFileBlob
	jne	.fail
	fastcall OpenPathIntoEditor,addr blob.path
	test	eax,eax
	jz	.fail
	invoke	SendMessage,[hEdit],EM_SETSEL,dword [blob.sel_start],dword [blob.sel_end]
	mov	eax,1
	ret
  .fail:
	xor	eax,eax
	ret
endp
