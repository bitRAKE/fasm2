#include <windows.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

extern uint64_t frame_fixture(uint64_t);
int main(void);
static int inside(void *address, void *function) {
    DWORD64 base;
    PRUNTIME_FUNCTION entry = RtlLookupFunctionEntry((DWORD64)function, &base, NULL);
    assert(entry);
    return (DWORD64)address >= base + entry->BeginAddress &&
           (DWORD64)address < base + entry->EndAddress;
}
void frame_probe(void) {
    void *frames[32];
    unsigned count = CaptureStackBackTrace(0, 32, frames, NULL);
    int fixture = 0, caller = 0;
    for (unsigned i = 0; i < count; ++i) {
        if (inside(frames[i], (void *)frame_fixture)) fixture = 1;
        if (fixture && inside(frames[i], (void *)main)) caller = 1;
    }
    assert(caller); /* Walk must get past the assembly frame. */
}
int main(void) {
    assert(frame_fixture(1234) == 1344);
    puts("NEWCOFF multi-register frame and native stack walk passed");
    return 0;
}
