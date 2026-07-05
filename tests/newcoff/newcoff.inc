; newcoff.inc - development harness for a ground-up MS COFF backend
; ============================================================================
; Include this file BEFORE the FORMAT statement of a source, then select:
;
;   format MS NEWCOFF       ; intercepted here -> newcoffms.inc (the rewrite)
;   format MS64 NEWCOFF     ;   (same backend; use32/use64 sets the machine)
;   format MS COFF          ; forwarded to the legacy handler, unchanged
;   format MS64 COFF        ;
;
; fasmg macro definitions stack: redefining format?.MS?/format?.MS64? shadows
; the handlers installed by fasm2.inc, and invoking the macro's own name
; inside its body reaches the PREVIOUS definition. That gives us a clean
; intercept point - unknown variants fall through to the legacy
; implementation, so one source tree can carry both backends while the new
; one is developed.
;
; The backend needs no per-variant settings: it reads x86.mode in POSTPONE
; and configures the object's machine accordingly, so the two intercepts
; differ only in the initial USE mode.
;
; See readme.md in this directory for the design rationale.
; ============================================================================

macro format?.MS? variant
	match =NEWCOFF?, variant
		if ~ definite format
			format binary as 'obj'
		end if

		; ground-up COFF implementation
		include 'newcoffms.inc'
		use32
	else
		; forward to the legacy handler
		format.MS variant
	end match
end macro

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
