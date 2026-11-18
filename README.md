# B-Minor Compiler Front-End & AST Syntax Highlighter

![Language](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)
![Dependencies](https://img.shields.io/badge/Dependencies-STL%20Only-brightgreen.svg)
![Build](https://img.shields.io/badge/Build-Make%20%2F%20g%2B%2B-orange.svg)
![Target](https://img.shields.io/badge/Target-B--Minor-yellow.svg)

A modular, zero-dependency **C++17** compiler front-end and IDE syntax-highlighting tool for the **B-Minor** programming language (Amirkabir University of Technology — Compiler Design Course, Fall 1405 / Phase 1).

---

## Architecture Overview

```text
Source Code (.bminor)
   │
   ▼  Manual DFA Lexer (O(n), 1-char lookahead)
LexResult { allTokens, parserTokens, Lexical diagnostics }
   │
   ▼  LL(1)/LL(2) Recursive-Descent Parser + Panic-Mode Recovery
ParseResult { ProgramNode AST, Syntactic diagnostics }
   │
   ▼  AST-Level SyntaxHighlighter (ASTVisitor + (line, col) role map)
HighlightedToken Stream
   ├──► formatAnsi(...) ──► ANSI Terminal Output
   └──► formatHtml(...) ──► Standalone HTML5 + Embedded CSS (No JS)
```

### Implemented Capabilities

- **Manual DFA Lexical Analyzer (`Lexer`):**
  - Supports all **11 B-Minor token categories**: 15 reserved keywords (`array`, `boolean`, `char`, `else`, `false`, `for`, `function`, `if`, `integer`, `print`, `return`, `string`, `true`, `void`, `while`), identifiers, integer literals, boolean literals, string literals with escapes, character literals, operators, delimiters, single-line comments (`//`), block comments (`/* ... */`), and whitespace.
  - Enforces **Longest Match (`Maximal Munch`)**, **Keyword-before-Identifier** priority, and **256-character limits** on identifiers and string literals.
  - Emits two token streams: `parserTokens` (structural tokens + `END_OF_FILE`) and `allTokens` (lossless stream preserving whitespace, comments, and `INVALID` tokens for exact source reconstruction).
  - Recovers from unrecognized characters, unterminated string/character literals, and unterminated block comments without aborting.

- **Abstract Syntax Tree (`AST`) & `LL(1)` / `LL(2)` Recursive-Descent `Parser`:**
  - Strict left-recursion-free EBNF implementation using `LL(1)` everywhere except a single `LL(2)` check (`IDENT ':'`) in `Parser::parseStmt()`.
  - Handles all B-Minor declarations (global/local variables, arrays, nested initializers, function prototypes and definitions), control flow (`if`, greedy dangling-`else`, `for`, `return`, `print`, blocks), and full operator precedence/associativity (right-associative `=` and `^`, left-associative arithmetic/relational/logical operators, prefix unary `-`/`!`, and chained postfix `[ ]`, `( )`, `++`, `--`).
  - Every `ASTNode` carries `location` (1-based `SourceLocation` of its first token) and `typeAnnotation` (`std::optional<std::string>`, initialized to `std::nullopt` for future semantic analysis).
  - **Panic-Mode Error Recovery:** Reports accurate diagnostics (e.g., `expected expression, got ';'` and `missing ')' before '{'`), avoids swallowing `{` on missing `)` before block bodies, synchronizes on `;`, `}`, `{`, statement keywords, `IDENT ':'`, and brace-depth-tracked declaration boundaries, and guarantees loop forward progress.

- **Context-Sensitive AST Syntax Highlighter & Output Formatters:**
  - Traverses the AST with `AstHighlightVisitor` rather than using regex or raw token streams, mapping exact `(line, column)` locations so function declarations (`FuncDecl`) and call targets (`CallExpr::callee`) are highlighted differently from variable and parameter identifiers—even when sharing the same identifier name.
  - Preserves comments, whitespace, and error tokens, underlining both lexical `INVALID` tokens and syntactic error locations.
  - Outputs both **ANSI Terminal** escape sequences (exact round-trip source preservation) and **self-contained HTML5/CSS** (with `<pre class="bminor-code">`, embedded `<style>`, full HTML entity escaping, and zero JavaScript).

---

## Repository Structure

```text
bminor/
├── Makefile
├── README.md
├── docs/
│   ├── lexical_specification.md   # Token definitions, regexes, priority & recovery rules
│   ├── lexer_design.md            # Manual DFA architecture & dual token-stream design
│   ├── grammar_ebnf.md            # Formal left-recursion-free B-Minor EBNF grammar
│   ├── parser_compatibility.md    # LL(k) FIRST/FOLLOW proof & panic-mode sync points
│   └── architecture_and_design.md # End-to-end architecture, data structures & extension guide
├── include/
│   ├── common/                    # SourceLocation & Diagnostic definitions
│   ├── lexer/                     # Token & Lexer APIs
│   ├── ast/                       # ASTNode hierarchy, ASTVisitor & dumpAst()
│   ├── parser/                    # LL(k) Recursive-Descent Parser API
│   ├── highlighter/               # SyntaxHighlighter & HighlightCategory
│   └── output/                    # ANSI & HTML/CSS formatter APIs
├── src/
│   ├── main.cpp                   # Integrated CLI driver
│   ├── lexer/                     # DFA scanner & token formatting
│   ├── ast/                       # AST visitor dispatch & tree dumper
│   ├── parser/                    # Recursive-descent rules & panic-mode recovery
│   ├── highlighter/               # ASTVisitor context classifier
│   └── output/                    # ANSI SGR & HTML5/CSS emitters
└── tests/
    ├── lexer_tests.cpp            # Unit tests for Lexer
    ├── parser_tests.cpp           # Unit tests for AST & Parser
    ├── highlighter_tests.cpp      # Unit tests for AST highlighting & formatters
    ├── integration_tests.cpp      # End-to-end pipeline & general-purpose tests
    ├── valid/
    │   ├── factorial.bminor
    │   └── language_features.bminor
    └── invalid/
        ├── lexer_errors.bminor
        ├── syntax_errors.bminor
        └── combined_errors.bminor
```

---

## Building & Running Tests

### Prerequisites
- C++17-compliant compiler (`g++`, `clang++`, or equivalent; tested with `g++ 16.1.0`)
- GNU Make (`mingw32-make` on Windows, `make` on Linux/macOS)
- Standard C++ Library only (no third-party packages)

### Build Commands

**Windows (PowerShell / MSYS2):**
```powershell
mingw32-make clean
mingw32-make all
mingw32-make test
```

**Linux / macOS:**
```bash
make clean
make all
make test
```

Compiling with `-std=c++17 -Wall -Wextra -Wpedantic -Iinclude` produces zero warnings and builds `build/bminor` plus all 4 test executables.

---

## CLI Usage & Examples

```text
Usage:
  bminor [--ansi | --highlight-ansi] <source-file>
  bminor [--html | --highlight-html] <source-file> [-o <output.html>]
  bminor --lex <source-file>
  bminor --parse <source-file>
```

| Flag | Pipeline Stage | Output Destination | Exit Code |
| :--- | :--- | :--- | :--- |
| `<file>` or `--ansi <file>` | `Lexer -> Parser/AST -> Highlighter -> ANSI` | ANSI text to `stdout`; diagnostics to `stderr` | `0` if clean, `1` on errors |
| `--html <file> [-o out.html]` | `Lexer -> Parser/AST -> Highlighter -> HTML/CSS` | HTML5 doc to `<out.html>` or `stdout`; diagnostics to `stderr` | `0` if clean, `1` on errors |
| `--lex <file>` | `Lexer` only | Space-separated formatted token stream to `stdout` | `0` if clean, `1` on lex errors |
| `--parse <file>` or `--ast <file>` | `Lexer -> Parser/AST` | Indented AST tree dump with `loc` metadata to `stdout` | `0` if clean, `1` on errors |
| `-h` / `--help` | Usage summary | Help message to `stdout` | `0` |

### 1. Lexical Token Stream (`--lex`)

Input (`tests/valid/factorial.bminor`):
```bminor
// Sample valid B-Minor program based on specification examples
factorial: function integer ( n: integer ) = {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

main: function integer () = {
    result: integer = factorial(5);
    print "5! = ", result, "\n";
    return 0;
}
```

Command:
```powershell
.\build\bminor.exe --lex tests\valid\factorial.bminor
```

Observed output (`stdout`, exit code `0`):
```text
IDENT(factorial) COLON KW(function) KW(integer) LPAREN IDENT(n) COLON KW(integer) RPAREN ASSIGN LBRACE KW(if) LPAREN IDENT(n) OP(<=) INT(1) RPAREN KW(return) INT(1) SEMI KW(return) IDENT(n) OP(*) IDENT(factorial) LPAREN IDENT(n) OP(-) INT(1) RPAREN SEMI RBRACE IDENT(main) COLON KW(function) KW(integer) LPAREN RPAREN ASSIGN LBRACE IDENT(result) COLON KW(integer) ASSIGN IDENT(factorial) LPAREN INT(5) RPAREN SEMI KW(print) STRING("5! = ") COMMA IDENT(result) COMMA STRING("\n") SEMI KW(return) INT(0) SEMI RBRACE EOF
```

### 2. Abstract Syntax Tree Dump (`--parse`)

Command:
```powershell
.\build\bminor.exe --parse tests\valid\factorial.bminor
```

Observed output (`stdout`, exit code `0`):
```text
Program(loc=2:1)
  decl[0]: FuncDecl(name='factorial', kind=definition, loc=2:1)
    returnType: AtomicType(integer, loc=2:21)
    param[0]: Param(name='n', loc=2:31)
      type: AtomicType(integer, loc=2:34)
    body: BlockStmt(loc=2:46)
      stmt[0]: IfStmt(loc=3:5)
        cond: BinaryExpr(op='<=', loc=3:9)
          left: Identifier(name='n', loc=3:9)
          right: IntLiteral(value=1, loc=3:14)
        then: ReturnStmt(loc=3:17)
          value: IntLiteral(value=1, loc=3:24)
      stmt[1]: ReturnStmt(loc=4:5)
        value: BinaryExpr(op='*', loc=4:12)
          left: Identifier(name='n', loc=4:12)
          right: CallExpr(callee='factorial', loc=4:16)
            args[0]: BinaryExpr(op='-', loc=4:26)
              left: Identifier(name='n', loc=4:26)
              right: IntLiteral(value=1, loc=4:30)
  decl[1]: FuncDecl(name='main', kind=definition, loc=7:1)
    returnType: AtomicType(integer, loc=7:16)
    body: BlockStmt(loc=7:29)
      stmt[0]: VarDecl(name='result', scope=local, loc=8:5)
        type: AtomicType(integer, loc=8:13)
        init: ExprInitializer(loc=8:23)
          expr: CallExpr(callee='factorial', loc=8:23)
            args[0]: IntLiteral(value=5, loc=8:33)
      stmt[1]: PrintStmt(loc=9:5)
        expr[0]: StringLiteral(raw="5! = ", loc=9:11)
        expr[1]: Identifier(name='result', loc=9:20)
        expr[2]: StringLiteral(raw="\n", loc=9:28)
      stmt[2]: ReturnStmt(loc=10:5)
        value: IntLiteral(value=0, loc=10:12)
```

### 3. ANSI Terminal & HTML/CSS Highlighting (`--ansi`, `--html`)

Commands:
```powershell
# Highlight to terminal with ANSI SGR sequences
.\build\bminor.exe --ansi tests\valid\factorial.bminor

# Export standalone HTML5/CSS file (open directly in any modern browser; no JavaScript)
.\build\bminor.exe --html tests\valid\factorial.bminor -o build\factorial.html
```

Observed `<pre>` markup excerpt inside `build\factorial.html`:
```html
    <pre class="bminor-code"><code><span class="comment">// Sample valid B-Minor program based on specification examples</span>
<span class="fn">factorial</span>: <span class="kw">function</span> <span class="type">integer</span> ( <span class="var">n</span>: <span class="type">integer</span> ) <span class="op">=</span> {
    <span class="kw">if</span> (<span class="var">n</span> <span class="op">&lt;=</span> <span class="lit">1</span>) <span class="kw">return</span> <span class="lit">1</span>;
    <span class="kw">return</span> <span class="var">n</span> <span class="op">*</span> <span class="fn">factorial</span>(<span class="var">n</span> <span class="op">-</span> <span class="lit">1</span>);
}
...
</code></pre>
```

### 4. Error Recovery & Diagnostics on Invalid Inputs

Command:
```powershell
.\build\bminor.exe --ansi tests\invalid\combined_errors.bminor
```

Observed diagnostics (`stderr`) while `stdout` renders the complete recovered ANSI source without crashing (exit code `1`):
```text
Lexical error at tests\invalid\combined_errors.bminor:5:19: Unexpected character '@'
Lexical error at tests\invalid\combined_errors.bminor:6:17: Unterminated string literal
Lexical error at tests\invalid\combined_errors.bminor:18:1: Unterminated block comment
Syntax error at tests\invalid\combined_errors.bminor:2:10: expected ':', got '100'
Syntax error at tests\invalid\combined_errors.bminor:7:6: expected ';', got ':'
Syntax error at tests\invalid\combined_errors.bminor:8:15: missing ')' before '{'
Syntax error at tests\invalid\combined_errors.bminor:11:16: expected expression, got ';'
```

---

## Syntax Highlighting Color Mapping

All **10 categories** from the specification are implemented consistently in [src/highlighter/highlighter.cpp](src/highlighter/highlighter.cpp) and [src/output/formatter.cpp](src/output/formatter.cpp):

| AST / Token Category | Visual Style | ANSI Sequence | HTML Class |
| :--- | :--- | :--- | :--- |
| Keywords (`if`, `return`, `function`, `array`, ...) | Bold blue | `\x1b[1;34m` | `.kw` |
| Type names (`integer`, `boolean`, `char`, `string`, `void`) | Teal / cyan | `\x1b[36m` | `.type` |
| Variable & parameter identifiers | White / default | Default terminal color | `.var` |
| Function names (declarations & calls) | Yellow / gold | `\x1b[93m` | `.fn` |
| Integer literals | Orange | `\x1b[38;5;208m` | `.lit` |
| Boolean literals (`true`, `false`) | Orange | `\x1b[38;5;208m` | `.lit` |
| String and character literals | Warm green | `\x1b[32m` | `.str` |
| Operators | Light gray | `\x1b[37m` | `.op` |
| Comments (`//` and `/* ... */`) | Dim gray, italic | `\x1b[2;3;90m` | `.comment` |
| Errors & `INVALID` tokens | Red underline | `\x1b[4;91m` | `.err` |

---

## Test Suite & Verification Guide

Running `mingw32-make test` builds and executes all four test executables sequentially:

```text
build/lexer_tests.exe
All Lexer unit tests passed successfully.
build/parser_tests.exe
All Parser & AST unit tests passed successfully.
build/highlighter_tests.exe
All Syntax Highlighter & Output Formatter unit tests passed successfully.
build/integration_tests.exe
All end-to-end integration tests passed successfully.
```

You can also build and invoke individual test suites or inspect specific fixtures directly:

| Test Executable / Fixture | Run Command | What It Verifies |
| :--- | :--- | :--- |
| `lexer_tests` | `.\build\lexer_tests.exe` | All 11 token categories, keyword-before-identifier priority, Maximal Munch (`x+++y`), escape sequences, 1-based line/column tracking, 256-char identifier/string bounds, and recovery from invalid characters/unterminated literals/unterminated block comments |
| `parser_tests` | `.\build\parser_tests.exe` | AST `SourceLocation` accuracy (`3:12`, `3:16`, `3:30`), unpopulated `typeAnnotation`, function prototypes & multi-dimensional arrays, dangling-`else` binding, right-associative `=` and `^`, chained postfix operators, panic-mode recovery (`expected expression, got ';'`, `missing ')' before '{'`), and degenerate/EOF inputs |
| `highlighter_tests` | `.\build\highlighter_tests.exe` | AST distinction between `FuncIdentifier` and `VarIdentifier` on identical names (`calc`), all 10 color categories, exact ANSI round-trip source preservation, HTML entity escaping (`&`, `<`, `>`, `"`, `'`), zero JavaScript, and red-underlined lexical/syntactic errors |
| `integration_tests` | `.\build\integration_tests.exe` | Full `Lexer -> Parser -> SyntaxHighlighter -> ANSI/HTML` runs across all 5 `.bminor` fixtures, recovery of trailing valid function `safe_after` in `combined_errors.bminor`, and dynamically generated multi-function programs proving general non-hardcoded behavior |
| Valid fixtures (`--parse` / `--html`) | `.\build\bminor.exe --parse tests\valid\language_features.bminor` | Prototypes (`void`), multi-dimensional arrays, nested initializers, `for`/`if-else`/`print`/`return`, and exponentiation/logical operators |
| Invalid syntax recovery | `.\build\bminor.exe --parse tests\invalid\syntax_errors.bminor` | Multi-error panic-mode recovery across broken top-level declarations, missing expressions, missing `)` before `{`, and bad operators while preserving subsequent functions (`after_recovery`) |
| Invalid lexical recovery | `.\build\bminor.exe --lex tests\invalid\lexer_errors.bminor` | Continuation past `@`, unterminated string, and unterminated block comment without aborting |

---

## Documentation Index

- [Lexical Specification](docs/lexical_specification.md) — Formal regular expressions, reserved keywords, operator groups, and disambiguation rules
- [Lexer Design](docs/lexer_design.md) — Manual DFA design rationale and lossless dual token streams
- [EBNF Grammar](docs/grammar_ebnf.md) — Complete B-Minor context-free grammar and syntax/semantics boundary
- [Parser Compatibility](docs/parser_compatibility.md) — Left-recursion elimination, `FIRST` sets, `LL(2)` check, and synchronization rules
- [Architecture & Design](docs/architecture_and_design.md) — End-to-end pipeline, data-structure decisions, color mapping, and extension points
