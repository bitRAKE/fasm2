; addon/csd/uia.inc - shared UI Automation support for CSD examples.
;
; This module owns policy-free UIA mechanics: constants, COM IDs, provider
; storage layout, API binding, reference counting, VARIANT helpers, and runtime
; ID array creation. Consumers still own the provider tree, element names,
; bounds policy, focus policy, and command invocation.

WM_GETOBJECT		= 003Dh
UiaRootObjectId		= -25
UiaAppendRuntimeId	= 3

S_OK			= 00000000h
E_NOTIMPL		= 080004001h
E_NOINTERFACE		= 080004002h
E_POINTER		= 080004003h
E_FAIL			= 080004005h
E_OUTOFMEMORY		= 08007000Eh

ProviderOptions_ServerSideProvider = 00000002h
NavigateDirection_Parent	= 0
NavigateDirection_NextSibling	= 1
NavigateDirection_PreviousSibling = 2
NavigateDirection_FirstChild	= 3
NavigateDirection_LastChild	= 4

UIA_InvokePatternId			= 10000
UIA_SelectionItemPatternId		= 10010
UIA_BoundingRectanglePropertyId		= 30001
UIA_ControlTypePropertyId		= 30003
UIA_NamePropertyId			= 30005
UIA_HasKeyboardFocusPropertyId		= 30008
UIA_IsKeyboardFocusablePropertyId	= 30009
UIA_IsEnabledPropertyId			= 30010
UIA_AutomationIdPropertyId		= 30011
UIA_ItemStatusPropertyId		= 30026
UIA_SelectionItemIsSelectedPropertyId	= 30079
UIA_ButtonControlTypeId			= 50000
UIA_TabControlTypeId			= 50018
UIA_TabItemControlTypeId		= 50019

VT_EMPTY		= 0
VT_I4			= 3
VT_BSTR			= 8
VT_BOOL			= 11
VARIANT_TRUE		= -1

struct CSD_VARIANT
  vt		dw ?
  reserved1	dw ?
  reserved2	dw ?
  reserved3	dw ?
  value		dq ?
  extra		dq ?
ends

struct UIA_RECT
  left		dq ?
  top		dq ?
  width		dq ?
  height	dq ?
ends

struct CSD_UIA_PROVIDER
  simpleVtbl	dq ?
  simpleOwner	dq ?
  fragmentVtbl	dq ?
  fragmentOwner dq ?
  rootVtbl	dq ?
  rootOwner	dq ?
  invokeVtbl	dq ?
  invokeOwner	dq ?
  selectionVtbl	dq ?
  selectionOwner dq ?
  refCount	dd ?
  slot		dd ?
  reserved	dd ?
  reserved2	dd ?
ends

CSD_UIA_INTERFACE_OWNER	= 8
CSD_UIA_SIMPLE_OFFSET	= CSD_UIA_PROVIDER.simpleVtbl
CSD_UIA_FRAGMENT_OFFSET = CSD_UIA_PROVIDER.fragmentVtbl
CSD_UIA_ROOT_OFFSET	= CSD_UIA_PROVIDER.rootVtbl
CSD_UIA_INVOKE_OFFSET	= CSD_UIA_PROVIDER.invokeVtbl
CSD_UIA_SELECTION_OFFSET = CSD_UIA_PROVIDER.selectionVtbl

define __GLOBAL_DATA__ csd_uia_data
define __GLOBAL_BSS__ csd_uia_bss

macro csd_uia_data
	align 8
	CSD_IID_IUnknown		dd 000000000h
					dw 00000h,00000h
					db 0C0h,000h,000h,000h,000h,000h,000h,046h
	CSD_IID_IRawElementProviderSimple dd 0D6DD68D1h
					dw 086FDh,04332h
					db 086h,066h,09Ah,0BEh,0DEh,0A2h,0D2h,04Ch
	CSD_IID_IRawElementProviderFragmentRoot dd 0620CE2A5h
					dw 0AB8Fh,040A9h
					db 086h,0CBh,0DEh,03Ch,075h,059h,09Bh,058h
	CSD_IID_IRawElementProviderFragment dd 0F7063DA8h
					dw 08359h,0439Ch
					db 092h,097h,0BBh,0C5h,029h,09Ah,07Dh,087h
	CSD_IID_IInvokeProvider		dd 054FCB24Bh
					dw 0E18Eh,047A2h
					db 0B4h,0D3h,0ECh,0CBh,0E7h,075h,099h,0A2h
	CSD_IID_ISelectionItemProvider	dd 02ACAD808h
					dw 0B2D4h,0452Dh
					db 0A4h,007h,091h,0FFh,01Ah,0D1h,067h,0B2h
purge csd_uia_data
end macro

macro csd_uia_bss
	align 8
	hCsdUiaCore	dq ?
	hCsdOleAut	dq ?
	pCsdUiaReturnRawElementProvider dq ?
	pCsdUiaHostProviderFromHwnd dq ?
	pCsdUiaDisconnectProvider dq ?
	pCsdSysAllocString dq ?
	pCsdSafeArrayCreateVector dq ?
	pCsdSafeArrayPutElement dq ?
	pCsdSafeArrayDestroy dq ?
	csd_uia_ready	dd ?
	dd ?
purge csd_uia_bss
end macro

proc CsdUiaLoadApis
	cmp	dword [csd_uia_ready],0
	jne	csd_uia_load_done

	invoke	LoadLibraryW,'UIAutomationCore.dll'
	mov	[hCsdUiaCore],rax
	test	rax,rax
	jz	csd_uia_load_done
	invoke	GetProcAddress,[hCsdUiaCore],<A,'UiaReturnRawElementProvider'>
	mov	[pCsdUiaReturnRawElementProvider],rax
	invoke	GetProcAddress,[hCsdUiaCore],<A,'UiaHostProviderFromHwnd'>
	mov	[pCsdUiaHostProviderFromHwnd],rax
	invoke	GetProcAddress,[hCsdUiaCore],<A,'UiaDisconnectProvider'>
	mov	[pCsdUiaDisconnectProvider],rax

	invoke	LoadLibraryW,'oleaut32.dll'
	mov	[hCsdOleAut],rax
	test	rax,rax
	jz	csd_uia_load_fail
	invoke	GetProcAddress,[hCsdOleAut],<A,'SysAllocString'>
	mov	[pCsdSysAllocString],rax
	invoke	GetProcAddress,[hCsdOleAut],<A,'SafeArrayCreateVector'>
	mov	[pCsdSafeArrayCreateVector],rax
	invoke	GetProcAddress,[hCsdOleAut],<A,'SafeArrayPutElement'>
	mov	[pCsdSafeArrayPutElement],rax
	invoke	GetProcAddress,[hCsdOleAut],<A,'SafeArrayDestroy'>
	mov	[pCsdSafeArrayDestroy],rax

	iterate pointer, pCsdUiaReturnRawElementProvider,pCsdUiaHostProviderFromHwnd,\
		pCsdUiaDisconnectProvider,pCsdSysAllocString,\
		pCsdSafeArrayCreateVector,pCsdSafeArrayPutElement,\
		pCsdSafeArrayDestroy
		cmp	qword [pointer],0
		je	csd_uia_load_fail
	end iterate
	mov	dword [csd_uia_ready],1

  csd_uia_load_done:
	mov	eax,[csd_uia_ready]
	ret

  csd_uia_load_fail:
	mov	dword [csd_uia_ready],0
	xor	eax,eax
	ret
endp

proc CsdUiaInitProvider providerp,slot,simpleVtbl,fragmentVtbl,rootVtbl,invokeVtbl,selectionVtbl
	mov	[rcx+CSD_UIA_PROVIDER.simpleVtbl],r8
	mov	[rcx+CSD_UIA_PROVIDER.simpleOwner],rcx
	mov	[rcx+CSD_UIA_PROVIDER.fragmentVtbl],r9
	mov	[rcx+CSD_UIA_PROVIDER.fragmentOwner],rcx
	mov	rax,[rootVtbl]
	mov	[rcx+CSD_UIA_PROVIDER.rootVtbl],rax
	mov	[rcx+CSD_UIA_PROVIDER.rootOwner],rcx
	mov	rax,[invokeVtbl]
	mov	[rcx+CSD_UIA_PROVIDER.invokeVtbl],rax
	mov	[rcx+CSD_UIA_PROVIDER.invokeOwner],rcx
	mov	rax,[selectionVtbl]
	mov	[rcx+CSD_UIA_PROVIDER.selectionVtbl],rax
	mov	[rcx+CSD_UIA_PROVIDER.selectionOwner],rcx
	mov	dword [rcx+CSD_UIA_PROVIDER.refCount],1
	mov	[rcx+CSD_UIA_PROVIDER.slot],edx
	mov	dword [rcx+CSD_UIA_PROVIDER.reserved],0
	ret
endp

proc CsdUiaDisconnectProvider simplep
	cmp	qword [pCsdUiaDisconnectProvider],0
	je	csd_uia_disconnect_done
	fastcall [pCsdUiaDisconnectProvider],rcx

  csd_uia_disconnect_done:
	ret
endp

proc CsdUiaGuidEquals guidp,knownp
	mov	rax,[rcx]
	cmp	rax,[rdx]
	jne	csd_uia_guid_not_equal
	mov	rax,[rcx+8]
	cmp	rax,[rdx+8]
	jne	csd_uia_guid_not_equal
	mov	eax,1
	ret

  csd_uia_guid_not_equal:
	xor	eax,eax
	ret
endp

proc CsdUiaReturnFragmentInterface providerp,outp
	test	rdx,rdx
	jz	csd_uia_return_fragment_no
	mov	qword [rdx],0
	test	rcx,rcx
	jz	csd_uia_return_fragment_no
	lea	rax,[rcx+CSD_UIA_FRAGMENT_OFFSET]
	mov	[rdx],rax
	inc	dword [rcx+CSD_UIA_PROVIDER.refCount]
	mov	eax,1
	ret

  csd_uia_return_fragment_no:
	xor	eax,eax
	ret
endp

proc CsdUiaReturnSimpleInterface providerp,outp
	test	rdx,rdx
	jz	csd_uia_return_simple_no
	mov	qword [rdx],0
	test	rcx,rcx
	jz	csd_uia_return_simple_no
	lea	rax,[rcx+CSD_UIA_SIMPLE_OFFSET]
	mov	[rdx],rax
	inc	dword [rcx+CSD_UIA_PROVIDER.refCount]
	mov	eax,1
	ret

  csd_uia_return_simple_no:
	xor	eax,eax
	ret
endp

proc CsdUiaVariantEmpty variantp
	mov	qword [rcx],0
	mov	qword [rcx+8],0
	mov	qword [rcx+16],0
	ret
endp

proc CsdUiaVariantI4 variantp,value
	fastcall CsdUiaVariantEmpty,rcx
	mov	word [rcx+CSD_VARIANT.vt],VT_I4
	mov	dword [rcx+CSD_VARIANT.value],edx
	xor	eax,eax
	ret
endp

proc CsdUiaVariantBool variantp,value
	fastcall CsdUiaVariantEmpty,rcx
	mov	word [rcx+CSD_VARIANT.vt],VT_BOOL
	xor	eax,eax
	test	edx,edx
	jz	csd_uia_variant_bool_store
	mov	ax,VARIANT_TRUE

  csd_uia_variant_bool_store:
	mov	word [rcx+CSD_VARIANT.value],ax
	xor	eax,eax
	ret
endp

proc CsdUiaVariantBstr variantp,stringp
	mov	[variantp],rcx
	fastcall CsdUiaVariantEmpty,rcx
	mov	word [rcx+CSD_VARIANT.vt],VT_BSTR
	fastcall [pCsdSysAllocString],rdx
	test	rax,rax
	jz	csd_uia_variant_bstr_oom
	mov	rcx,[variantp]
	mov	[rcx+CSD_VARIANT.value],rax
	xor	eax,eax
	ret

  csd_uia_variant_bstr_oom:
	mov	eax,E_OUTOFMEMORY
	ret
endp

proc CsdUiaProvider_AddRef thisp
	mov	rax,[rcx+CSD_UIA_INTERFACE_OWNER]
	inc	dword [rax+CSD_UIA_PROVIDER.refCount]
	mov	eax,[rax+CSD_UIA_PROVIDER.refCount]
	ret
endp

proc CsdUiaProvider_Release thisp
	mov	rcx,[rcx+CSD_UIA_INTERFACE_OWNER]
	cmp	dword [rcx+CSD_UIA_PROVIDER.refCount],1
	jbe	csd_uia_provider_release_done
	dec	dword [rcx+CSD_UIA_PROVIDER.refCount]

  csd_uia_provider_release_done:
	mov	eax,[rcx+CSD_UIA_PROVIDER.refCount]
	ret
endp

proc CsdUiaSimple_ProviderOptions thisp,pRetVal
	test	rdx,rdx
	jz	csd_uia_simple_options_bad_pointer
	mov	dword [rdx],ProviderOptions_ServerSideProvider
	xor	eax,eax
	ret

  csd_uia_simple_options_bad_pointer:
	mov	eax,E_POINTER
	ret
endp

proc CsdUiaRuntimeIdForSlot hwnd,slot,outp
    locals
	arrayp	dq ?
	outp_local dq ?
	index	dd ?
	value	dd ?
    endl

	mov	[hwnd],rcx
	mov	dword [slot],edx
	mov	[outp_local],r8
	test	r8,r8
	jz	csd_uia_runtime_bad_pointer
	mov	qword [r8],0
	fastcall [pCsdSafeArrayCreateVector],VT_I4,0,3
	test	rax,rax
	jz	csd_uia_runtime_oom
	mov	[arrayp],rax

	mov	dword [index],0
	mov	dword [value],UiaAppendRuntimeId
	fastcall [pCsdSafeArrayPutElement],[arrayp],addr index,addr value
	test	eax,eax
	jnz	csd_uia_runtime_fail
	mov	dword [index],1
	mov	eax,dword [hwnd]
	mov	[value],eax
	fastcall [pCsdSafeArrayPutElement],[arrayp],addr index,addr value
	test	eax,eax
	jnz	csd_uia_runtime_fail
	mov	dword [index],2
	mov	eax,dword [slot]
	inc	eax
	mov	[value],eax
	fastcall [pCsdSafeArrayPutElement],[arrayp],addr index,addr value
	test	eax,eax
	jnz	csd_uia_runtime_fail

	mov	rax,[arrayp]
	mov	rdx,[outp_local]
	mov	[rdx],rax
	xor	eax,eax
	ret

  csd_uia_runtime_fail:
	fastcall [pCsdSafeArrayDestroy],[arrayp]
	mov	eax,E_FAIL
	ret

  csd_uia_runtime_oom:
	mov	eax,E_OUTOFMEMORY
	ret

  csd_uia_runtime_bad_pointer:
	mov	eax,E_POINTER
	ret
endp

proc CsdUiaFragment_GetRuntimeId thisp,hwnd,outp
    locals
	hwnd_local dq ?
	outp_local dq ?
	slot_local dd ?
	dd ?
    endl

	mov	[hwnd_local],rdx
	mov	[outp_local],r8
	mov	rax,[rcx+CSD_UIA_INTERFACE_OWNER]
	mov	edx,[rax+CSD_UIA_PROVIDER.slot]
	mov	[slot_local],edx
	fastcall CsdUiaRuntimeIdForSlot,[hwnd_local],dword [slot_local],[outp_local]
	ret
endp

proc CsdUiaSelection_SelectionContainer rootProviderp,outp
	test	rdx,rdx
	jz	csd_uia_selection_container_bad_pointer
	fastcall CsdUiaReturnSimpleInterface,rcx,rdx
	xor	eax,eax
	ret

  csd_uia_selection_container_bad_pointer:
	mov	eax,E_POINTER
	ret
endp

proc CsdUiaHandleGetObject hwnd,wparam,lparam,rootSimplep
    locals
	hwnd_local dq ?
	wparam_local dq ?
	lparam_local dq ?
	root_local dq ?
    endl

	mov	[hwnd_local],rcx
	mov	[wparam_local],rdx
	mov	[lparam_local],r8
	mov	[root_local],r9
	cmp	dword [lparam_local],UiaRootObjectId
	jne	csd_uia_getobject_no
	cmp	qword [root_local],0
	je	csd_uia_getobject_no
	cmp	qword [pCsdUiaReturnRawElementProvider],0
	je	csd_uia_getobject_no
	fastcall [pCsdUiaReturnRawElementProvider],[hwnd_local],[wparam_local],\
		[lparam_local],[root_local]
	ret

  csd_uia_getobject_no:
	xor	eax,eax
	ret
endp
