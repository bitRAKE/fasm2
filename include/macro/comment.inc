; Mult-line Comment Blocks (dual mode, nesting)
;	comment
;		<comment lines>
;	end comment
;
;	comment |
;		<comment lines>
;	|
;
; Notes:
;   - custom delimiters are case-sensitive
;
define comment? comment?

namespace comment?

	define ENDER

	calminstruction start &line&
		take ENDER,line
		arrange line,=mvmacro ?,=comment?=.=intercept
		assemble line
	end calminstruction

	calminstruction intercept! &line&
		local ender,text,nested
		match ,ENDER
		jno custom_close
		match =END? =COMMENT?,line
		jyes clear
	maybe_nested:
		match =COMMENT? nested?,line
		jno skip
		take ENDER,nested
	skip:	exit

	custom_close:
		arrange ender,ENDER
		arrange text,line
		stringify ender
		stringify text
		check ender=text
		jno maybe_nested
	clear:
		take ,ENDER
		take ENDER,ENDER
		jyes skip
		arrange line,=mvmacro =comment?=.=intercept,?
		assemble line
	end calminstruction

	restore ENDER

end namespace

mvmacro comment?,comment?.start
