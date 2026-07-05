The PE/COFF specification defines seven explicit COMDAT selection criteria. While you are currently utilizing `EXACT_MATCH` (4) for data deduplication and leveraging the COMDAT structure for unreferenced section GC, the remaining selection types dictate how the linker resolves multiple definitions of the same symbol across object files.

If you are targeting ELF, note that the specification flattens much of this logic: it primarily uses `GRP_COMDAT` for "Any" semantics and `SHT_GROUP` for associative linking. The detailed selection types below are native to PE/COFF auxiliary symbol records.

Here is how the remaining COMDAT selection types function, applied to an architecture of algorithms and data tables.

### COMDAT Selection Types

| Value | Selection Type | Resolution Behavior |
| --- | --- | --- |
| `1` | `NODUPLICATES` | Throws a linker error if multiple definitions exist. |
| `2` | `ANY` | Arbitrarily selects one definition and discards the rest. |
| `3` | `SAME_SIZE` | Selects any definition, but throws an error if sizes differ. |
| `5` | `ASSOCIATIVE` | Links the section's retention to a primary leader section. |
| `6` | `LARGEST` | Selects the definition with the largest byte size. |
| `7` | `NEWEST` | Selects the definition with the most recent timestamp. |

---

### 1. `NODUPLICATES` (Value 1)

This type enforces the One Definition Rule (ODR) strictly. While it behaves like a standard non-COMDAT section by throwing a multiple-definition error if duplicates are found, wrapping the data in a COMDAT section allows it to be aggressively garbage-collected if unreferenced (`/OPT:REF`).

* **Application:** Your primary `dispatch_algorithm()` function. It must only be defined exactly once in the final binary. Putting it in a `NODUPLICATES` COMDAT ensures it can be stripped if the entire subsystem goes unused, while preventing accidental redefinitions.

### 2. `ANY` (Value 2)

The linker encounters multiple definitions, arbitrarily picks the first one it processes, and discards all others.

* **Application:** Generic, non-specialized inline functions or template instantiations (e.g., a fallback `scalar_compute()` method). Because the compiler guarantees all emitted object files contain functionally identical instructions for these inline functions, the linker can safely retain whichever copy it sees first.

### 3. `SAME_SIZE` (Value 3)

Similar to `ANY`, the linker will arbitrarily pick one definition, but it validates that all discarded definitions have the exact same byte size as the chosen one. If a size mismatch occurs, the linker throws an error.

* **Application:** Jump tables or vtables. If different translation units compile the same table, the exact bytes might differ due to local label offsets or relocations, meaning `EXACT_MATCH` would fail. However, `SAME_SIZE` ensures that the ABI footprint of the table remains perfectly strictly aligned across object files.

### 4. `ASSOCIATIVE` (Value 5)

This does not resolve definitions on its own; instead, it creates a dependency tree. An associative COMDAT section specifies another section (a "leader") in its auxiliary record. If the linker retains the leader, it retains the associative section. If the leader is discarded (either through COMDAT deduplication or GC), the associative section is discarded.

* **Application:** Linking your ISA-specific algorithms to their specific data tables. If you compile an `avx512_compute` `.text` section, you can mark its accompanying `.rdata` lookup table, `.pdata` stack unwind records, and `.debug` info as `ASSOCIATIVE`. If the linker selects a different implementation or drops `avx512_compute` via GC, the associated metadata and tables are automatically stripped, preventing binary bloat.

### 5. `LARGEST` (Value 6)

The linker collects all definitions, selects the one with the largest size, and discards the rest.

* **Application:** Uninitialized shared scratchpad buffers (`.bss`). If module A requires a 256-byte scratchpad for a shared algorithmic task, and module B requires a 512-byte scratchpad for the same task, emitting them as `LARGEST` ensures the linker provisions the 512-byte block, safely accommodating both modules without duplicating the buffer.

### 6. `NEWEST` (Value 7)

The linker compares the timestamps of the object files containing the definitions and selects the most recently compiled one.

* **Application:** Historically used for dynamic linking and library versioning, ensuring the linker picked the most updated patched function from a set of static libraries. In modern toolchains, this is effectively deprecated because reproducible builds rely on zeroed or deterministic timestamps, rendering `NEWEST` non-functional.
