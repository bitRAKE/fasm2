"""Check CodeView local widths/offsets; run from an x64 VS developer prompt."""
import struct
import subprocess
import tempfile
from pathlib import Path

from check_proc_frames import sections


def locals_in_object(path):
    result = {}
    for name, data in sections(path):
        if name != b".debug$S":
            continue
        assert data[:4] == struct.pack("<I", 4), "expected C13"
        at = 4
        while at + 8 <= len(data):
            kind, size = struct.unpack_from("<II", data, at)
            start, end = at + 8, at + 8 + size
            assert end <= len(data)
            if kind == 0xF1:  # DEBUG_S_SYMBOLS
                record = start
                while record < end:
                    length, symbol = struct.unpack_from("<HH", data, record)
                    assert length >= 2 and record + 2 + length <= end
                    if symbol == 0x1111:  # S_REGREL32
                        offset, type_index, register = struct.unpack_from("<iIH", data, record + 4)
                        label = data[record + 14:record + 2 + length].split(b"\0", 1)[0].decode()
                        assert label not in result, label
                        result[label] = (offset, type_index, register)
                    record += 2 + length
            at = start + ((size + 3) & ~3)
    return result


def main():
    here = Path(__file__).resolve().parent
    assembler = here.parent.parent / "fasm2.cmd"
    with tempfile.TemporaryDirectory(prefix="newcoff-local-types-") as temporary:
        for level in (1, 6):
            obj = Path(temporary) / f"types{level}.obj"
            subprocess.run([str(assembler), f"-iNEWCOFF.DEBUG:={level}",
                            str(here / "cv_local_types.asm"), str(obj)], cwd=here, check=True)
            fields = locals_in_object(obj)
            assert len(fields) == 18, fields
            base = fields["a"][0]
            for index, name in enumerate("abcdefgh"):
                assert fields[name] == (base + 4 * index, 0x75, 335), (name, fields[name])
            assert fields["fifth"] == (base + 32, 0x77, 335)
            assert fields["local_dword"][1:] == (0x75, 335)
            assert fields["local_qword"][1:] == (0x77, 335)
            expected = {"default_dword": (8, 0x75), "explicit_signed": (12, 0x74),
                        "explicit_qword": (16, 0x77), "default_qword": (20, 0x77),
                        "pointer": (28, 0x603), "legacy_dword": (36, 0x75),
                        "legacy_qword": (40, 0x77)}
            for name, (offset, type_index) in expected.items():
                assert fields[name] == (offset, type_index, 335), (name, fields[name])
    print("NEWCOFF DEBUG=1/6: DWORD/QWORD defaults, packed homes, explicit types and offsets passed")


if __name__ == "__main__":
    main()
