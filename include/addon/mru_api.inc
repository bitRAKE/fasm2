; addon/mru_api.inc - comctl32 MRU ordinal binding and small instance helpers.
;
; This addon centralizes the registry-backed MRU data model used by examples
; and applications. It binds comctl32 entry points by ordinal because most of
; this API is not exported by name.

define MRU_API_ORDINALS\
	151, pMRU_CreateMRUListA,\
	152, pMRU_FreeMRUList,\
	153, pMRU_AddMRUStringA,\
	154, pMRU_EnumMRUListA,\
	155, pMRU_FindMRUStringA,\
	156, pMRU_DelMRUString,\
	157, pMRU_CreateMRUListLazyA,\
	167, pMRU_AddMRUData,\
	169, pMRU_FindMRUData,\
	400, pMRU_CreateMRUListW,\
	401, pMRU_AddMRUStringW,\
	402, pMRU_FindMRUStringW,\
	403, pMRU_EnumMRUListW,\
	404, pMRU_CreateMRUListLazyW

pMRU_CreateMRUListA%	= 1
pMRU_FreeMRUList%	= 1
pMRU_AddMRUStringA%	= 2
pMRU_EnumMRUListA%	= 4
pMRU_FindMRUStringA%	= 3
pMRU_DelMRUString%	= 2
pMRU_CreateMRUListLazyA% = 4
pMRU_AddMRUData%	= 3
pMRU_FindMRUData%	= 4
pMRU_CreateMRUListW%	= 1
pMRU_AddMRUStringW%	= 2
pMRU_FindMRUStringW%	= 3
pMRU_EnumMRUListW%	= 4
pMRU_CreateMRUListLazyW% = 4

MRU_ORD_CreateMRUListA		= 151
MRU_ORD_FreeMRUList		= 152
MRU_ORD_AddMRUStringA		= 153
MRU_ORD_EnumMRUListA		= 154
MRU_ORD_FindMRUStringA		= 155
MRU_ORD_DelMRUString		= 156
MRU_ORD_CreateMRUListLazyA	= 157
MRU_ORD_AddMRUData		= 167
MRU_ORD_FindMRUData		= 169
MRU_ORD_CreateMRUListW		= 400
MRU_ORD_AddMRUStringW		= 401
MRU_ORD_FindMRUStringW		= 402
MRU_ORD_EnumMRUListW		= 403
MRU_ORD_CreateMRUListLazyW	= 404

MRU_STRING	= 0
if ~ defined MRU_BINARY
	MRU_BINARY = 1
end if
if ~ defined MRU_CACHEWRITE
	MRU_CACHEWRITE = 2
end if
MRU_MAX		= 29

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
	is_wide 	dq ?
ends

define __GLOBAL_BSS__ mru_api_bss

macro mru_api_bss
	align 8
	hComctl dq ?
	mru_find_slot dd ?
	dd ?
	match any,MRU_API_ORDINALS
		iterate <ordinal,slot>, any
			if used slot
				slot dq ?
			end if
		end iterate
	end match
purge mru_api_bss
end macro

macro MRU_BIND ordinal, slot
	if used slot
		invoke	GetProcAddress,[hComctl],ordinal
		test	rax,rax
		jz	.fail
		mov	[slot],rax
	end if
end macro

proc LoadMRUAPI
	invoke	LoadLibrary,'comctl32.dll'
	test	rax,rax
	jz	.fail
	mov	[hComctl],rax
	match any,MRU_API_ORDINALS
		iterate <ordinal,slot>, any
			MRU_BIND ordinal,slot
		end iterate
	end match
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
	cmp	qword [rbx+MRUDESC.is_wide],0
	je	.ansi
	invoke	pMRU_CreateMRUListW,rcx
	jmp	.stash
  .ansi:
	invoke	pMRU_CreateMRUListA,rcx
  .stash:
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

macro MRU_FIND_DEL_ADD inst, ptr
    local add_only
	invoke	pMRU_FindMRUData,[inst#.handle],ptr,\
		[inst#.blob_size],addr mru_find_slot
	cmp	eax,0
	jl	add_only
	invoke	pMRU_DelMRUString,[inst#.handle],eax
  add_only:
	invoke	pMRU_AddMRUData,[inst#.handle],ptr,[inst#.blob_size]
end macro

macro MRU_ENUM_W inst, rpos, dst
	invoke	pMRU_EnumMRUListW,[inst#.handle],rpos,dst,[inst#.blob_size]
end macro

macro MRU_ENUM_A inst, rpos, dst
	invoke	pMRU_EnumMRUListA,[inst#.handle],rpos,dst,[inst#.blob_size]
end macro

