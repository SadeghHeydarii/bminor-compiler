# تحلیل سازگاری گرامر B-Minor با Recursive-Descent Parser

این سند نشان می‌دهد گرامر مستندشده در [grammar_ebnf.md](file:///e:/OneDrive/AUT/Compiler/project/bminor/docs/grammar_ebnf.md) به‌طور کامل با راهبرد **تحلیل‌گر بازگشتی نزولی (`Recursive-Descent Parser` — خانواده `LL(k)`)** سازگار است (بخش‌های ۳ و ۴ مستند [فاز اول پروژه کامپایلر.pdf](file:///e:/OneDrive/AUT/Compiler/project/phase1/%D9%81%D8%A7%D8%B2%20%D8%A7%D9%88%D9%84%20%D9%BE%D8%B1%D9%88%DA%98%D9%87%20%DA%A9%D8%A7%D9%85%D9%BE%D8%A7%DB%8C%D9%84%D8%B1.pdf)، ص ۱۰–۱۱).

---

## ۱. انتخاب راهبرد Parsing

- **راهبرد انتخابی:** `LL(1) / LL(k)` با پیاده‌سازی دستی **Recursive-Descent Parser** (هر نماد غیرپایانه در گرامر یک متد متناظر در کلاس `Parser` دارد).
- **میزان Lookahead مورد نیاز:** گرامر در تمام بخش‌ها با **`LL(1)`** (نگاه به یک توکن پیش‌رو) پیش می‌رود، به‌جز **یک نقطه مشخص در سطح دستور (`stmt`)** که برای تفکیک اعلان متغیر محلی (`IDENT ':' ...`) از دستور عبارتی شروع‌شده با شناسه (`IDENT '=' ...` یا `IDENT '(' ...`) از **`LL(2)`** (بررسی توکن دوم برای تشخیص `':'`) استفاده می‌کند.

---

## ۲. بررسی و حذف بازگشت چپ (`Left Recursion`)

تحلیل‌گر بازگشتی نزولی در صورت وجود بازگشت چپ مستقیم یا غیرمستقیم دچار حلقه بی‌نهایت می‌شود. وضعیت تمام غیرپایانه‌های گرامر B-Minor به شرح زیر بررسی و تضمین شده است:

| غیرپایانه | وضعیت بازگشت چپ | نحوه تضمین سازگاری با Recursive-Descent |
| :--- | :---: | :--- |
| `program`, `declaration`, `block` | فاقد بازگشت چپ | استفاده از تکرار `*` (`declaration*`, `stmt*`) و شروع با پایانه یا غیرپایانه متمایز |
| `param_list`, `expr_list` | فاقد بازگشت چپ | استفاده از تکرار `( ',' ... )*` به جای `list ::= list ',' item` |
| `logical_or`, `logical_and`, `comparison`, `additive`, `multiplicative` | فاقد بازگشت چپ | تبدیل بازگشت چپ عملگرهای شرکت‌پذیر به چپ به ساختار حلقه `while` متناظر با تکرار `( op rhs )*` در EBNF (مشابه `Listing 1` ص ۱۱ مستند) |
| `assign`, `power` | فاقد بازگشت چپ | این دو عملگر شرکت‌پذیر به راست هستند (`a = b = 1` و `a^b^c`)؛ با **بازگشت راست** (`logical_or ( '=' assign )?` و `unary ( '^' power )?`) نوشته شده‌اند |
| `unary` | فاقد بازگشت چپ | با پیشوند پایانه `'-'` یا `'!'` شروع می‌شود و سپس به `unary` یا `postfix` می‌رود |
| `postfix` | فاقد بازگشت چپ | به جای `postfix ::= postfix '[' expr ']' \| ...` به‌صورت `primary postfix_op*` نوشته شده است تا در حلقه `while` پسوندهای زنجیره‌ای (مثل `f(x)[i]++`) از چپ به راست پارس شوند |

**نتیجه:** گرامر هیچ‌گونه بازگشت چپ مستقیم یا غیرمستقیم ندارد.

---

## ۳. مجموعه‌های `FIRST` و تحلیل تعارض‌های `FIRST / FOLLOW`

### ۳.۱. مجموعه‌های `FIRST` برای غیرپایانه‌های کلیدی

- `FIRST(declaration)` = `{ IDENT }`
- `FIRST(function_tail)` = `{ 'function' }`
- `FIRST(var_tail)` = `FIRST(type)` = `{ 'integer', 'boolean', 'char', 'string', 'array' }`
- `FIRST(return_type)` = `{ 'integer', 'boolean', 'char', 'string', 'array', 'void' }`
- `FIRST(if_stmt)` = `{ 'if' }`
- `FIRST(for_stmt)` = `{ 'for' }`
- `FIRST(return_stmt)` = `{ 'return' }`
- `FIRST(print_stmt)` = `{ 'print' }`
- `FIRST(block)` = `{ '{' }`
- `FIRST(local_var_decl)` = `{ IDENT }`
- `FIRST(expr_stmt)` = `FIRST(expr)` = `{ '-', '!', '(', IDENT, INT_LITERAL, BOOL_LITERAL, CHAR_LITERAL, STRING_LITERAL }`
- `FIRST(postfix_op)` = `{ '[', '(', '++', '--' }`

### ۳.۲. بررسی تک‌تک نقاط تداخل احتمالی و راه‌حل قطعی

1. **تداخل ظاهری در `declaration` (تابع در برابر متغیر سراسری):**
   - هر دو با `IDENT ':'` شروع می‌شوند.
   - با فاکتورگیری چپ در قاعده `declaration ::= IDENT ':' ( function_tail | var_tail )`:
     - `FIRST(function_tail) = { 'function' }`
     - `FIRST(var_tail) = { 'integer', 'boolean', 'char', 'string', 'array' }`
     - اشتراک این دو مجموعه تهی است (**کاملاً `LL(1)`**).

2. **تداخل `FIRST(local_var_decl)` و `FIRST(expr_stmt)` در `stmt` روی `IDENT`:**
   - هر دو با `IDENT` شروع می‌شوند (`x: integer;` در برابر `x = 5;` یا `foo(1);`).
   - **راه‌حل (`LL(2)`):** وقتی توکن فعلی `IDENT` است، `Parser` توکن بعدی (`peek(1)`) را بررسی می‌کند:
     - اگر `peek(1) == COLON` بود ← قطعاً `local_var_decl` است.
     - در غیر این صورت ← قطعاً `expr_stmt` است.
   - بدون هیچ‌گونه Backtracking و با پیچیدگی زمانی $O(1)$ حل می‌شود.

3. **تداخل در `assign` بین `lvalue` و `logical_or`:**
   - اگر `assign` به‌صورت `lvalue '=' assign | logical_or` نوشته می‌شد، هر دو شاخه با `IDENT` شروع می‌شدند و نیازمند Backtracking بود.
   - **راه‌حل در گرامر:** استفاده از `assign ::= logical_or ( '=' assign )?`.
     - ابتدا `logical_or` پارس می‌شود (`LL(1)`).
     - اگر توکن بعدی `'='` بود، با نگاه به یک توکن (`LL(1)`) شاخه `'=' assign` انتخاب شده و بررسی می‌شود که گره سمت چپ یک هدف انتساب معتبر (`Identifier` یا دسترسی اندیسی آرایه) باشد؛ در غیر این صورت خطای معنایی/نحوی مناسب گزارش می‌شود.

4. **ابهام `Dangling Else` در `if_stmt` (`LL(1)` conflict روی `'else'` در `FOLLOW(if_stmt)`):**
   - در دستور تودرتوی `if (a) if (b) s1; else s2;`، توکن `'else'` هم می‌تواند متعلق به `if` داخلی و هم `if` خارجی باشد.
   - **راه‌حل استاندارد Recursive-Descent:** تابع `parseIfStmt()` بلافاصله پس از پارس کردن بخش `then`، در صورت مشاهده `'else'` به‌صورت حریصانه آن را مصرف می‌کند؛ بنابراین هر `'else'` به‌طور قطعی به نزدیک‌ترین `'if'` داخلی متصل می‌شود (مطابق رفتار استاندارد زبان C).

5. **تداخل در `for_stmt` و `return_stmt` و `print_stmt` با بخش‌های اختیاری:**
   - در `return_stmt ::= 'return' expr? ';'`:
     - `FIRST(expr)` با `{ ';' }` هیچ اشتراکی ندارد (`LL(1)`).
   - در `print_stmt ::= 'print' expr_list? ';'`:
     - `FIRST(expr_list)` با `{ ';' }` هیچ اشتراکی ندارد (`LL(1)`).
   - در `for_stmt ::= 'for' '(' expr? ';' expr? ';' expr? ')' stmt`:
     - دو بخش اول اختیاری پیش از `';'` و بخش سوم پیش از `')'` قرار دارند؛ اشتراک `FIRST(expr)` با `{ ';' , ')' }` تهی است (`LL(1)`).
   - در `array_type ::= 'array' '[' expr? ']' type`:
     - اشتراک `FIRST(expr)` با `{ ']' }` تهی است (`LL(1)`).

6. **پایان `postfix` (`postfix_op*`) و `FOLLOW(postfix)`:**
   - `FIRST(postfix_op) = { '[', '(', '++', '--' }`
   - هیچ عملگر پیشوندی یا دوتایی بعدی با `[`, `(`, `++`, `--` شروع نمی‌شود؛ بنابراین حلقه `while` در `parsePostfix()` با ۱ توکن lookahead کاملاً قطعی است.

---

## ۴. نقاط همگام‌سازی برای بازیابی خطای `Panic-Mode Recovery` (آماده‌سازی فاز ۳)

بر اساس بخش ۵ مستند فاز اول (ص ۱۳ و مثال آن)، نقاط همگام‌سازی زیر با ساختار `Recursive-Descent Parser` هم‌تراز در نظر گرفته می‌شوند:
- **در سطح `declaration` / `program`:** توقف تا رسیدن به `;`، `}` یا شروع اعلان جدید (`IDENT` پیش از `:` در سطح بالا) و `EOF`.
- **در سطح `stmt` / `block`:** نقاط همگام‌سازی شامل `;`، `}`، `{` (برای بازیابی خطاهایی نظیر جاافتادن `)` پیش از `{` مطابق مثال ص ۱۳)، و کلمات کلیدی شروع دستور شامل `if`، `for`، `return`، `print`.
- با این سازوکار، خطاهای نحوی گزارش شده و `Parser` بدون توقف ناگهانی ادامه برنامه را تحلیل می‌کند.
