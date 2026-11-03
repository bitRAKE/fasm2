; CPU headers to test (fasm2's x86-2.inc is included through fasm2.inc,
; which provides the data directives its encoders call):

	include 'fasm2.inc'

; Verification module:

	include 'iev.alm'

; Reference files to test:

	include 'pass/instructions.ref'
