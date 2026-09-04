"""NEWCOFF frame/CodeView regressions; run from an x64 VS developer prompt."""
import struct
import subprocess
import tempfile
from pathlib import Path

def sections(path):
    data = path.read_bytes()
    assert data[:4] == b"\0\0\xff\xff", "expected NEWCOFF bigobj"
    count, symbols, nsymbols = struct.unpack_from("<III", data, 44)
    strings = symbols + nsymbols * 20
    result = []
    for index in range(count):
        at = 56 + index * 40
        name = data[at:at + 8].rstrip(b"\0")
        if name.startswith(b"/"):
            start = strings + int(name[1:])
            name = data[start:data.index(b"\0", start)]
        size, offset = struct.unpack_from("<II", data, at + 16)
        result.append((name, data[offset:offset + size] if offset else b""))
    return result


def line_tables(entries):
    count = 0
    for name, data in entries:
        if name != b".debug$S":
            continue
        assert data[:4] == struct.pack("<I", 4), "expected CodeView C13"
        at = 4
        while at + 8 <= len(data):
            kind, size = struct.unpack_from("<II", data, at)
            count += kind == 0xF2
            at += 8 + ((size + 3) & ~3)
    return count


def main():
    here = Path(__file__).resolve().parent
    assembler = here.parent.parent / "fasm2.cmd"
    pdbutil = Path(r"C:\Program Files\LLVM\bin\llvm-pdbutil.exe")
    with tempfile.TemporaryDirectory(prefix="newcoff-frames-") as temporary:
        directory = Path(temporary)
        for level in (1, 6):
            obj = directory / f"frames{level}.obj"
            exe = directory / f"frames{level}.exe"
            subprocess.run([str(assembler), f"-iNEWCOFF.DEBUG:={level}",
                            str(here / "proc_frames.asm"), str(obj)], cwd=here, check=True)
            entries = sections(obj)
            assert {b".pdata", b".xdata", b".debug$S"} <= {n for n, _ in entries}
            assert bool(line_tables(entries)) == (level > 5)
            # /WX makes LNK4209 (discarded debug information) a failure.
            subprocess.run(["cl", "/nologo", "/W4", "/Od", "/Zi",
                            str(here / "proc_frames.c"), str(obj),
                            "/Fo:" + str(directory / "test.obj"),
                            "/Fd:" + str(directory / "compile.pdb"),
                            "/Fe:" + str(exe), "/link", "/WX", "/DEBUG:FULL",
                            "/INCREMENTAL:NO"], cwd=directory, check=True)
            subprocess.run([str(exe)], check=True)
            if pdbutil.exists():
                symbols = subprocess.check_output(
                    [str(pdbutil), "dump", "-symbols", str(exe.with_suffix(".pdb"))],
                    text=True)
                assert "frame_fixture" in symbols and "saved_seed" in symbols
        print("NEWCOFF DEBUG=1/6: unwind, USES forwarding, lines and PDB checks passed")


if __name__ == "__main__":
    main()



