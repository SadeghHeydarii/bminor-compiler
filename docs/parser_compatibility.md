# Grammar Compatibility Analysis — LL(k) Recursive-Descent Parser

This document demonstrates that the EBNF grammar in [grammar_ebnf.md](grammar_ebnf.md) is strictly free of left recursion and deterministically parseable using a hand-written **Recursive-Descent Parser (`LL(1)` / `LL(2)`)**.

---

## 1. Selected Parsing Strategy

- **Strategy:** Top-down **Recursive-Descent Parser** (`Parser` class in [include/parser/parser.hpp](../include/parser/parser.hpp)) with one method per non-terminal.
- **Lookahead Requirement:** Strictly **`LL(1)`** across the entire grammar, except at a single disambiguation point in `parseStmt()` that uses **`LL(2)`** (`IDENT` followed by `:`) to separate `local_var_decl` from `expr_stmt` in $O(1)$ without backtracking.

---

## 2. Left-Recursion Elimination

Recursive-descent parsers enter infinite recursion on direct or indirect left recursion. Every potentially recursive rule in B-Minor is handled as follows:

| Non-Terminal | Status | Resolution in Grammar & Parser |
| :--- | :---: | :--- |
| `program`, `declaration`, `block` | Free | Iterative `declaration*` / `stmt*` loops bounded by `EOF` / `}` |
| `param_list`, `expr_list` | Free | Iteration `( ',' entity )*` instead of left-recursive lists |
| `logical_or`, `logical_and`, `comparison`, `additive`, `multiplicative` | Free | Left-associative iteration `( op rhs )*` implemented via shared helper `Parser::parseBinaryLeft` |
| `assign`, `power` | Free | Right-associative (`a = b = 1`, `a ^ b ^ c`), expressed cleanly via right recursion |
| `unary` | Free | Prefix terminal (`'-'` \| `'!'`) precedes recursive call |
| `postfix` | Free | Factored as `primary postfix_op*` so chains like `f(x)[0]++` parse iteratively left-to-right |

---

## 3. `FIRST` Sets & Conflict Resolution

### Key `FIRST` Sets
- `FIRST(declaration)` = `{ IDENT }`
- `FIRST(function_tail)` = `{ 'function' }`
- `FIRST(var_tail)` = `FIRST(type)` = `{ 'integer', 'boolean', 'char', 'string', 'array' }`
- `FIRST(return_type)` = `FIRST(type) ∪ { 'void' }`
- `FIRST(if_stmt)` = `{ 'if' }`, `FIRST(for_stmt)` = `{ 'for' }`, `FIRST(return_stmt)` = `{ 'return' }`, `FIRST(print_stmt)` = `{ 'print' }`, `FIRST(block)` = `{ '{' }`
- `FIRST(expr_stmt)` = `FIRST(expr)` = `{ '-', '!', '(', IDENT, INT_LITERAL, BOOL_LITERAL, CHAR_LITERAL, STRING_LITERAL }`
- `FIRST(postfix_op)` = `{ '[', '(', '++', '--' }`

### Disambiguation Points
1. **`declaration` (Function vs. Global Variable):** Left-factored into `IDENT ':' ( function_tail | var_tail )`. `FIRST(function_tail) ∩ FIRST(var_tail) = ∅` → **`LL(1)`**.
2. **`stmt` (`local_var_decl` vs. `expr_stmt` on `IDENT`):**
   - When `peek(0) == IDENT` and `peek(1) == COLON` → `parseLocalVarDeclStmt()` (`LL(2)`).
   - Otherwise → `parseExprStmt()`. Zero backtracking.
3. **`assign` (`logical_or ( '=' assign )?`):** Avoids factoring lvalues against arbitrary expressions by parsing `logical_or` first and descending into `'=' assign` only if `peek() == OP_ASSIGN`.
4. **Dangling `else` in `if_stmt`:** `parseIfStmt()` greedily consumes an immediately following `KW_ELSE`, binding `else` deterministically to the innermost enclosing `if`.
5. **Optional Clauses (`for`, `return`, `print`, `array_type`):** In every case, `FIRST(expr)` / `FIRST(expr_list)` is disjoint from the closing delimiter (`';'`, `')'`, `']'`).
6. **Chained `postfix_op*`:** `[`, `(`, `++`, `--` do not overlap with any binary/prefix continuation operator, keeping the `while` loop in `parsePostfix()` purely `LL(1)`.

---

## 4. Synchronization Points for Panic-Mode Recovery

To prevent cascading diagnostics and crashes on invalid inputs:
- **Declaration Level (`synchronizeDecl`):** Tracks `{` / `}` brace depth and synchronizes on `;` or matching `}` at depth `0`, or a fresh top-level `IDENT ':'` header, or `END_OF_FILE`.
- **Statement Level (`synchronizeStmt`):** Synchronizes on `;` (consumed), `}`, `{` (left unconsumed so missing-`)` recovery before `{` preserves the block body), statement keywords (`if`, `for`, `return`, `print`), local `IDENT ':'` declarations, and `END_OF_FILE`.
- **Forward-Progress Guards:** Loop iterations in `parse()` and `parseBlock()` verify `current_` advances or explicitly advance past unexpected tokens before `EOF`, guaranteeing termination on arbitrary malformed streams.
