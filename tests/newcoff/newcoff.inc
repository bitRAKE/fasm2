; newcoff.inc - development harness for a ground-up MS COFF backend
; ============================================================================
; Include this file BEFORE the FORMAT statement of a source, then select:
;
;   format MS64 NEWCOFF     ; intercepted here -> newcoffms.inc (the rewrite)
;   format MS64 COFF        ; forwarded to the legacy handler, unchanged
;
; fasmg macro definitions stack: defining format?.MS64? again shadows the
; handler installed by fasm2.inc, and invoking the macro's own name inside
; its body reaches the PREVIOUS definition. That gives us a clean intercept
; point - unknown variants fall through to the legacy implementation, so one
; source tree can carry both backends while the new one is developed.
;
; See readme.md in this directory for the design rationale.
; ============================================================================

macro format?.MS64? variant
	match =NEWCOFF?, variant
		if ~ definite format
			format binary as 'obj'
		end if

		; ground-up COFF implementation
		include 'newcoffms.inc'
		use64
	else
		; forward to the legacy handler
		format.MS64 variant
	end match
end macro
