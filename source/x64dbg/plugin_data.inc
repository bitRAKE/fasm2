CONFIG_CAPACITY = 1024
SOURCE_CAPACITY = 4096
ERROR_PIPE_CAPACITY = 65536

PLUG_INITSTRUCT.sdkVersion = 4
PLUG_INITSTRUCT.pluginVersion = 8
PLUG_INITSTRUCT.pluginName = 12

PLUG_CB_ASSEMBLE.addr = 0
PLUG_CB_ASSEMBLE.instruction = 4
PLUG_CB_ASSEMBLE.dest = 8
PLUG_CB_ASSEMBLE.destSize = 12
PLUG_CB_ASSEMBLE.size = 16
PLUG_CB_ASSEMBLE.error = 20
PLUG_CB_ASSEMBLE.errorSize = 24
PLUG_CB_ASSEMBLE.handled = 28
PLUG_CB_ASSEMBLE.success = 29

module_handle dd 0
path_tail dd 0
pipe_read dd 0
pipe_write dd 0
bytes_read dd 0
assembly_status dd 0
configured_maximum_passes dd 100
configured_include_length dd 0
plugin_output_region MEMORY_REGION
assembly_lock rb 24
module_directory rb MAX_PATH
config_path rb CONFIG_CAPACITY
configured_include rb CONFIG_CAPACITY
include_path rb CONFIG_CAPACITY
startup_source rb CONFIG_CAPACITY
source_buffer rb SOURCE_CAPACITY

config_section db 'fasm2',0
empty_string db 0
include_key db 'IncludeDirectory',0
startup_key db 'StartupSource',0
passes_key db 'MaximumPasses',0
config_name db 'fasm2.ini',0
default_include db 0
fasm2_include db 'fasm2.inc',0
source_prefix db "include '",0
source_middle db "'",10,0
source_format db 'format binary',10,'use32',10,'org 0',0
error_config db 'Invalid fasm2 startup configuration',0
error_source db 'Only one assembly source line is accepted',0
error_length db 'The assembly source line is too long',0
error_pipe db 'Unable to create the fasmg diagnostic pipe',0
error_buffer db 'The assembly output buffer is too small',0
error_assembly db 'fasmg failed to assemble the instruction',0
