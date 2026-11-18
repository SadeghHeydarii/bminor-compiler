# B-Minor Formal EBNF Grammar

This document specifies the complete context-free grammar of the **B-Minor** language in **Extended Backus-Naur Form (EBNF)**, derived from the course reference (`Compiler-CH0B.pdf`) and structured for deterministic top-down parsing without left recursion.

---

## 1. EBNF Notation Conventions

- `LHS ::= RHS` — Rule definition (`LHS` non-terminals in lowercase `snake_case`)
- `A B` — **Sequence** (left-to-right concatenation)
- `A | B` — **Choice** (alternation)
- `A?` — **Option** (zero or one occurrence)
- `A*` — **Repetition** (zero or more occurrences)
- `( ... )` — Grouping
- `'...'` — **Terminal** symbol (exact keyword, operator, or delimiter)
- `IDENT`, `INT_LITERAL`, `BOOL_LITERAL`, `CHAR_LITERAL`, `STRING_LITERAL`, `EOF` — Lexical tokens produced by `Lexer`

---

## 2. Complete Grammar Rules (Left-Recursion-Free)

```ebnf
(* ------------------------------------------------------------ *)
(* 1. Program & Top-Level Declarations                          *)
(* ------------------------------------------------------------ *)

program          ::= declaration* EOF

declaration      ::= IDENT ':' ( function_tail | var_tail )

function_tail    ::= 'function' return_type '(' param_list? ')' ( '=' block | ';' )

var_tail         ::= type ( '=' initializer )? ';'

initializer      ::= expr
                   | '{' initializer ( ',' initializer )* '}'

(* ------------------------------------------------------------ *)
(* 2. Types & Parameters                                        *)
(* ------------------------------------------------------------ *)

return_type      ::= type
                   | 'void'

type             ::= atomic_type
                   | array_type

atomic_type      ::= 'integer'
                   | 'boolean'
                   | 'char'
                   | 'string'

array_type       ::= 'array' '[' expr? ']' type

param_list       ::= param ( ',' param )*

param            ::= IDENT ':' type

(* ------------------------------------------------------------ *)
(* 3. Blocks & Statements                                       *)
(* ------------------------------------------------------------ *)

block            ::= '{' stmt* '}'

stmt             ::= if_stmt
                   | for_stmt
                   | return_stmt
                   | print_stmt
                   | block
                   | local_var_decl
                   | expr_stmt

local_var_decl   ::= IDENT ':' var_tail

if_stmt          ::= 'if' '(' expr ')' stmt ( 'else' stmt )?

for_stmt         ::= 'for' '(' expr? ';' expr? ';' expr? ')' stmt

return_stmt      ::= 'return' expr? ';'

print_stmt       ::= 'print' expr_list? ';'

expr_stmt        ::= expr ';'

(* ------------------------------------------------------------ *)
(* 4. Expressions (Lowest to Highest Precedence)                *)
(* ------------------------------------------------------------ *)

expr_list        ::= expr ( ',' expr )*

expr             ::= assign

assign           ::= logical_or ( '=' assign )?

logical_or       ::= logical_and ( '||' logical_and )*

logical_and      ::= comparison ( '&&' comparison )*

comparison       ::= additive ( ( '<' | '<=' | '>' | '>=' | '==' | '!=' ) additive )*

additive         ::= multiplicative ( ( '+' | '-' ) multiplicative )*

multiplicative   ::= power ( ( '*' | '/' | '%' ) power )*

power            ::= unary ( '^' power )?

unary            ::= ( '-' | '!' ) unary
                   | postfix

postfix          ::= primary postfix_op*

postfix_op       ::= '[' expr ']'
                   | '(' expr_list? ')'
                   | '++'
                   | '--'

primary          ::= IDENT
                   | INT_LITERAL
                   | BOOL_LITERAL
                   | CHAR_LITERAL
                   | STRING_LITERAL
                   | '(' expr ')'
```

---

## 3. Language-Specific Notes

1. **No Nested Functions:** `function_tail` is valid only inside top-level `declaration`; `stmt` allows `local_var_decl` (atomic or array variables) only.
2. **Function Prototypes vs. Definitions:** Prototypes terminate with `;` (e.g., `puts: function void ( s: string );`), whereas definitions end with `= { ... }`.
3. **Arrays:** `array_type ::= 'array' '[' expr? ']' type` covers both sized variable arrays (`a: array [5] integer;`) and unsized array parameters (`arr: array [] integer`).
4. **Built-in `print` Statement:** `print` is a statement taking an optional comma-separated `expr_list`, not a function call.
5. **Control Flow:** B-Minor supports `if`, `if-else`, and `for`; there is no `while`, `do-while`, or `switch` statement.
6. **Comma-Separated Lists:** Using `entity ( ',' entity )*` rejects trailing or consecutive commas (`{1,2,}`, `{1,,2}`) at the syntactic level.

---

## 4. Boundary Between Syntax & Semantics

Context-sensitive checks belong to future semantic-analysis passes rather than the context-free grammar:
- Requiring a `main` function returning `integer` in a complete program.
- Restricting global variable initializers to constant expressions.
- Strict type checking, array-size presence checks in declarations vs. parameters, and function arity/argument type validation.
- Verifying that the left-hand side of `=` and the operand of `++`/`--` denote assignable lvalues (`IDENT` or `array[index]`).
- Scope resolution and declaration-before-use enforcement.
