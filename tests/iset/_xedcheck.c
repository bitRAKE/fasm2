/* _xedcheck.c -- decode many instructions with XED in one process.

   Reads hex bytes from stdin, one line per source line, and decodes each
   line instruction after instruction. Writes one line per input:
       OK <Intel-syntax text>[; <next instruction>...]
       ERR <xed error name> at <byte offset>
   64-bit mode unless the line starts with "16:" or "32:"; all chips
   (XED's default).

   Build against an XED kit (see readme.md):
       cl /nologo /O2 /I<kit>\include _xedcheck.c <kit>\lib\xed.lib
*/
#include <stdio.h>
#include <string.h>
#include "xed/xed-interface.h"

static int hexval(int c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}

int main(void)
{
	static char line[4096];
	xed_tables_init();
	while (fgets(line, sizeof line, stdin)) {
		xed_uint8_t buf[XED_MAX_INSTRUCTION_BYTES * 2];
		unsigned n = 0;
		int hi = -1;
		char *p = line;
		/* an optional mode: "16:" or "32:" before the bytes (default 64) */
		xed_machine_mode_enum_t mode = XED_MACHINE_MODE_LONG_64;
		xed_address_width_enum_t width = XED_ADDRESS_WIDTH_64b;
		if (!strncmp(p, "16:", 3)) {
			mode = XED_MACHINE_MODE_LEGACY_16; width = XED_ADDRESS_WIDTH_16b; p += 3;
		} else if (!strncmp(p, "32:", 3)) {
			mode = XED_MACHINE_MODE_LEGACY_32; width = XED_ADDRESS_WIDTH_32b; p += 3;
		} else if (!strncmp(p, "64:", 3)) {
			p += 3;
		}
		for (; *p && n < sizeof buf; p++) {
			int v = hexval(*p);
			if (v < 0) continue;
			if (hi < 0) hi = v;
			else { buf[n++] = (xed_uint8_t)(hi << 4 | v); hi = -1; }
		}
		/* decode instruction after instruction until the bytes run out */
		static char out[4096];
		char *o = out;
		unsigned at = 0;
		xed_error_enum_t err = XED_ERROR_NONE;
		while (at < n) {
			xed_decoded_inst_t xedd;
			unsigned left = n - at;
			xed_decoded_inst_zero(&xedd);
			xed_decoded_inst_set_mode(&xedd, mode, width);
			err = xed_decode(&xedd, buf + at, left > XED_MAX_INSTRUCTION_BYTES ? XED_MAX_INSTRUCTION_BYTES : left);
			if (err != XED_ERROR_NONE)
				break;
			char text[256];
			if (!xed_format_context(XED_SYNTAX_INTEL, &xedd, text, sizeof text, 0, 0, 0))
				strcpy(text, "?");
			o += sprintf(o, "%s%s", o == out ? "" : "; ", text);
			at += xed_decoded_inst_get_length(&xedd);
		}
		if (err != XED_ERROR_NONE)
			printf("ERR %s at %u\n", xed_error_enum_t2str(err), at);
		else
			printf("OK %s\n", out);
		fflush(stdout);
	}
	return 0;
}
