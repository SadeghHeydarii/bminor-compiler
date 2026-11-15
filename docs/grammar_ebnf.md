# گرامر رسمی زبان B-Minor به نمادگذاری EBNF

این سند گرامر مستقل از متن (CFG) کامل زبان **B-Minor** را به‌صورت رسمی با نمادگذاری **EBNF** مستندسازی می‌کند و بر اساس [فاز اول پروژه کامپایلر.pdf](file:///e:/OneDrive/AUT/Compiler/project/phase1/%D9%81%D8%A7%D8%B2%20%D8%A7%D9%88%D9%84%20%D9%BE%D8%B1%D9%88%DA%98%D9%87%20%DA%A9%D8%A7%D9%85%D9%BE%D8%A7%DB%8C%D9%84%D8%B1.pdf)، [ضمیمه 1.pdf](file:///e:/OneDrive/AUT/Compiler/project/phase1/%D8%B6%D9%85%DB%8C%D9%85%D9%87%201.pdf) و مرجع درسی [Compiler-CH0B.pdf](file:///e:/OneDrive/AUT/Compiler/slides/Compiler-CH0B.pdf) تدوین شده است.

---

## ۱. قراردادهای نمادگذاری EBNF

برای خوانایی بالا و هم‌خوانی مستقیم با ساختار `Listing 1` (ص ۱۱ مستند فاز اول) و چهار فرم کنترلی معرفی‌شده در [ضمیمه 1.pdf](file:///e:/OneDrive/AUT/Compiler/project/phase1/%D8%B6%D9%85%DB%8C%D9%85%D9%87%201.pdf) (جدول 1.1)، از قراردادهای زیر استفاده می‌شود:

- `LHS ::= RHS` : تعریف قاعده (`LHS` نام غیرپایانه با حروف کوچک و `underscore`)
- `A B` : **توالی (Sequence)** — حضور متوالی از چپ به راست
- `A | B` : **انتخاب (Choice)** — یکی از گزینه‌های جایگزین
- `A?` : **اختیاری (Option)** — صفر یا یک بار (معادل `[A]` در ضمیمه ۱)
- `A*` : **تکرار (Repetition)** — صفر یا چند بار (معادل `{A}` در ضمیمه ۱)
- `( ... )` : گروه‌بندی عبارات در سمت راست
- `'...'` : **نماد پایانه (Terminal)** — کلمه کلیدی، عملگر یا جداکننده دقیق (معادل کاراکترهای کادردار در ضمیمه ۱)
- `IDENT`, `INT_LITERAL`, `BOOL_LITERAL`, `CHAR_LITERAL`, `STRING_LITERAL`, `EOF` : توکن‌های تولیدشده توسط `Lexer`

---

## ۲. قواعد کامل گرامر B-Minor (بدون بازگشت چپ)

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
(* 4. Expressions (Ordered from Lowest to Highest Precedence)   *)
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

## ۳. توضیحات ساختارها و تطبیق با مرجع درسی (`Compiler-CH0B.pdf`)

1. **عدم امکان توابع تو در تو (اسلاید ۱۶):**
   - قاعده `function_tail` فقط در `declaration` (سطح سراسری برنامه) مجاز است و درون `stmt` تنها `local_var_decl` (اعلان متغیر محلی شامل آرایه) اجازه حضور دارد.
2. **پروتوتایپ و تعریف تابع (اسلایدهای ۲۰، ۲۲، ۲۳):**
   - پروتوتایپ تابع با `;` پایان می‌یابد (مثل `puts: function void ( s: string );`) و تعریف تابع با `= { ... }` (مثل `main: function integer () = { ... }`).
   - نوع بازگشتی تابع می‌تواند یکی از چهار نوع اتمی (`integer`, `boolean`, `char`, `string`) یا `void` باشد (اسلاید ۲۰).
3. **آرایه‌ها (اسلایدهای ۱۰ و ۲۲):**
   - در تعریف متغیر آرایه اندازه ثابت نوشته می‌شود (`a: array [5] integer;`) و در پارامتر تابع اندازه داخل براکت خالی است (`a: array [] integer`). قاعده `array_type ::= 'array' '[' expr? ']' type` هر دو حالت را پشتیبانی می‌کند و بررسی وجود/نبود اندازه در محل مناسب به مرحله تحلیل معنایی سپرده می‌شود.
4. **دستور `print` (اسلاید ۱۹):**
   - `print` یک دستور مستقل است و نه فراخوانی تابع؛ دنباله‌ای از عبارات با جداکننده کاما را دریافت می‌کند (`print "The temperature is: ", temp, " degrees\n";`).
5. **دستورهای کنترل جریان (اسلایدهای ۱۷ و ۱۸):**
   - زبان B-Minor شامل `if`, `if-else` و `for` است و دستورهای `switch`، `while` و `do-while` در آن وجود ندارند.
6. **الگوی فهرست‌های جداشده با کاما (ضمیمه ۱، بخش ۱.۶.۲):**
   - قواعد `param_list` و `expr_list` از الگوی استاندارد `entity ( ',' entity )*` استفاده می‌کنند تا فهرست‌های دارای کامای اضافه یا تکراری (مثل `{1,,2}` یا `{1,2,}`) در سطح نحوی رد شوند و اختیاری‌بودن کل فهرست در محل فراخوانی (`param_list?`, `expr_list?`) قرار گیرد.

---

## ۴. مرز میان نحو (Syntax) و معنا (Semantics)

مطابق بخش ۱.۶ [ضمیمه 1.pdf](file:///e:/OneDrive/AUT/Compiler/project/phase1/%D8%B6%D9%85%DB%8C%D9%85%D9%87%201.pdf) و اسلایدهای ۱۲ تا ۲۴ [Compiler-CH0B.pdf](file:///e:/OneDrive/AUT/Compiler/slides/Compiler-CH0B.pdf)، موارد زیر **قیود معنایی (Context-Sensitive)** هستند و نه نحوی (بنابراین توسط AST و جدول نمادها در فازهای بعدی بررسی می‌شوند):
- وجود اجباری تابع `main` با نوع بازگشتی `integer` در یک برنامه کامل (اسلاید ۲۴).
- محدودیت اینکه متغیرهای سراسری باید مقدار اولیه ثابت داشته باشند، در حالی که متغیرهای محلی می‌توانند با عبارات دلخواه مقداردهی شوند (اسلاید ۱۶).
- بررسی نوع سخت‌گیرانه (`Strict Typing` — اسلایدهای ۱۲ تا ۱۵)، عدم تطابق تعداد/نوع آرگومان‌های تابع، بررسی وجود اندازه در تعریف آرایه متغیر و خالی‌بودن اندازه آرایه در پارامتر تابع.
- اطمینان از اینکه سمت چپ انتساب `=` (`assign`) و عملوند `++`/`--` یک `lvalue` معتبر (شناسه یا دسترسی به عنصر آرایه `a[i]`) است.
- اعلان‌شدن متغیرها و توابع پیش از استفاده و قواعد حوزه (`Scope`).
