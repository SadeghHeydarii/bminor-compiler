# Lexer Design — Manual DFA Architecture

This document summarizes the implementation strategy chosen in Phase 2 for the **B-Minor** lexical analyzer (`include/lexer/`, `src/lexer/`).

---

## 1. Implementation Method & Theoretical Grounding

- **Selected Strategy:** **Manual Deterministic Finite Automaton (`Manual DFA`)** written in standard C++17.
- **Connection to Automata Theory:** Each token pattern defined in [lexical_specification.md](lexical_specification.md) describes a regular language. Conceptually, the union of these regular expressions compiles via **Thompson's Construction (NFA) → Subset Construction (DFA) → Hopcroft Minimization** into a single deterministic recognizer. In our `Manual DFA` implementation, the minimized DFA transitions are encoded directly as deterministic state-dispatch branches using the current character and at most one lookahead character (for multi-character tokens such as `<=`, `++`, `&&`, `//`, and `/*`):
  - Linear $O(n)$ time complexity with respect to source length $n$, with zero backtracking.
  - Zero external regex dependencies.
  - Exact control over 1-based `SourceLocation` tracking (`line`, `column`), 256-character identifier/string bounds checking, and non-fatal error recovery.

---

## 2. Dual Token-Stream Output (`LexResult`)

`Lexer::tokenize()` produces a `LexResult` containing three fields ([include/lexer/lexer.hpp](../include/lexer/lexer.hpp)):

1. `allTokens`: Lossless sequence of every scanned token (`WHITESPACE`, `COMMENT_LINE`, `COMMENT_BLOCK`, `INVALID`, syntactic tokens, and trailing `END_OF_FILE`). Concatenating the lexemes of `allTokens` reproduces the exact original source byte-for-byte, enabling lossless AST-level syntax highlighting.
2. `parserTokens`: Filtered stream excluding `WHITESPACE` and comments (retaining `END_OF_FILE`) consumed directly by the `Parser`.
3. `diagnostics`: Accumulated `Diagnostic` records (`DiagnosticStage::Lexical`) with exact `SourceLocation` metadata and no process termination.
