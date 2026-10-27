# cmd-editor

A small, modular, state-machine-based text editor for the Windows command line, written in C++20.

```
keystroke -> KeyBindingRegistry -> Action -> EditorState (mode) -> TextProcessBackend -> DirtyRegion -> Renderer
```

## Build

From a Visual Studio Developer Command Prompt (or any shell with a C++20 compiler and CMake >= 3.20):

```bat
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
build\Debug\cmd_editor.exe notes.txt
```

On non-Windows hosts only the portable core library and the test suite are built
(`cmd_editor_core`, `cmd_editor_tests`); the console front-end is Windows-only.
The tests use a tiny in-tree harness, so there are no third-party dependencies.

## Using the editor

Starts in **normal** mode (vi-like).

| Mode    | Keys |
|---------|------|
| Normal  | `h j k l` / arrows move, `0` `$` line start/end, `i` insert, `a` `A` `I` `o` insert variants, `x` delete char, `u` undo, `Ctrl+R` redo, `:` command mode |
| Insert  | type text, `Enter`, `Backspace`, `Delete`, `Tab`, arrows, `Esc` back to normal |
| Command | `:w` save, `:q` quit (refuses if unsaved), `:q!` discard, `:wq`, `:x` |
| Any     | `Ctrl+S` save, `Ctrl+Z` undo, `Ctrl+Y` redo, `PgUp`/`PgDn` scroll |

## Layout and responsibilities

| Module        | Key types | Responsibility |
|---------------|-----------|----------------|
| `document/`   | `AugmentedPieceTable`, `BufferStore`, `HistoryManager`, `DocumentOperation`, `TextProcessBackend`, `IDocumentView` | Text storage (treap-backed piece table with per-subtree length and newline aggregates), operation-log undo/redo, loading/saving, line navigation |
| `datasource/` | `IDataSource`, `LocalFileDataSource`, `MemoryDataSource` | Where bytes come from/go to; the piece table never sees a file |
| `state/`      | `EditorState`, `HandlerTableState`, `StateRegistry`, `EditorContext`, `ViewState`, `CommandRegistry` | Modes, cursor/viewport state, `:` commands |
| `input/`      | `KeyEvent`, `KeyBindingRegistry`, `installDefaultKeyBindings` | Data-driven (mode, key) -> action(s) mapping |
| `actions/`    | `Action`, `ActionResult` | Semantic actions and their coordination results |
| `controller/` | `MainController` | Wires everything; no piece-table or drawing code |
| `renderer/`   | `Renderer`, `IRenderLayer`, `DirtyRegion`, `RenderedScreen`, `ITerminal`, `WindowsTerminal` | Diff-based incremental drawing and the platform console |

Dependency rules from the spec hold: the renderer sees the document only through
`IDocumentView`; the piece table depends on nothing but `BufferStore`; the backend
knows nothing about terminals; Win32 calls live only in `WindowsTerminal`.

## Design decisions worth knowing

* **Piece table.** Pieces live in an implicit treap. Each node caches subtree length and `\n` count,
  so offset lookup, line start, offset-to-line, insert and erase are `O(log P)` (plus scanning *inside*
  one piece, which skips whole 64 KiB chunks using cached per-chunk newline counts).
  Typing runs extend the previous add-buffer piece, and erasing re-fuses pieces that became contiguous.
* **Buffers.** Both buffers are append-only chunked storage; piece offsets never move.
* **Line endings.** CRLF is normalized to LF on load; if CRLF lines outnumber bare-LF lines the file is
  written back with CRLF. Lone `\r` is kept as data. A trailing newline yields a final empty line; an
  empty document has one line.
* **Undo/redo.** Operation log (`InsertOperation`, `DeleteOperation`, `CompositeOperation`), no snapshots.
  `beginGroup()/endGroup()` make several edits one undo step. Undo returns a cursor hint; history never
  owns cursor state. "Modified" tracks the save point, so undoing back to the saved state is clean again.
* **Saving.** Streams the piece list to the data source (no whole-document string). `LocalFileDataSource`
  writes `<file>.tmp~` then renames, so a failed save leaves the old file intact and the document modified.
* **Rendering.** The renderer keeps a `RenderedScreen` cache (what is on the terminal). For each dirty row
  it builds the desired cells, diffs against the cache and emits only the changed runs; blank runs use
  `clearRange`. `DirtyRegion` limits which rows are examined; because of the diff, a coarse region never
  causes extra output. Full-screen clear happens only on first draw and on resize. `DirtyRange` is recorded
  for completeness, but rows are diffed whole (a column hint would be unsafe with tabs and horizontal scroll).
* **Extensibility.** New mode: subclass `HandlerTableState`, `states().add(...)`, bind keys.
  New action: any string id plus a handler in the modes that care. New command: `commands().add(...)`.
  New UI: an `IRenderLayer`. New storage: an `IDataSource`. See `test_state_transitions.cpp` for a mode
  added from outside the library.

## Known limitations

* **Windows front-end is untested in CI here**: the console code (`WindowsTerminal.cpp`) and the CMake files
  were written against the Win32/CMake documentation but were not compiled in the environment this was
  developed in (Linux, no CMake). The portable core and all tests were built with GCC 13 and run clean under
  AddressSanitizer/UBSan. Expect to fix small issues the first time you build on Windows.
* **Text model is byte-based.** One byte is one terminal cell; control characters and bytes >= 0x80 render as
  `?`, and non-ASCII keyboard input is ignored. Files containing UTF-8 are preserved byte-for-byte on save,
  but editing inside a multi-byte character can corrupt it. No wide-character layout.
* **"Lazy" loading is chunked streaming.** The file is read in 64 KiB chunks into the original buffer
  (line counts require one pass). The `IDataSource` seam is offset-based, but on-demand paging for remote
  sources would additionally need a page-fault-capable original buffer; that is not implemented.
* `DocumentStatus::PartiallyLoaded` exists in the enum but is never entered because loading is synchronous.
* Normal mode lets the cursor rest one cell past the end of a line (unlike vi).
* No selection, search, clipboard or multi-key commands (`dd`, `gg`) yet; `ViewState::selection` is a placeholder.
