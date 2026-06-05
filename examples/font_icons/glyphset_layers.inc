; glyphset_layers.inc - drop-in layered glyph configurations for glyphset.asm.
;
; Paste `uwpchar` Layering output blocks into a namespace, then add one
; GLYPHSET_ITEM row in glyphset_user_items. Namespaces let every pasted block
; keep the default `icon_layers` label without collisions.

macro glyphset_user_strings
	glyphset_heart_title GLOBWSTR 'Heart alert',0
	glyphset_wifi_title GLOBWSTR 'Wi-Fi transfer warning',0
	glyphset_click_title GLOBWSTR 'Click target',0
	glyphset_french_title GLOBWSTR 'Star non française',0
purge glyphset_user_strings
end macro

macro glyphset_user_layers
	namespace glyphset_heart
		label icon_layers:icon_layers.bytes/sizeof.FONTICON_LAYER
			FONTICON_LAYER glyph: 0EB52h, color: 000000DCh ; HeartFill
			FONTICON_LAYER glyph: 0EB51h, color: 00FFFFFFh ; Heart
		.bytes = $ - icon_layers
	end namespace

	namespace glyphset_wifi
		label icon_layers:icon_layers.bytes/sizeof.FONTICON_LAYER
			FONTICON_LAYER glyph: 0EB5Ah, color: 0000FF00h ; WifiError0
			FONTICON_LAYER glyph: 0EB5Bh, color: 000000FFh ; WifiError1
			FONTICON_LAYER glyph: 0EBFFh, color: 00F7F2EEh ; StatusErrorLeft
			FONTICON_LAYER glyph: 0E880h, color: 00FFFF00h ; StatusDataTransfer
		.bytes = $ - icon_layers
	end namespace

	namespace glyphset_click
		label icon_layers:icon_layers.bytes/sizeof.FONTICON_LAYER
			FONTICON_LAYER glyph: 0F8AFh, color: 0000D7FFh ; ClickSolid
			FONTICON_LAYER glyph: 0E8B0h, color: 00F7F2EEh ; Click
		.bytes = $ - icon_layers
	end namespace

	namespace glyphset_french
		label icon_layers:icon_layers.bytes/sizeof.FONTICON_LAYER
			FONTICON_LAYER glyph: 0E735h, color: 00FFFFFFh ; FavoriteStarFill
			FONTICON_LAYER glyph: 0F0E6h, color: 000000FFh ; OutlineQuarterStarRight
			FONTICON_LAYER glyph: 0F0E5h, color: 00FF0000h ; OutlineQuarterStarLeft
			FONTICON_LAYER glyph: 0E734h, color: 00FF8080h ; FavoriteStar
		.bytes = $ - icon_layers
	end namespace

purge glyphset_user_layers
end macro

macro glyphset_user_items
	dq glyphset_heart_title,glyphset_heart.icon_layers
	dd sizeof glyphset_heart.icon_layers
	align 8
	dq glyphset_wifi_title,glyphset_wifi.icon_layers
	dd sizeof glyphset_wifi.icon_layers
	align 8
	dq glyphset_click_title,glyphset_click.icon_layers
	dd sizeof glyphset_click.icon_layers
	align 8
	dq glyphset_french_title,glyphset_french.icon_layers
	dd sizeof glyphset_french.icon_layers
	align 8
purge glyphset_user_items
end macro
