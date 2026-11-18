# B-Minor Lexical Specification

This document defines the formal lexical rules, regular expressions, disambiguation priorities, and error-recovery behavior for the **B-Minor** programming language, aligned with the Phase 1 specification and course reference (`Compiler-CH0B.pdf`).

---

## 1. Token Structure

Implemented in [include/lexer/token.hpp](../include/lexer/token.hpp) and [include/common/source_location.hpp](../include/common/source_location.hpp), every emitted `Token` carries:

| Field | Type | Description |
| :--- | :--- | :--- |
| `type` | `TokenType` | Lexical category, `INVALID` (recovered error), or `END_OF_FILE` |
| `lexeme` | `std::string` | Exact matched substring from the source input |
| `location` | `SourceLocation` | 1-based `filename`, `line`, and `column` of the token's first character |
| `startOffset` | `std::size_t` | Zero-based byte offset in the input buffer |
| `length` | `std::size_t` | Byte length of the matched lexeme |

> **Dual-Stream Note:** Whitespace (`WHITESPACE`) and comments (`COMMENT_LINE`, `COMMENT_BLOCK`) are excluded from `LexResult::parserTokens`, while `LexResult::allTokens` retains every token verbatim so the AST-level Syntax Highlighter can reconstruct the original source layout and highlight comments and invalid tokens.

---

## 2. Token Categories & Formal Regular Expressions (11 Categories)

### 2.1. Keywords
B-Minor reserves **15 keywords** that cannot be used as identifiers:

- **Structural & Statement Keywords:** `array`, `else`, `for`, `function`, `if`, `print`, `return`, `void`, `while`
- **Type Keywords:** `boolean`, `char`, `integer`, `string`
- **Boolean Literal Keywords:** `false`, `true` (classified as `BOOL_LITERAL`)

> **Note on `while`:** Although `while` statements are absent from B-Minor's statement grammar, `while` is reserved as a keyword (`KW_WHILE`) at the lexical level.

```regex
array|boolean|char|else|false|for|function|if|integer|print|return|string|true|void|while
```

### 2.2. Identifiers (`IDENT`)
- Must begin with an ASCII letter (`a-z`, `A-Z`) or underscore (`_`), followed by letters, digits (`0-9`), or underscores.
- **Length Limit:** Identifiers may not exceed **256 characters** (longer identifiers trigger a lexical diagnostic without stopping scanning).

```regex
[a-zA-Z_][a-zA-Z0-9_]*
```

Examples: `i`, `x`, `myVar`, `_count`, `x1`, `factorial`.

### 2.3. Integer Literals (`INT_LITERAL`)
- Unsigned decimal integer digit sequences at the lexical stage (formatted as `INT(<lexeme>)`):

```regex
[0-9]+
```

- **Unary Negation:** Leading `-` is scanned as the operator `OP_MINUS` (`OP(-)`). Expressions such as `n - 1` or `x-1` therefore tokenize cleanly without misclassifying `-1` as a signed integer token.

Examples: `0`, `42`, `123`.

### 2.4. Boolean Literals (`BOOL_LITERAL`)
```regex
true|false
```
Matched prior to identifiers (`BOOL(true)`, `BOOL(false)`).

### 2.5. String Literals (`STRING_LITERAL`)
- Enclosed in double quotes (`"..."`) with backslash escape support (`\n`, `\0`, `\"`, `\\`, and `\<c>`).
- **Length Limit:** String content may not exceed **256 characters**.
- Cannot cross raw unescaped newlines (`\r`, `\n`) or reach `EOF` unclosed.

```regex
"([^"\\\r\n]|\\[^\r\n])*"
```

Example: `"hello\n"`.

### 2.6. Character Literals (`CHAR_LITERAL`)
- Enclosed in single quotes (`'...'`) representing a single ASCII character or escape sequence:

```regex
'([^'\\\r\n]|\\[^\r\n])'
```

Examples: `'a'`, `'\n'`, `'\0'`, `'\''`.

### 2.7. Operators
Complete B-Minor operator set organized by grammatical role:

| Group | Tokens | Description |
| :--- | :--- | :--- |
| Postfix | `++`, `--` | Postfix increment / decrement |
| Unary | `!`, `-` | Logical negation / arithmetic negation |
| Exponentiation | `^` | Power operator |
| Multiplicative / Additive | `*`, `/`, `%`, `+`, `-` | Arithmetic binary operators |
| Relational / Equality | `<`, `<=`, `>`, `>=`, `==`, `!=` | Ordering and equality comparisons |
| Logical Binary | `&&`, `\|\|` | Logical AND / OR |
| Assignment | `=` | Assignment (`ASSIGN`) |

```regex
\+\+|--|<=|>=|==|!=|&&|\|\||[+\-*/%^!<>=]
```

### 2.8. Delimiters
```regex
[(){}\[\]:;,]
```
Tokens: `(` (`LPAREN`), `)` (`RPAREN`), `{` (`LBRACE`), `}` (`RBRACE`), `[` (`LBRACKET`), `]` (`RBRACKET`), `:` (`COLON`), `;` (`SEMI`), `,` (`COMMA`).

### 2.9. Single-Line Comments (`COMMENT_LINE`)
Extends from `//` to the end of the current line (excluding `\r` and `\n`):

```regex
//[^\r\n]*
```

### 2.10. Block Comments (`COMMENT_BLOCK`)
Extends from `/*` to the first closing `*/` (non-nesting):

```regex
/\*([^*]|\*+[^*/])*\*+/
```

### 2.11. Whitespace (`WHITESPACE`)
```regex
[ \t\r\n]+
```
Tracked continuously to maintain accurate 1-based `line` and `column` positions (incrementing `line` and resetting `column = 1` on `\n`).

---

## 3. Disambiguation & Priority Rules

When multiple rules could match at the current input position, the scanner deterministically enforces:

1. **Longest Match (`Maximal Munch`):**
   - `<=` is scanned as a single `OP(<=)` rather than `OP(<)` followed by `ASSIGN`.
   - `x+++y` is tokenized as `IDENT(x) OP(++) OP(+) IDENT(y)`.
   - `//` and `/*` take precedence over `OP(/)`.
   - `if1` is scanned as `IDENT(if1)` rather than `KW(if)` followed by `INT(1)`.

2. **Keyword-Before-Identifier Priority:**
   - After scanning a maximal `[a-zA-Z_][a-zA-Z0-9_]*` lexeme, it is looked up in the reserved keyword table before yielding `IDENT`:
     - `"true"` / `"false"` → `BOOL_LITERAL`
     - Any of the remaining 13 keywords → `KW_*`
     - Otherwise → `IDENT`

---

## 4. Lexical Error Recovery

The lexer never aborts or crashes on malformed input:

1. **Unrecognized Characters (e.g., `@`, `$`):** Records a `Lexical` diagnostic with exact `SourceLocation`, emits `TokenType::INVALID` with that character, and resumes scanning immediately at the next character.
2. **Unterminated String Literals:** When a raw `\n`, `\r`, or `EOF` is reached before the closing `"`, emits `Diagnostic` (`Unterminated string literal`), produces `INVALID`, and resumes cleanly on the following line/EOF.
3. **Unterminated Block Comments:** When `EOF` is reached after `/*` without closing `*/`, emits `Diagnostic` (`Unterminated block comment`) anchored at the comment start and produces `INVALID` before `END_OF_FILE`.
4. **Malformed Character Literals:** Empty (`''`), multi-character (`'ab'`), or unclosed `'...'` literals emit `INVALID` with a clear diagnostic and continue scanning.
5. **256-Character Exceeded Limits:** Identifiers or string literals exceeding 256 characters log a lexical diagnostic while consuming the full token so scanning stays synchronized.
