; addon/csd/theme.inc - DWM/theme helpers for CSD examples.

include 'addon/system_dark_mode.inc'

CSD_THEME_FALLBACK_ARGB	= 0FF3E8B52h
CSD_THEME_LIGHT_BODY	= 00FFFFFFh
CSD_THEME_LIGHT_TEXT	= 00242120h
CSD_THEME_LIGHT_BUTTON	= 00D7E8D2h
CSD_THEME_DARK_BODY	= 00242120h
CSD_THEME_DARK_TEXT	= 00F4F7F2h
CSD_THEME_DARK_BUTTON	= 00434D45h
CSD_THEME_CLOSE		= 004242D7h
CSD_THEME_WHITE		= 00FFFFFFh
CSD_THEME_DARK_CAPTION	= 002D332Fh

struct CSD_THEME
  generation		dd ?
  darkMode		dd ?	; DWMWA_USE_IMMERSIVE_DARK_MODE
  accentArgb		dd ?
  accentColor		dd ?
  captionColor		dd ?	; DWMWA_CAPTION_COLOR
  borderColor		dd ?	; DWMWA_BORDER_COLOR
  titleTextColor	dd ?	; DWMWA_TEXT_COLOR
  bodyBackColor		dd ?
  bodyTextColor		dd ?
  buttonColor		dd ?
  closeColor		dd ?
  statusTextColor	dd ?
  cornerPreference	dd ?	; DWMWA_WINDOW_CORNER_PREFERENCE
ends

proc CsdDwmSetDwordAttribute hwnd,attr,value
	mov	[value],r8d
	invoke	DwmSetWindowAttribute,rcx,edx,addr value,4
	ret
endp

proc CsdThemeRefresh uses rbx, themep
    locals
	color_argb	dd ?
	opaque_blend	dd ?
	dark_mode	dd ?
	changed		dd ?
    endl
	mov	rbx,rcx
	mov	dword [color_argb],CSD_THEME_FALLBACK_ARGB
	mov	dword [opaque_blend],0
	invoke	DwmGetColorizationColor,addr color_argb,addr opaque_blend
	fastcall WindowsAppsUseDarkMode
	mov	[dark_mode],eax

	mov	dword [changed],0
	cmp	dword [rbx+CSD_THEME.generation],0
	je	theme_mark_changed
	mov	eax,[rbx+CSD_THEME.accentArgb]
	cmp	eax,[color_argb]
	jne	theme_mark_changed
	mov	eax,[rbx+CSD_THEME.darkMode]
	cmp	eax,[dark_mode]
	je	theme_assign

  theme_mark_changed:
	mov	dword [changed],1

  theme_assign:
	mov	eax,[color_argb]
	mov	[rbx+CSD_THEME.accentArgb],eax
	bswap	eax
	shr	eax,8
	mov	[rbx+CSD_THEME.accentColor],eax
	mov	[rbx+CSD_THEME.borderColor],eax

	mov	eax,[dark_mode]
	mov	[rbx+CSD_THEME.darkMode],eax
	test	eax,eax
	jz	theme_light

	mov	eax,[rbx+CSD_THEME.accentColor]
	mov	[rbx+CSD_THEME.captionColor],eax
	mov	dword [rbx+CSD_THEME.titleTextColor],CSD_THEME_WHITE
	mov	dword [rbx+CSD_THEME.bodyBackColor],CSD_THEME_DARK_BODY
	mov	dword [rbx+CSD_THEME.bodyTextColor],CSD_THEME_DARK_TEXT
	mov	dword [rbx+CSD_THEME.buttonColor],CSD_THEME_DARK_BUTTON
	mov	dword [rbx+CSD_THEME.closeColor],CSD_THEME_CLOSE
	mov	eax,[rbx+CSD_THEME.accentColor]
	mov	[rbx+CSD_THEME.statusTextColor],eax
	mov	dword [rbx+CSD_THEME.cornerPreference],DWMWCP_ROUND
	jmp	theme_done

  theme_light:
	mov	eax,[rbx+CSD_THEME.accentColor]
	mov	[rbx+CSD_THEME.captionColor],eax
	mov	dword [rbx+CSD_THEME.titleTextColor],CSD_THEME_WHITE
	mov	dword [rbx+CSD_THEME.bodyBackColor],CSD_THEME_LIGHT_BODY
	mov	dword [rbx+CSD_THEME.bodyTextColor],CSD_THEME_LIGHT_TEXT
	mov	dword [rbx+CSD_THEME.buttonColor],CSD_THEME_LIGHT_BUTTON
	mov	dword [rbx+CSD_THEME.closeColor],CSD_THEME_CLOSE
	mov	eax,[rbx+CSD_THEME.accentColor]
	mov	[rbx+CSD_THEME.statusTextColor],eax
	mov	dword [rbx+CSD_THEME.cornerPreference],DWMWCP_ROUND
  theme_done:
	cmp	dword [changed],0
	je	theme_return
	inc	dword [rbx+CSD_THEME.generation]
  theme_return:
	mov	eax,[rbx+CSD_THEME.generation]
	ret
endp

proc CsdThemeApplyDwm uses rbx rsi, hwnd,themep
	mov rbx, rcx
	mov rsi, rdx
	iterate <member,		enumeration>,\
		darkMode,		DWMWA_USE_IMMERSIVE_DARK_MODE,\
		cornerPreference,	DWMWA_WINDOW_CORNER_PREFERENCE,\
		captionColor,		DWMWA_CAPTION_COLOR,\
		borderColor,		DWMWA_BORDER_COLOR,\
		titleTextColor,		DWMWA_TEXT_COLOR,\

		invoke DwmSetWindowAttribute, rbx, enumeration,\
			addr rsi+CSD_THEME.member, 4
	end iterate
	ret
endp
