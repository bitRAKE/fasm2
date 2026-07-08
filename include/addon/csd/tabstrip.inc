; addon/csd/tabstrip.inc - caption tab-strip rows and small geometry helpers.

CSD_TAB_NO_INDEX		= -1
CSD_TAB_PART_NONE		= 0
CSD_TAB_PART_GAP		= 1
CSD_TAB_PART_BODY		= 2
CSD_TAB_PART_CLOSE		= 3

CSD_TAB_FLAG_VISIBLE		= 0001h

CSD_TAB_VALUE_ACTIVE		= 5

struct CSD_TAB_ITEM
  id		dd ?
  flags		dd ?
  titlep	dq ?
  iconGlyph	dd ?
  minWidthDip	dd ?
  preferredDip	dd ?
  maxWidthDip	dd ?
ends

struct CSD_TAB_GEOMETRY
  tabRect	RECT
  closeRect	RECT
  titleRect	RECT
ends

struct CSD_TAB_STATE
  value		db ?
  closeValue	db ?
  fadePhase	db ?
  reserved	db ?
ends

struct CSD_TABSTRIP
  itemp		dq ?
  geomp		dq ?
  statep	dq ?
  count		dd ?
  capacity	dd ?
  activeIndex	dd ?
  hotIndex	dd ?
  hotPart	dd ?
  pressedIndex	dd ?
  pressedPart	dd ?
  reserved	dd ?
ends

proc CsdRectContains rectp,x,y
	cmp	edx,[rcx+RECT.left]
	jl	rect_contains_no
	cmp	edx,[rcx+RECT.right]
	jge	rect_contains_no
	cmp	r8d,[rcx+RECT.top]
	jl	rect_contains_no
	cmp	r8d,[rcx+RECT.bottom]
	jge	rect_contains_no
	mov	eax,1
	ret

  rect_contains_no:
	xor	eax,eax
	ret
endp

proc CsdRectClear rectp
	mov	dword [rcx+RECT.left],0
	mov	dword [rcx+RECT.top],0
	mov	dword [rcx+RECT.right],0
	mov	dword [rcx+RECT.bottom],0
	ret
endp

proc CsdTabGeometryClear geomp
	fastcall CsdRectClear,rcx
	add	rcx,sizeof.RECT
	fastcall CsdRectClear,rcx
	add	rcx,sizeof.RECT
	fastcall CsdRectClear,rcx
	ret
endp

proc CsdTabStripBind stripp,itemp,geomp,statep,capacity
	mov	[rcx+CSD_TABSTRIP.itemp],rdx
	mov	[rcx+CSD_TABSTRIP.geomp],r8
	mov	[rcx+CSD_TABSTRIP.statep],r9
	mov	eax,dword [capacity]
	mov	[rcx+CSD_TABSTRIP.capacity],eax
	mov	dword [rcx+CSD_TABSTRIP.count],0
	mov	dword [rcx+CSD_TABSTRIP.activeIndex],CSD_TAB_NO_INDEX
	mov	dword [rcx+CSD_TABSTRIP.hotIndex],CSD_TAB_NO_INDEX
	mov	dword [rcx+CSD_TABSTRIP.hotPart],CSD_TAB_PART_NONE
	mov	dword [rcx+CSD_TABSTRIP.pressedIndex],CSD_TAB_NO_INDEX
	mov	dword [rcx+CSD_TABSTRIP.pressedPart],CSD_TAB_PART_NONE
	ret
endp

proc CsdTabStripClearInput stripp
	mov	dword [rcx+CSD_TABSTRIP.hotIndex],CSD_TAB_NO_INDEX
	mov	dword [rcx+CSD_TABSTRIP.hotPart],CSD_TAB_PART_NONE
	mov	dword [rcx+CSD_TABSTRIP.pressedIndex],CSD_TAB_NO_INDEX
	mov	dword [rcx+CSD_TABSTRIP.pressedPart],CSD_TAB_PART_NONE
	ret
endp
