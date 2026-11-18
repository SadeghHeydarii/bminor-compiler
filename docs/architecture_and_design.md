# Architecture, Algorithms & Design Decisions

This document presents the end-to-end architecture, core algorithms, data-structure choices, and extension guidelines for Phase 1 of the **B-Minor** Compiler Front-End.

---

## 1. End-to-End Pipeline

Driven from [src/main.cpp](../src/main.cpp), compilation analysis proceeds through the following modular pipeline:

```text
Source Code (.bminor)
   │
   ▼  bminor::Lexer (Manual DFA)
LexResult { allTokens, parserTokens, diagnostics(Lexical) }
   │
   ▼  bminor::Parser (LL(1)/LL(2) Recursive-Descent + Panic-Mode Recovery)
ParseResult { ProgramNode AST, diagnostics(Syntactic) }
   │
   ▼  bminor::SyntaxHighlighter (ASTVisitor + Location Map + allTokens)
std::vector<HighlightedToken>
   ├──► bminor::formatAnsi(...) ──► ANSI Terminal Output
   └──► bminor::formatHtml(...) ──► Standalone HTML5 / Embedded CSS (No JS)
```

---

## 2. Module Breakdown & Data-Structure Rationale

### 2.1. Shared Diagnostics & Source Positions (`include/common/`)
- **[source_location.hpp](../include/common/source_location.hpp):** `SourceLocation` stores `filename`, 1-based `line`, and 1-based `column`. Formatting helpers `toLineColString()` (`"3:12"`) and `toString()` (`"file:3:12"`) ensure consistent diagnostic and AST position output across all stages.
- **[diagnostic.hpp](../include/common/diagnostic.hpp):** `Diagnostic` distinguishes `DiagnosticStage::Lexical` from `DiagnosticStage::Syntactic`, recording the error stage, exact source location, and human-readable message.

### 2.2. Lexical Analyzer (`include/lexer/`, `src/lexer/`)
- References: [lexical_specification.md](lexical_specification.md), [lexer_design.md](lexer_design.md).
- **Manual DFA:** Scans source input in $O(n)$ single-pass time with deterministic transitions, enforcing **Maximal Munch**, **Keyword-before-Identifier** priority across the 15 reserved keywords, and 256-character limits on identifiers and string literals.
- **Lossless + Filtered Token Streams:** `LexResult` provides both `allTokens` (including whitespace, comments, and `INVALID` tokens) and `parserTokens` (structural tokens + `END_OF_FILE`), preventing any loss of formatting or comment text needed during highlighting.

### 2.3. Abstract Syntax Tree (`include/ast/ast.hpp`, `src/ast/ast.cpp`)
- **Ownership & Hierarchy:** Uses `std::unique_ptr` throughout an abstract hierarchy rooted at `ASTNode` with specialized bases `DeclNode`, `StmtNode`, `ExprNode`, `TypeNode`, `InitializerNode`, `ParamNode`, and `ProgramNode`. Recovery placeholders (`ErrorExpr`, `ErrorStmt`) allow partially valid subtrees to stay well-formed during syntax recovery.
- **Mandatory Fields on Every `ASTNode`:**
  1. `location` (`SourceLocation`): 1-based line and column of the construct's first token (e.g., `Identifier(name='n', loc=3:12)`, `CallExpr(callee='factorial', loc=3:16)`).
  2. `typeAnnotation` (`std::optional<std::string>`): Initialized to `std::nullopt` during Phase 1 AST construction and reserved for Semantic Analysis in subsequent project phases.
- **Visitor Pattern (`ASTVisitor`):** Separates tree traversal from node representation, cleanly powering both `dumpAst()` (`--parse`) and `AstHighlightVisitor` (`--ansi` / `--html`).

### 2.4. Recursive-Descent Parser & Panic-Mode Recovery (`include/parser/`, `src/parser/`)
- References: [grammar_ebnf.md](grammar_ebnf.md), [parser_compatibility.md](parser_compatibility.md).
- **Parsing Algorithm:** Deterministic `LL(1)` recursive descent with a single `LL(2)` lookahead (`IDENT` + `COLON`) in `parseStmt()`. Shared template helper `Parser::parseBinaryLeft` handles all left-associative operator precedence levels cleanly without code duplication, while `parseAssign()` and `parsePower()` use right recursion for right-associative `=` and `^`.
- **Panic-Mode Error Recovery:**
  - Filters out trivia and lexer-already-diagnosed `INVALID` tokens so syntax diagnostics do not duplicate lexical errors.
  - Uses `panicMode_` to suppress cascading error spam inside a single broken expression or statement.
  - Special handling in `expectClosingParenBeforeBlockOrStmt()` reports `missing ')' before '{'` while leaving `{` unconsumed so the conditional block body still parses intact.
  - Combines statement synchronization (`;`, `}`, `{`, `if`, `for`, `return`, `print`, `IDENT ':'`, `EOF`), brace-depth-aware declaration synchronization (`synchronizeDecl()`), and loop progress guards (`startPos`) to guarantee termination without crashes.

### 2.5. AST-Level Syntax Highlighter & Output Formatters (`include/highlighter/`, `include/output/`)
- **Context-Sensitive Classification ([highlighter.cpp](../src/highlighter/highlighter.cpp)):**
  - Rather than regex matching or token-stream heuristics, `AstHighlightVisitor` walks the parsed AST and maps exact 64-bit `(line, column)` location keys into `roleByLocation`.
  - Function declarations (`FuncDecl::nameLocation`) and call targets (`CallExpr::callee` when `IdentifierExpr`) receive `HighlightCategory::FuncIdentifier`, whereas `VarDecl`, `ParamNode`, and `IdentifierExpr` receive `HighlightCategory::VarIdentifier`—correctly distinguishing functions from variables even when both share the exact same identifier spelling.
  - `INVALID` tokens and non-EOF syntactic diagnostic positions map to `HighlightCategory::Error`.
- **Color Category Mapping ([formatter.cpp](../src/output/formatter.cpp)):**

| Category | Role | ANSI Escape Sequence | HTML CSS Class |
| :--- | :--- | :--- | :--- |
| `Keyword` | Structural keywords (`if`, `return`, `function`, `array`, ...) | `\x1b[1;34m` (Bold blue) | `.kw` |
| `TypeName` | Type names (`integer`, `boolean`, `char`, `string`, `void`) | `\x1b[36m` (Teal / cyan) | `.type` |
| `VarIdentifier` | Variable & parameter identifiers | Default / plain (White / default) | `.var` |
| `FuncIdentifier` | Function declaration & call identifiers | `\x1b[93m` (Yellow / gold) | `.fn` |
| `IntLiteral` | Integer constants | `\x1b[38;5;208m` (Orange) | `.lit` |
| `BoolLiteral` | Boolean constants (`true`, `false`) | `\x1b[38;5;208m` (Orange) | `.lit` |
| `StringOrCharLiteral` | String and character literals | `\x1b[32m` (Warm green) | `.str` |
| `Operator` | Arithmetic, relational, logical, postfix, assignment operators | `\x1b[37m` (Light gray) | `.op` |
| `Comment` | Single-line (`//`) and block (`/* ... */`) comments | `\x1b[2;3;90m` (Dim gray, italic) | `.comment` |
| `Error` | Lexical `INVALID` tokens and syntax error locations | `\x1b[4;91m` (Red underline) | `.err` |

- **Output Format Guarantees:**
  - `formatAnsi()` wraps colored tokens with `\x1b[0m` resets while passing whitespace and delimiters through verbatim, ensuring ANSI stripping restores the exact source text.
  - `formatHtml()` produces a self-contained HTML5 document with an embedded `<style>` sheet, `<pre class="bminor-code"><code>...</code></pre>`, full HTML entity escaping (`&`, `<`, `>`, `"`, `'`), and zero JavaScript.

---

## 3. Extension Points for Future Phases

- **Adding a Keyword or Token:** Add the enum entry in [include/lexer/token.hpp](../include/lexer/token.hpp), update `lookupKeywordOrIdent()` or DFA dispatch in [src/lexer/lexer.cpp](../src/lexer/lexer.cpp), and update `formatToken()` in [src/lexer/token.cpp](../src/lexer/token.cpp).
- **Adding a Language Construct:** Add the AST node in [include/ast/ast.hpp](../include/ast/ast.hpp), extend `ASTVisitor` and `AstDumper`, parse the construct in [src/parser/parser.cpp](../src/parser/parser.cpp), and register its highlight roles in [src/highlighter/highlighter.cpp](../src/highlighter/highlighter.cpp).
- **Semantic Analysis Integration (Next Course Phase):** Implement a symbol table and type-checking `ASTVisitor` that populates the pre-existing `ASTNode::typeAnnotation` field without modifying the `Lexer` or `Parser`.
