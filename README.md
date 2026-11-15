# B-Minor Compiler Front-End & IDE Analysis Tools

پروژه درس اصول طراحی کامپایلر (دانشگاه صنعتی امیرکبیر — پاییز ۱۴۰۵)  
پیاده‌سازی‌شده منحصراً با زبان استاندارد **C++17** و کتابخانه‌های استاندارد بدون هیچ وابستگی بیرونی.

## ویژگی‌های فاز اول

- **تحلیل‌گر لغوی (`Manual DFA Lexer`):** پیاده‌سازی دستی DFA با پشتیبانی از تمام ۱۱ دسته توکن B-Minor، قوانین `Maximal Munch` و `Keyword-before-Identifier`، ردیابی موقعیت خط و ستون مبنای ۱ (`SourceLocation`)، نگهداری هم‌زمان `allTokens` و `parserTokens` و بازیابی خطای لغوی (کاراکترهای نامعتبر، رشته‌ها و کامنت‌های بلوکی بسته‌نشده).
- **درخت نحو انتزاعی (`AST`) و تحلیل‌گر نحوی (`Recursive-Descent Parser`):** پیاده‌سازی منطبق با گرامر EBNF زبان B-Minor (`LL(1)` و یک نقطه `LL(2)` برای تشخیص `IDENT ':'`)، فیلد اجباری `SourceLocation` و `typeAnnotation` (رزروشده برای فاز معنایی) روی تمام گره‌ها، به همراه بازیابی خطای `Panic-Mode Recovery`.
- **رنگ‌آمیز نحوی در سطح AST و قالب‌های خروجی:** تفکیک زمینه‌ای نام توابع در اعلان/فراخوانی از شناسه‌های متغیر بر پایه پیمایش `ASTVisitor`، پوشش کامل ۱۰ دسته جدول رنگ‌آمیزی، و تولید خروجی در هر دو قالب `ANSI Terminal` و `HTML/CSS` مستقل (بدون JavaScript).

## ساختار ماژولار پروژه

- `include/` و `src/`: هدرها و پیاده‌سازی ماژول‌های مستقل (`common`, `lexer`, `ast`, `parser`, `highlighter`, `output`) و نقطه ورود `src/main.cpp`.
- `docs/`:
  - [lexical_specification.md](docs/lexical_specification.md) — تعریف رسمی توکن‌ها، عبارات منظم و اولویت‌ها
  - [grammar_ebnf.md](docs/grammar_ebnf.md) — گرامر رسمی EBNF زبان B-Minor بدون بازگشت چپ
  - [parser_compatibility.md](docs/parser_compatibility.md) — اثبات سازگاری گرامر با `LL(k)` Recursive-Descent و نقاط همگام‌سازی
  - [lexer_design.md](docs/lexer_design.md) — مستند طراحی Manual DFA و جریان توکن‌ها
  - [architecture_and_design.md](docs/architecture_and_design.md) — معماری کلان، الگوریتم‌ها و تصمیمات طراحی
- `tests/`: تست‌های واحد و یکپارچه (`lexer_tests.cpp`, `parser_tests.cpp`, `highlighter_tests.cpp`, `integration_tests.cpp`) به همراه فایل‌های نمونه در `tests/valid/` و `tests/invalid/`.

## کامپایل، تست و اجرای برنامه (CLI)

### ساخت و اجرای تست‌ها

در محیط Windows (با `mingw32-make` و `g++`):

```powershell
mingw32-make clean
mingw32-make all
mingw32-make test
```

در محیط‌های Unix-like (با GNU `make` و `g++`):

```bash
make clean
make all
make test
```

### اجرای خط لوله روی فایل B-Minor

```powershell
# رنگ‌آمیزی در ترمینال ANSI (حالت پیش‌فرض)
./build/bminor tests/valid/factorial.bminor
./build/bminor --ansi tests/valid/factorial.bminor

# تولید فایل HTML/CSS مستقل (بدون JavaScript)
./build/bminor --html tests/valid/factorial.bminor -o build/factorial.html

# نمایش جریان توکن‌ها (Lexer)
./build/bminor --lex tests/valid/factorial.bminor

# نمایش درخت نحو انتزاعی (Parser/AST)
./build/bminor --parse tests/valid/factorial.bminor
```
