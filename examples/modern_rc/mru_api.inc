; mru_api.inc - ordinal binding for comctl32's MRU API.
;
; The public include files expose only part of this API. This module uses
; LoadLibraryW/GetProcAddress with MAKEINTRESOURCE ordinals and stores the
; resulting function pointers in local slots prefixed with pMRU_.

pMRU_CreateMRUListW% = 1
pMRU_FreeMRUList%	= 1
pMRU_AddMRUData%	= 3
pMRU_FindMRUData%	= 4
pMRU_EnumMRUListW%	= 4
pMRU_DelMRUString%	= 2

MRU_ORD_CreateMRUListW	= 400
MRU_ORD_FreeMRUList	= 152
MRU_ORD_AddMRUData	= 167
MRU_ORD_FindMRUData	= 169
MRU_ORD_EnumMRUListW	= 403
MRU_ORD_DelMRUString	= 156

MRU_STRING		= 0
MRU_BINARY		= 1
MRU_CACHEWRITE		= 2

struct MRUINFO
	cbSize		dd ?
	uMax		dd ?
	fFlags		dd ?
			dd ?
	hKey		dq ?
	lpszSubKey	dq ?
	lpfnCompare	dq ?
ends

struct MRUDESC
	info_ptr	dq ?
	handle		dq ?
	blob_size	dq ?
ends

define __GLOBAL_BSS__ mru_api_bss

macro mru_api_bss
	align 8
	hComctl		dq ?
	pMRU_CreateMRUListW dq ?
	pMRU_FreeMRUList    dq ?
	pMRU_AddMRUData     dq ?
	pMRU_FindMRUData    dq ?
	pMRU_EnumMRUListW   dq ?
	pMRU_DelMRUString   dq ?
purge mru_api_bss
end macro

macro MRU_BIND ordinal, slot
	invoke	GetProcAddress,[hComctl],ordinal
	test	rax,rax
	jz	.fail
	mov	[slot],rax
end macro

proc LoadMRUAPI
	invoke	LoadLibrary,'comctl32.dll'
	test	rax,rax
	jz	.fail
	mov	[hComctl],rax
	MRU_BIND MRU_ORD_CreateMRUListW,pMRU_CreateMRUListW
	MRU_BIND MRU_ORD_FreeMRUList,pMRU_FreeMRUList
	MRU_BIND MRU_ORD_AddMRUData,pMRU_AddMRUData
	MRU_BIND MRU_ORD_FindMRUData,pMRU_FindMRUData
	MRU_BIND MRU_ORD_EnumMRUListW,pMRU_EnumMRUListW
	MRU_BIND MRU_ORD_DelMRUString,pMRU_DelMRUString
	mov	eax,1
	ret
  .fail:
	xor	eax,eax
	ret
endp

proc MRUOpen uses rbx, desc
	mov	[desc],rcx
	mov	rbx,rcx
	mov	rcx,[rbx+MRUDESC.info_ptr]
	invoke	pMRU_CreateMRUListW,rcx
	mov	rbx,[desc]
	mov	[rbx+MRUDESC.handle],rax
	test	rax,rax
	setnz	al
	movzx	eax,al
	ret
endp

proc MRUClose uses rbx, desc
	mov	[desc],rcx
	mov	rbx,rcx
	mov	rcx,[rbx+MRUDESC.handle]
	test	rcx,rcx
	jz	.done
	invoke	pMRU_FreeMRUList,rcx
	mov	rbx,[desc]
	mov	qword [rbx+MRUDESC.handle],0
  .done:
	xor	eax,eax
	ret
endp
