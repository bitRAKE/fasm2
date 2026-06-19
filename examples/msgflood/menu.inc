; menu.inc - runtime menus for dialing message flood volume.

define FILTER_TOGGLE_LIST\
	IDM_FILTER_MOUSE,NOISE_MOUSE,\
	IDM_FILTER_RAW,NOISE_RAW,\
	IDM_FILTER_TIMER,NOISE_TIMER,\
	IDM_FILTER_PAINT,NOISE_PAINT,\
	IDM_FILTER_NONCLIENT,NOISE_NONCLIENT,\
	IDM_FILTER_POSITION,NOISE_POSITION,\
	IDM_FILTER_QUERY,NOISE_QUERY,\
	IDM_FILTER_CONTROL,NOISE_CONTROL

proc BuildMenus uses rbx, hwnd
	mov	rbx,rcx
	invoke	CreateMenu
	mov	[hMenubar],rax
	invoke	CreatePopupMenu
	mov	[hFileMenu],rax
	invoke	CreatePopupMenu
	mov	[hLogMenu],rax
	invoke	CreatePopupMenu
	mov	[hFilterMenu],rax

	invoke	AppendMenu,[hMenubar],MF_POPUP,[hFileMenu],'&File'
	invoke	AppendMenu,[hMenubar],MF_POPUP,[hLogMenu],'&Log'
	invoke	AppendMenu,[hMenubar],MF_POPUP,[hFilterMenu],'&Filters'

	invoke	AppendMenu,[hFileMenu],MF_STRING,IDM_FILE_EXIT,'E&xit'

	invoke	AppendMenu,[hLogMenu],MF_STRING,IDM_LOG_CLEAR,'&Clear'
	invoke	AppendMenu,[hLogMenu],MF_STRING,IDM_LOG_COALESCE,'&Coalesce adjacent identical messages'

	invoke	AppendMenu,[hFilterMenu],MF_STRING,IDM_FILTER_MASTER,'&Noise filter enabled'
	invoke	AppendMenu,[hFilterMenu],MF_SEPARATOR,0,0
	invoke	AppendMenu,[hFilterMenu],MF_STRING,IDM_FILTER_MOUSE,'Show &mouse move / tracking'
	invoke	AppendMenu,[hFilterMenu],MF_STRING,IDM_FILTER_RAW,'Show &raw input'
	invoke	AppendMenu,[hFilterMenu],MF_STRING,IDM_FILTER_TIMER,'Show &timer'
	invoke	AppendMenu,[hFilterMenu],MF_STRING,IDM_FILTER_PAINT,'Show &paint / erase'
	invoke	AppendMenu,[hFilterMenu],MF_STRING,IDM_FILTER_NONCLIENT,'Show &non-client hit-test'
	invoke	AppendMenu,[hFilterMenu],MF_STRING,IDM_FILTER_POSITION,'Show window &position'
	invoke	AppendMenu,[hFilterMenu],MF_STRING,IDM_FILTER_QUERY,'Show text / &icon queries'
	invoke	AppendMenu,[hFilterMenu],MF_STRING,IDM_FILTER_CONTROL,'Show &control color'
	invoke	AppendMenu,[hFilterMenu],MF_SEPARATOR,0,0
	invoke	AppendMenu,[hFilterMenu],MF_STRING,IDM_FILTER_SHOW_ALL,'Show &all noisy groups'
	invoke	AppendMenu,[hFilterMenu],MF_STRING,IDM_FILTER_HIDE_ALL,'&Hide all noisy groups'

	invoke	SetMenu,rbx,[hMenubar]
	fastcall UpdateLogMenuChecks
	fastcall UpdateFilterMenuChecks
	ret
endp

proc CheckMenuByBool item_id,is_checked
	mov	dword [item_id],ecx
	mov	eax,MF_BYCOMMAND or MF_UNCHECKED
	test	edx,edx
	je	.apply
	mov	eax,MF_BYCOMMAND or MF_CHECKED
  .apply:
	invoke	CheckMenuItem,[hFilterMenu],dword [item_id],eax
	ret
endp

proc CheckLogMenuByBool item_id,is_checked
	mov	dword [item_id],ecx
	mov	eax,MF_BYCOMMAND or MF_UNCHECKED
	test	edx,edx
	je	.apply
	mov	eax,MF_BYCOMMAND or MF_CHECKED
  .apply:
	invoke	CheckMenuItem,[hLogMenu],dword [item_id],eax
	ret
endp

proc CheckMenuByMask item_id,mask
	mov	dword [item_id],ecx
	mov	dword [mask],edx
	mov	eax,dword [noise_show_mask]
	and	eax,dword [mask]
	fastcall CheckMenuByBool,dword [item_id],eax
	ret
endp

proc UpdateLogMenuChecks
	cmp	qword [hLogMenu],0
	je	.done
	fastcall CheckLogMenuByBool,IDM_LOG_COALESCE,dword [coalesce_adjacent]
	invoke	DrawMenuBar,[hMain]
  .done:
	ret
endp

proc UpdateFilterMenuChecks
	cmp	qword [hFilterMenu],0
	je	.done
	fastcall CheckMenuByBool,IDM_FILTER_MASTER,dword [filter_noise]
	match any,FILTER_TOGGLE_LIST
		iterate <item_id,mask>, any
			fastcall CheckMenuByMask,item_id,mask
		end iterate
	end match
	invoke	DrawMenuBar,[hMain]
  .done:
	ret
endp

proc HandleMenuCommand hwnd,cmdid
	cmp	edx,IDM_FILE_EXIT
	je	.exit
	cmp	edx,IDM_LOG_CLEAR
	je	.clear
	cmp	edx,IDM_LOG_COALESCE
	je	.coalesce
	cmp	edx,IDM_FILTER_MASTER
	je	.master
	match any,FILTER_TOGGLE_LIST
		iterate <item_id,mask>, any
			cmp	edx,item_id
			je	.toggle_#item_id
		end iterate
	end match
	cmp	edx,IDM_FILTER_SHOW_ALL
	je	.show_all
	cmp	edx,IDM_FILTER_HIDE_ALL
	je	.hide_all

	xor	eax,eax
	ret

  .exit:
	invoke	DestroyWindow,rcx
	jmp	.handled
  .clear:
	fastcall ClearLog
	jmp	.handled
  .coalesce:
	fastcall ToggleAdjacentCoalesce
	fastcall UpdateLogMenuChecks
	jmp	.handled
  .master:
	fastcall ToggleNoiseFilter
	jmp	.refresh
	match any,FILTER_TOGGLE_LIST
		iterate <item_id,mask>, any
		  .toggle_#item_id:
			fastcall ToggleNoiseGroup,mask
			jmp	.refresh
		end iterate
	end match
  .show_all:
	fastcall ShowAllNoiseGroups
	jmp	.refresh
  .hide_all:
	fastcall HideAllNoiseGroups
  .refresh:
	fastcall UpdateFilterMenuChecks
  .handled:
	mov	eax,1
	ret
endp
