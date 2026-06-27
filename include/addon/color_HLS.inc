; addon/color_HLS.inc - Win2K ControlPaint HLS color helpers.
;
; Host requirements:
;   include after a Win64 Windows include with proc/endp, fastcall, and structs
;
; COLORREF values use the Win32/GDI byte order: 00BBGGRR.
; Floating percentage arguments follow the Microsoft x64 ABI slot position:
;   ColorHLS_Darker(hlscolorp, perc) receives perc in xmm1.
;
; Public surface:
;   COLOR_HLS                         ; hue, saturation, luminosity dwords
;   ColorHLS_FromColorRef(hlscolorp, colorref) -> hlscolorp
;   ColorHLS_ToColorRef(hlscolorp) -> COLORREF
;   ColorHLS_ColorFromHLS(hue, luminosity, saturation) -> COLORREF
;   ColorHLS_NewLuma(luminosity, adjustment, scale) -> luminosity
;   ColorHLS_Darker(hlscolorp, float perc) -> COLORREF
;   ColorHLS_Lighter(hlscolorp, float perc) -> COLORREF
;   ColorHLS_DarkenColorRef(colorref, float perc) -> COLORREF
;   ColorHLS_LightenColorRef(colorref, float perc) -> COLORREF
;   ColorHLS_DarkerQ16(hlscolorp, percent_q16) -> COLORREF
;   ColorHLS_LighterQ16(hlscolorp, percent_q16) -> COLORREF
;   ColorHLS_DarkenColorRefQ16(colorref, percent_q16) -> COLORREF
;   ColorHLS_LightenColorRefQ16(colorref, percent_q16) -> COLORREF

if ~ definite COLOR_HLS_INC
COLOR_HLS_INC = 1

COLOR_HLS_SHADOW_ADJ	= -333
COLOR_HLS_HILIGHT_ADJ	= 500
COLOR_HLS_RANGE		= 240
COLOR_HLS_MAX		= COLOR_HLS_RANGE
COLOR_HLS_RGB_MAX	= 255
COLOR_HLS_UNDEFINED	= COLOR_HLS_MAX * 2 / 3
COLOR_HLS_Q16_ONE	= 10000h

struct COLOR_HLS
	hue		dd ?
	saturation	dd ?
	luminosity	dd ?
ends

proc ColorHLS_NewLuma luminosity,adjustment,scale
	mov	eax,ecx
	test	edx,edx
	jnz	newluma_nonzero
	ret

  newluma_nonzero:
	test	r8d,r8d
	jz	newluma_unscaled
	test	edx,edx
	jle	newluma_scale_down

  newluma_scale_up:
	mov	r10d,1000
	sub	r10d,edx
	mov	eax,ecx
	imul	eax,r10d
	movsxd	rax,eax
	movsxd	r10,edx
	imul	r10,r10,COLOR_HLS_RANGE + 1
	add	rax,r10
	cqo
	mov	r10d,1000
	idiv	r10
	ret

  newluma_scale_down:
	mov	r10d,edx
	add	r10d,1000
	mov	eax,ecx
	imul	eax,r10d
	cdq
	mov	r10d,1000
	idiv	r10d
	ret

  newluma_unscaled:
	movsxd	rax,edx
	imul	rax,rax,COLOR_HLS_RANGE
	cqo
	mov	r10d,1000
	idiv	r10
	add	eax,ecx
	jns	newluma_check_high
	xor	eax,eax
	ret

  newluma_check_high:
	cmp	eax,COLOR_HLS_MAX
	jle	newluma_done
	mov	eax,COLOR_HLS_MAX
  newluma_done:
	ret
endp

proc ColorHLS_HueToRGB n1,n2,hue
	mov	eax,r8d
	test	eax,eax
	jns	huetorgb_check_high
	add	eax,COLOR_HLS_MAX

  huetorgb_check_high:
	cmp	eax,COLOR_HLS_MAX
	jle	huetorgb_normalized
	sub	eax,COLOR_HLS_MAX

  huetorgb_normalized:
	cmp	eax,COLOR_HLS_MAX / 6
	jl	huetorgb_first
	cmp	eax,COLOR_HLS_MAX / 2
	jl	huetorgb_second
	cmp	eax,(COLOR_HLS_MAX * 2) / 3
	jl	huetorgb_third
	mov	eax,ecx
	ret

  huetorgb_second:
	mov	eax,edx
	ret

  huetorgb_first:
	mov	r9d,edx
	sub	r9d,ecx
	imul	r9d,eax
	add	r9d,COLOR_HLS_MAX / 12
	mov	eax,r9d
	cdq
	mov	r10d,COLOR_HLS_MAX / 6
	idiv	r10d
	add	eax,ecx
	ret

  huetorgb_third:
	mov	r9d,(COLOR_HLS_MAX * 2) / 3
	sub	r9d,eax
	mov	eax,edx
	sub	eax,ecx
	imul	eax,r9d
	add	eax,COLOR_HLS_MAX / 12
	cdq
	mov	r10d,COLOR_HLS_MAX / 6
	idiv	r10d
	add	eax,ecx
	ret
endp

proc ColorHLS_FromColorRef hlscolorp,colorref
    locals
	red		dd ?
	green		dd ?
	blue		dd ?
	max_rgb		dd ?
	min_rgb		dd ?
	sum_rgb		dd ?
	dif_rgb		dd ?
	luminosity	dd ?
	red_delta	dd ?
	green_delta	dd ?
	blue_delta	dd ?
    endl
	mov	[hlscolorp],rcx
	mov	dword [colorref],edx

	mov	eax,dword [colorref]
	and	eax,0FFh
	mov	[red],eax
	mov	eax,dword [colorref]
	shr	eax,8
	and	eax,0FFh
	mov	[green],eax
	mov	eax,dword [colorref]
	shr	eax,16
	and	eax,0FFh
	mov	[blue],eax

	mov	eax,[red]
	cmp	eax,[green]
	cmovl	eax,[green]
	cmp	eax,[blue]
	cmovl	eax,[blue]
	mov	[max_rgb],eax

	mov	eax,[red]
	cmp	eax,[green]
	cmovg	eax,[green]
	cmp	eax,[blue]
	cmovg	eax,[blue]
	mov	[min_rgb],eax

	mov	eax,[max_rgb]
	add	eax,[min_rgb]
	mov	[sum_rgb],eax
	imul	eax,eax,COLOR_HLS_MAX
	add	eax,COLOR_HLS_RGB_MAX
	cdq
	mov	ecx,2 * COLOR_HLS_RGB_MAX
	idiv	ecx
	mov	[luminosity],eax
	mov	rcx,[hlscolorp]
	mov	[rcx+COLOR_HLS.luminosity],eax

	mov	eax,[max_rgb]
	sub	eax,[min_rgb]
	mov	[dif_rgb],eax
	test	eax,eax
	jnz	from_chromatic

	mov	rcx,[hlscolorp]
	mov	[rcx+COLOR_HLS.saturation],0
	mov	[rcx+COLOR_HLS.hue],COLOR_HLS_UNDEFINED
	mov	rax,rcx
	ret

  from_chromatic:
	cmp	[luminosity],COLOR_HLS_MAX / 2
	jg	from_saturation_high
	mov	eax,[dif_rgb]
	imul	eax,eax,COLOR_HLS_MAX
	mov	ecx,[sum_rgb]
	sar	ecx,1
	add	eax,ecx
	cdq
	idiv	dword [sum_rgb]
	jmp	from_saturation_ready

  from_saturation_high:
	mov	ecx,2 * COLOR_HLS_RGB_MAX
	sub	ecx,[sum_rgb]
	mov	edx,ecx
	sar	edx,1
	mov	eax,[dif_rgb]
	imul	eax,eax,COLOR_HLS_MAX
	add	eax,edx
	cdq
	idiv	ecx

  from_saturation_ready:
	mov	rcx,[hlscolorp]
	mov	[rcx+COLOR_HLS.saturation],eax

	mov	eax,[max_rgb]
	sub	eax,[red]
	imul	eax,eax,COLOR_HLS_MAX / 6
	mov	ecx,[dif_rgb]
	sar	ecx,1
	add	eax,ecx
	cdq
	idiv	dword [dif_rgb]
	mov	[red_delta],eax

	mov	eax,[max_rgb]
	sub	eax,[green]
	imul	eax,eax,COLOR_HLS_MAX / 6
	mov	ecx,[dif_rgb]
	sar	ecx,1
	add	eax,ecx
	cdq
	idiv	dword [dif_rgb]
	mov	[green_delta],eax

	mov	eax,[max_rgb]
	sub	eax,[blue]
	imul	eax,eax,COLOR_HLS_MAX / 6
	mov	ecx,[dif_rgb]
	sar	ecx,1
	add	eax,ecx
	cdq
	idiv	dword [dif_rgb]
	mov	[blue_delta],eax

	mov	eax,[red]
	cmp	eax,[max_rgb]
	jne	from_green_max
	mov	eax,[blue_delta]
	sub	eax,[green_delta]
	jmp	from_hue_ready

  from_green_max:
	mov	eax,[green]
	cmp	eax,[max_rgb]
	jne	from_blue_max
	mov	eax,COLOR_HLS_MAX / 3
	add	eax,[red_delta]
	sub	eax,[blue_delta]
	jmp	from_hue_ready

  from_blue_max:
	mov	eax,(COLOR_HLS_MAX * 2) / 3
	add	eax,[green_delta]
	sub	eax,[red_delta]

  from_hue_ready:
	test	eax,eax
	jns	from_hue_nonnegative
	add	eax,COLOR_HLS_MAX
  from_hue_nonnegative:
	cmp	eax,COLOR_HLS_MAX
	jle	from_hue_store
	sub	eax,COLOR_HLS_MAX
  from_hue_store:
	mov	rcx,[hlscolorp]
	mov	[rcx+COLOR_HLS.hue],eax
	mov	rax,rcx
	ret
endp

proc ColorHLS_ColorFromHLS hue,luminosity,saturation
    locals
	magic1	dd ?
	magic2	dd ?
	red	dd ?
	green	dd ?
    endl
	mov	dword [hue],ecx
	mov	dword [luminosity],edx
	mov	dword [saturation],r8d

	cmp	dword [saturation],0
	jnz	colorfrom_chromatic
	mov	eax,dword [luminosity]
	imul	eax,eax,COLOR_HLS_RGB_MAX
	cdq
	mov	ecx,COLOR_HLS_MAX
	idiv	ecx
	and	eax,0FFh
	imul	eax,eax,010101h
	ret

  colorfrom_chromatic:
	cmp	dword [luminosity],COLOR_HLS_MAX / 2
	jg	colorfrom_high_luminosity
	mov	eax,dword [luminosity]
	mov	ecx,COLOR_HLS_MAX
	add	ecx,dword [saturation]
	imul	eax,ecx
	add	eax,COLOR_HLS_MAX / 2
	cdq
	mov	ecx,COLOR_HLS_MAX
	idiv	ecx
	mov	[magic2],eax
	jmp	colorfrom_magic_ready

  colorfrom_high_luminosity:
	mov	eax,dword [luminosity]
	imul	eax,dword [saturation]
	add	eax,COLOR_HLS_MAX / 2
	cdq
	mov	ecx,COLOR_HLS_MAX
	idiv	ecx
	mov	ecx,dword [luminosity]
	add	ecx,dword [saturation]
	sub	ecx,eax
	mov	[magic2],ecx

  colorfrom_magic_ready:
	mov	eax,dword [luminosity]
	shl	eax,1
	sub	eax,[magic2]
	mov	[magic1],eax

	mov	r8d,dword [hue]
	add	r8d,COLOR_HLS_MAX / 3
	fastcall ColorHLS_HueToRGB,dword [magic1],dword [magic2],r8d
	imul	eax,eax,COLOR_HLS_RGB_MAX
	add	eax,COLOR_HLS_MAX / 2
	cdq
	mov	ecx,COLOR_HLS_MAX
	idiv	ecx
	and	eax,0FFh
	mov	[red],eax

	fastcall ColorHLS_HueToRGB,dword [magic1],dword [magic2],dword [hue]
	imul	eax,eax,COLOR_HLS_RGB_MAX
	add	eax,COLOR_HLS_MAX / 2
	cdq
	mov	ecx,COLOR_HLS_MAX
	idiv	ecx
	and	eax,0FFh
	mov	[green],eax

	mov	r8d,dword [hue]
	sub	r8d,COLOR_HLS_MAX / 3
	fastcall ColorHLS_HueToRGB,dword [magic1],dword [magic2],r8d
	imul	eax,eax,COLOR_HLS_RGB_MAX
	add	eax,COLOR_HLS_MAX / 2
	cdq
	mov	ecx,COLOR_HLS_MAX
	idiv	ecx
	and	eax,0FFh
	shl	eax,16
	mov	ecx,[green]
	shl	ecx,8
	or	eax,ecx
	or	eax,[red]
	ret
endp

proc ColorHLS_ToColorRef hlscolorp
    locals
	hue		dd ?
	luminosity	dd ?
	saturation	dd ?
    endl
	mov	[hlscolorp],rcx
	mov	rax,[hlscolorp]
	mov	ecx,[rax+COLOR_HLS.hue]
	mov	[hue],ecx
	mov	ecx,[rax+COLOR_HLS.luminosity]
	mov	[luminosity],ecx
	mov	ecx,[rax+COLOR_HLS.saturation]
	mov	[saturation],ecx
	fastcall ColorHLS_ColorFromHLS,dword [hue],dword [luminosity],dword [saturation]
	ret
endp

proc ColorHLS_Darker hlscolorp,percDarker
    locals
	percent		dd ?
	hue		dd ?
	luminosity	dd ?
	saturation	dd ?
	zero_lum	dd ?
	new_lum		dd ?
    endl
	mov	[hlscolorp],rcx
	movss	dword [percent],xmm1

	mov	rax,[hlscolorp]
	mov	ecx,[rax+COLOR_HLS.hue]
	mov	[hue],ecx
	mov	ecx,[rax+COLOR_HLS.luminosity]
	mov	[luminosity],ecx
	mov	ecx,[rax+COLOR_HLS.saturation]
	mov	[saturation],ecx

	fastcall ColorHLS_NewLuma,dword [luminosity],COLOR_HLS_SHADOW_ADJ,1
	mov	[zero_lum],eax
	cvtsi2ss xmm0,dword [zero_lum]
	mulss	xmm0,dword [percent]
	cvttss2si ecx,xmm0
	mov	edx,[zero_lum]
	sub	edx,ecx
	mov	[new_lum],edx
	fastcall ColorHLS_ColorFromHLS,dword [hue],dword [new_lum],dword [saturation]
	ret
endp

proc ColorHLS_Lighter hlscolorp,percLighter
    locals
	percent		dd ?
	hue		dd ?
	luminosity	dd ?
	saturation	dd ?
	one_lum		dd ?
	delta_lum	dd ?
	new_lum		dd ?
    endl
	mov	[hlscolorp],rcx
	movss	dword [percent],xmm1

	mov	rax,[hlscolorp]
	mov	ecx,[rax+COLOR_HLS.hue]
	mov	[hue],ecx
	mov	ecx,[rax+COLOR_HLS.luminosity]
	mov	[luminosity],ecx
	mov	ecx,[rax+COLOR_HLS.saturation]
	mov	[saturation],ecx

	fastcall ColorHLS_NewLuma,dword [luminosity],COLOR_HLS_HILIGHT_ADJ,1
	mov	[one_lum],eax
	sub	eax,[luminosity]
	mov	[delta_lum],eax
	cvtsi2ss xmm0,dword [delta_lum]
	mulss	xmm0,dword [percent]
	cvttss2si ecx,xmm0
	mov	edx,[luminosity]
	add	edx,ecx
	mov	[new_lum],edx
	fastcall ColorHLS_ColorFromHLS,dword [hue],dword [new_lum],dword [saturation]
	ret
endp

proc ColorHLS_ScaleQ16 delta,percent_q16
	movsxd	rax,ecx
	movsxd	r10,edx
	imul	rax,r10
	cqo
	mov	r10d,COLOR_HLS_Q16_ONE
	idiv	r10
	ret
endp

proc ColorHLS_DarkerQ16 hlscolorp,percent_q16
    locals
	hue		dd ?
	luminosity	dd ?
	saturation	dd ?
	zero_lum	dd ?
	new_lum		dd ?
    endl
	mov	[hlscolorp],rcx
	mov	dword [percent_q16],edx

	mov	rax,[hlscolorp]
	mov	ecx,[rax+COLOR_HLS.hue]
	mov	[hue],ecx
	mov	ecx,[rax+COLOR_HLS.luminosity]
	mov	[luminosity],ecx
	mov	ecx,[rax+COLOR_HLS.saturation]
	mov	[saturation],ecx

	fastcall ColorHLS_NewLuma,dword [luminosity],COLOR_HLS_SHADOW_ADJ,1
	mov	[zero_lum],eax
	fastcall ColorHLS_ScaleQ16,dword [zero_lum],dword [percent_q16]
	mov	edx,[zero_lum]
	sub	edx,eax
	mov	[new_lum],edx
	fastcall ColorHLS_ColorFromHLS,dword [hue],dword [new_lum],dword [saturation]
	ret
endp

proc ColorHLS_LighterQ16 hlscolorp,percent_q16
    locals
	hue		dd ?
	luminosity	dd ?
	saturation	dd ?
	one_lum		dd ?
	delta_lum	dd ?
	new_lum		dd ?
    endl
	mov	[hlscolorp],rcx
	mov	dword [percent_q16],edx

	mov	rax,[hlscolorp]
	mov	ecx,[rax+COLOR_HLS.hue]
	mov	[hue],ecx
	mov	ecx,[rax+COLOR_HLS.luminosity]
	mov	[luminosity],ecx
	mov	ecx,[rax+COLOR_HLS.saturation]
	mov	[saturation],ecx

	fastcall ColorHLS_NewLuma,dword [luminosity],COLOR_HLS_HILIGHT_ADJ,1
	mov	[one_lum],eax
	sub	eax,[luminosity]
	mov	[delta_lum],eax
	fastcall ColorHLS_ScaleQ16,dword [delta_lum],dword [percent_q16]
	mov	edx,[luminosity]
	add	edx,eax
	mov	[new_lum],edx
	fastcall ColorHLS_ColorFromHLS,dword [hue],dword [new_lum],dword [saturation]
	ret
endp

proc ColorHLS_DarkenColorRef colorref,percDarker
    locals
	percent	dd ?
	hls	COLOR_HLS
    endl
	mov	dword [colorref],ecx
	movss	dword [percent],xmm1
	fastcall ColorHLS_FromColorRef,addr hls,dword [colorref]
	movss	xmm1,dword [percent]
	fastcall ColorHLS_Darker,addr hls,float xmm1
	ret
endp

proc ColorHLS_LightenColorRef colorref,percLighter
    locals
	percent	dd ?
	hls	COLOR_HLS
    endl
	mov	dword [colorref],ecx
	movss	dword [percent],xmm1
	fastcall ColorHLS_FromColorRef,addr hls,dword [colorref]
	movss	xmm1,dword [percent]
	fastcall ColorHLS_Lighter,addr hls,float xmm1
	ret
endp

proc ColorHLS_DarkenColorRefQ16 colorref,percent_q16
    locals
	hls	COLOR_HLS
    endl
	mov	dword [colorref],ecx
	mov	dword [percent_q16],edx
	fastcall ColorHLS_FromColorRef,addr hls,dword [colorref]
	fastcall ColorHLS_DarkerQ16,addr hls,dword [percent_q16]
	ret
endp

proc ColorHLS_LightenColorRefQ16 colorref,percent_q16
    locals
	hls	COLOR_HLS
    endl
	mov	dword [colorref],ecx
	mov	dword [percent_q16],edx
	fastcall ColorHLS_FromColorRef,addr hls,dword [colorref]
	fastcall ColorHLS_LighterQ16,addr hls,dword [percent_q16]
	ret
endp

end if
