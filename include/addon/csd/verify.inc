; addon/csd/verify.inc - assemble-time layout probes for public CSD rows.
;
; Include after the CSD modules used by the source. Each block only verifies
; structures that have already been defined.

if defined CSD_CAPTION_DESCRIPTOR.id
	assert sizeof.CSD_CAPTION_DESCRIPTOR = 24
	assert CSD_CAPTION_DESCRIPTOR.id = 0
	assert CSD_CAPTION_DESCRIPTOR.glyph = 4
	assert CSD_CAPTION_DESCRIPTOR.command = 8
	assert CSD_CAPTION_DESCRIPTOR.hitCode = 12
	assert CSD_CAPTION_DESCRIPTOR.flags = 16
	assert CSD_CAPTION_DESCRIPTOR.widthDip = 20
end if

if defined CSD_CAPTION_GEOMETRY.rect
	assert sizeof.CSD_CAPTION_GEOMETRY = 16
	assert CSD_CAPTION_GEOMETRY.rect = 0
end if

if defined CSD_CAPTION_STATE.value
	assert sizeof.CSD_CAPTION_STATE = 4
	assert CSD_CAPTION_STATE.value = 0
	assert CSD_CAPTION_STATE.fadePhase = 1
	assert CSD_CAPTION_STATE.fromValue = 2
	assert CSD_CAPTION_STATE.animFlags = 3
end if

if defined CSD_CAPTION_INPUT.hotIndex
	assert sizeof.CSD_CAPTION_INPUT = 16
	assert CSD_CAPTION_INPUT.hotIndex = 0
	assert CSD_CAPTION_INPUT.pressedIndex = 4
	assert CSD_CAPTION_INPUT.tracking = 8
	assert CSD_CAPTION_INPUT.active = 12
end if

if defined CSD_THEME.generation
	assert sizeof.CSD_THEME = 52
	assert CSD_THEME.generation = 0
	assert CSD_THEME.darkMode = 4
	assert CSD_THEME.accentArgb = 8
	assert CSD_THEME.accentColor = 12
	assert CSD_THEME.captionColor = 16
	assert CSD_THEME.borderColor = 20
	assert CSD_THEME.titleTextColor = 24
	assert CSD_THEME.bodyBackColor = 28
	assert CSD_THEME.bodyTextColor = 32
	assert CSD_THEME.buttonColor = 36
	assert CSD_THEME.closeColor = 40
	assert CSD_THEME.statusTextColor = 44
	assert CSD_THEME.cornerPreference = 48
end if

if defined CSD_SNAP_POLICY.snapCapable
	assert sizeof.CSD_SNAP_POLICY = 20
	assert CSD_SNAP_POLICY.snapCapable = 0
	assert CSD_SNAP_POLICY.maxHitCode = 4
	assert CSD_SNAP_POLICY.osMajor = 8
	assert CSD_SNAP_POLICY.osMinor = 12
	assert CSD_SNAP_POLICY.osBuild = 16
end if

if defined CSD_BACKDROP_POLICY.capable
	assert sizeof.CSD_BACKDROP_POLICY = 36
	assert CSD_BACKDROP_POLICY.capable = 0
	assert CSD_BACKDROP_POLICY.osMajor = 4
	assert CSD_BACKDROP_POLICY.osMinor = 8
	assert CSD_BACKDROP_POLICY.osBuild = 12
	assert CSD_BACKDROP_POLICY.requestedType = 16
	assert CSD_BACKDROP_POLICY.appliedType = 20
	assert CSD_BACKDROP_POLICY.readbackType = 24
	assert CSD_BACKDROP_POLICY.lastResult = 28
	assert CSD_BACKDROP_POLICY.readbackResult = 32
end if

if defined CSD_BACKDROP_VIEWER_CONFIG.hostEnabled
	assert sizeof.CSD_BACKDROP_VIEWER_CONFIG = 20
	assert CSD_BACKDROP_VIEWER_CONFIG.hostEnabled = 0
	assert CSD_BACKDROP_VIEWER_CONFIG.alphaEnabled = 4
	assert CSD_BACKDROP_VIEWER_CONFIG.alphaCapable = 8
	assert CSD_BACKDROP_VIEWER_CONFIG.hostResult = 12
	assert CSD_BACKDROP_VIEWER_CONFIG.alphaResult = 16
end if

if defined CSD_TAB_ITEM.id
	assert sizeof.CSD_TAB_ITEM = 32
	assert CSD_TAB_ITEM.id = 0
	assert CSD_TAB_ITEM.flags = 4
	assert CSD_TAB_ITEM.titlep = 8
	assert CSD_TAB_ITEM.iconGlyph = 16
	assert CSD_TAB_ITEM.minWidthDip = 20
	assert CSD_TAB_ITEM.preferredDip = 24
	assert CSD_TAB_ITEM.maxWidthDip = 28
end if

if defined CSD_TAB_GEOMETRY.tabRect
	assert sizeof.CSD_TAB_GEOMETRY = 48
	assert CSD_TAB_GEOMETRY.tabRect = 0
	assert CSD_TAB_GEOMETRY.closeRect = 16
	assert CSD_TAB_GEOMETRY.titleRect = 32
end if

if defined CSD_TAB_STATE.value
	assert sizeof.CSD_TAB_STATE = 4
	assert CSD_TAB_STATE.value = 0
	assert CSD_TAB_STATE.closeValue = 1
	assert CSD_TAB_STATE.fadePhase = 2
	assert CSD_TAB_STATE.reserved = 3
end if

if defined CSD_TABSTRIP.itemp
	assert sizeof.CSD_TABSTRIP = 56
	assert CSD_TABSTRIP.itemp = 0
	assert CSD_TABSTRIP.geomp = 8
	assert CSD_TABSTRIP.statep = 16
	assert CSD_TABSTRIP.count = 24
	assert CSD_TABSTRIP.capacity = 28
	assert CSD_TABSTRIP.activeIndex = 32
	assert CSD_TABSTRIP.hotIndex = 36
	assert CSD_TABSTRIP.hotPart = 40
	assert CSD_TABSTRIP.pressedIndex = 44
	assert CSD_TABSTRIP.pressedPart = 48
	assert CSD_TABSTRIP.reserved = 52
end if

if defined CSD_VARIANT.vt
	assert sizeof.CSD_VARIANT = 24
	assert CSD_VARIANT.vt = 0
	assert CSD_VARIANT.value = 8
	assert CSD_VARIANT.extra = 16
end if

if defined UIA_RECT.left
	assert sizeof.UIA_RECT = 32
	assert UIA_RECT.left = 0
	assert UIA_RECT.top = 8
	assert UIA_RECT.width = 16
	assert UIA_RECT.height = 24
end if

if defined CSD_UIA_PROVIDER.simpleVtbl
	assert sizeof.CSD_UIA_PROVIDER = 96
	assert CSD_UIA_PROVIDER.simpleVtbl = 0
	assert CSD_UIA_PROVIDER.simpleOwner = 8
	assert CSD_UIA_PROVIDER.fragmentVtbl = 16
	assert CSD_UIA_PROVIDER.fragmentOwner = 24
	assert CSD_UIA_PROVIDER.rootVtbl = 32
	assert CSD_UIA_PROVIDER.rootOwner = 40
	assert CSD_UIA_PROVIDER.invokeVtbl = 48
	assert CSD_UIA_PROVIDER.invokeOwner = 56
	assert CSD_UIA_PROVIDER.selectionVtbl = 64
	assert CSD_UIA_PROVIDER.selectionOwner = 72
	assert CSD_UIA_PROVIDER.refCount = 80
	assert CSD_UIA_PROVIDER.slot = 84
	assert CSD_UIA_PROVIDER.reserved = 88
	assert CSD_UIA_PROVIDER.reserved2 = 92
end if
