# chukl-levi

Rebuild of the CHUKL Meteor addon (Java) as a LeviLaunchroid native mod (`.levipack`).
Not a code port: the Java code depends on Meteor and Java Minecraft internals. Only the behavior is reproduced.

## Status
| Part | State |
|---|---|
| Freecam / FreeLook / FullBright logic (`src/core`) | Done, host unit tests pass (`ctest`) |
| SDK glue (`src/mod`, `src/main.cpp`) | Skeleton. Not compiled against the real SDK; see `SDK-CHECK` markers |
| `manifest.json` | Keys are a guess. Copy the real one from `examples/full-cpp-mod` in LeviLaunchroid |
| Android build (`CMakeLists.txt`) | SDK block must be pasted from Camera Overhaul's CMakeLists |
| Game hooks (camera, input, brightness) | Not done. Need offsets/RTTI work on libminecraftpe.so for your version |
| ESPs, chunk/base finders | Not started |

## Next steps
1. Replace `manifest.json` and fill the SDK block in `CMakeLists.txt` from the references above.
2. Make the empty mod build and import with one visible toggle (verify the SDK-CHECK lines).
3. Implement the hooks in `ChuklLeviMod::installHooks`, one feature at a time.
