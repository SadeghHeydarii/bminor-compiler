# معماری، الگوریتم‌ها و تصمیمات طراحی (فاز اول B-Minor)

این سند جمع‌بندی معماری تمام ماژول‌های فاز اول، منطق الگوریتم‌های اصلی و علت انتخاب ساختارهای داده در مخزن `bminor` است.

---

## ۱. نمای کلی خط لوله (Pipeline)

اجرای برنامه از نقطه ورود `src/main.cpp` به‌صورت ماژولار و در خط لوله زیر انجام می‌شود:

```text
Source Code (.bminor)
   │
   ▼  Lexer (Manual DFA)
LexResult { allTokens, parserTokens, diagnostics(Lexical) }
   │
   ▼  Parser (LL(1)/LL(2) Recursive-Descent + Panic-Mode Recovery)
ParseResult { ProgramNode (AST), diagnostics(Syntactic) }
   │
   ▼  SyntaxHighlighter (AST Visitor + Location-Keyed Map + allTokens)
vector<HighlightedToken>
   ├──► formatAnsi(...) ──► ANSI Terminal Output
   └──► formatHtml(...) ──► Standalone HTML5 / Embedded CSS (No JS)
```

---

## ۲. ماژول‌های کلیدی و علت انتخاب ساختارهای داده

### ۲.۱. ماژول پایه (`include/common/`)
- [source_location.hpp](file:///e:/OneDrive/AUT/Compiler/project/bminor/include/common/source_location.hpp): ساختار `SourceLocation` (`filename`, `line`, `column` مبنای ۱) برای گزارش خطای دقیق، ساختار گره‌های AST و نگاشت متناظر توکن‌های منبع در رنگ‌آمیز.
- [diagnostic.hpp](file:///e:/OneDrive/AUT/Compiler/project/bminor/include/common/diagnostic.hpp): ساختار `Diagnostic` با تفکیک مرحله (`DiagnosticStage::Lexical` در برابر `DiagnosticStage::Syntactic`) جهت ثبت و چاپ پیام‌های خطا بدون توقف اجرای برنامه.

### ۲.۲. تحلیل‌گر لغوی (`include/lexer/`, `src/lexer/`)
- مستندات مرتبط: [lexical_specification.md](file:///e:/OneDrive/AUT/Compiler/project/bminor/docs/lexical_specification.md) و [lexer_design.md](file:///e:/OneDrive/AUT/Compiler/project/bminor/docs/lexer_design.md).
- **الگوریتم:** پیاده‌سازی دستی DFA (`Manual DFA`) در زمان خطی $O(n)$ با حداکثر ۱ کاراکتر lookahead.
- **طراحی جریان دوگانه در `LexResult`:**
  - `parserTokens`: توکن‌های معنایی بدون `WHITESPACE`، `COMMENT_LINE` و `COMMENT_BLOCK` به‌همراه `END_OF_FILE` جهت مصرف توسط `Parser`.
  - `allTokens`: تو sequences کامل و بدون اتلاف از تمام کاراکترهای ورودی (شامل فاصله‌ها، کامنت‌ها و توکن‌های `INVALID`) تا `SyntaxHighlighter` و خروجی‌های ANSI/HTML بتوانند کد منبع اصلی را عیناً بازنمایی کنند.
- **بازیابی خطای لغوی:** در برخورد با کاراکتر ناشناخته، توکن `INVALID` با موقعیت دقیق صادر می‌شود و اسکن بقیه فایل ادامه می‌یابد؛ همچنین رشته‌ها و کامنت‌های بلوکی بسته‌نشده بدون کرش تشخیص و گزارش می‌شوند.

### ۲.۳. درخت نحو انتزاعی (`include/ast/ast.hpp`, `src/ast/ast.cpp`)
- **سلسله‌مراتب شیءگرا با مالکیت یکتا (`std::unique_ptr`):** پایه `ASTNode` به گره‌های دسته‌بندی‌شده `DeclNode`, `StmtNode`, `ExprNode`, `TypeNode`, `InitializerNode`, `ParamNode` و ریشه `ProgramNode` تفکیک شده است تا مدیریت حافظه بدون نشت و خودکار باشد.
- **فیلدهای الزامی روی تمام گره‌ها:**
  1. `location` (`SourceLocation`): خط و ستون اولین توکن گره.
  2. `typeAnnotation` (`std::optional<std::string>`): در فاز اول `std::nullopt` باقی می‌ماند و برای مرحله تحلیل معنایی در فازهای بعدی رزرو شده است.
- **الگوی Visitor (`ASTVisitor`):** پیمایش AST از منطق نمایشگر درخت (`dumpAst`) و موتور رنگ‌آمیزی (`AstHighlightVisitor`) جدا شده است تا افزودن پیمایش‌های معنایی یا CFG در فازهای بعدی بدون تغییر کلاس‌های گره انجام شود.

### ۲.۴. تحلیل‌گر نحوی و بازیابی خطا (`include/parser/`, `src/parser/`)
- مستندات مرتبط: [grammar_ebnf.md](file:///e:/OneDrive/AUT/Compiler/project/bminor/docs/grammar_ebnf.md) و [parser_compatibility.md](file:///e:/OneDrive/AUT/Compiler/project/bminor/docs/parser_compatibility.md).
- **الگوریتم:** `Recursive-Descent Parser` کاملاً `LL(1)` به‌جز یک تصمیم `LL(2)` در `parseStmt()` برای تفکیک اعلان متغیر محلی (`IDENT ':' ...`) از دستور عبارتی (`IDENT ...`).
- **اولویت و شرکت‌پذیری عملگرها:** با استفاده از تابع‌های تودرتو برای سطوح اولویت، و الگوی مشترک `parseBinaryLeft` برای عملگرهای چپ‌شرکت‌پذیر (`||`, `&&`, مقایسه، `+`/`-`, `*`/`/`/`%`) و بازگشت راست در `parseAssign` (`=`) و `parsePower` (`^`).
- **بازیابی خطای `Panic-Mode Recovery`:**
  - استفاده از پرچم `panicMode_` برای جلوگیری از پیام‌های خطای آبشاری روی یک ساختار خراب منفرد.
  - حفظ ساختار `{ ... }` هنگام گزارش `missing ')' before '{'` بدون بلعیدن آکولاد باز بلوک.
  - همگام‌سازی در سطح دستور (`;`, `{`, `}`, `if`, `for`, `return`, `print`, `IDENT ':'`, `EOF`) و سطح اعلان با شمارش عمق آکولادها (`braceDepth`) به‌علاوه محافظ پیشروی (`startPos`) در حلقه‌ها برای تضمین قطعی عدم افتادن در حلقه بی‌نهایت.

### ۲.۵. رنگ‌آمیز نحوی در سطح AST و تولیدکننده‌های خروجی (`include/highlighter/`, `include/output/`)
- **مکانیزم AST-محور (`SyntaxHighlighter`):**
  - به جای تکیه بر Regex یا صرفاً جریان توکن‌ها، `AstHighlightVisitor` ابتدا روی AST حرکت کرده و نقش زمینه‌ای هر گره را در یک جدول درهم‌سازی با کلید ۶۴بیتی `(line, column)` (`roleByLocation`) ثبت می‌کند.
  - در نتیجه، شناسه تابع در `FuncDecl::nameLocation` و هدف فراخوانی در `CallExpr::callee` با دسته `FuncIdentifier` (زرد/طلایی) و شناسه‌های متغیر/پارامتر (`IdentifierExpr`, `VarDecl`, `ParamNode`) با `VarIdentifier` (سفید/پیش‌فرض) رنگ‌آمیزی می‌شوند؛ حتی اگر نام شناسه تابع و متغیر یکسان باشد.
  - توکن‌های `INVALID` و موقعیت خطاهای نحوی غیر-EOF با دسته `Error` (زیرخط قرمز) مشخص می‌شوند.
- **قالب‌های خروجی (`formatter.hpp`, `formatter.cpp`):**
  - `formatAnsi`: افزودن کدهای SGR استاندارد ANSI و `\x1b[0m` حول هر توکن رنگی بدون دستکاری فاصله‌ها و حفظ بازنمایی دقیق کد منبع.
  - `formatHtml`: تولید سند مستقل HTML5 با `<style>` تعبیه‌شده، بلوک `<pre class="bminor-code"><code>`، کلاس‌های `<span class="kw">`, `<span class="lit">`, `<span class="fn">` و غیره، و Escape کامل کاراکترهای `&`, `<`, `>`, `"`, `'` بدون وابستگی به JavaScript.

---

## ۳. راهنمای سریع تغییر یا توسعه در فازهای بعدی

- **افزودن یک کلمه کلیدی یا عملگر جدید:** تعریف در `TokenType` (`include/lexer/token.hpp`)، افزودن در جدول `kKeywords` یا DFA (`src/lexer/lexer.cpp`) و نگاشت در `formatToken` (`src/lexer/token.cpp`).
- **افزودن یک دستور یا عبارت نحوی جدید:** افزودن گره جدید در `include/ast/ast.hpp`، متد `visit` در `ASTVisitor`، پیاده‌سازی قاعده در `src/parser/parser.cpp` و ثبت نقش رنگی آن در `AstHighlightVisitor` (`src/highlighter/highlighter.cpp`).
- **پیاده‌سازی تحلیل معنایی (فاز ۲ پروژه):** افزودن ماژول `SymbolTable` و پرکردن فیلد آماده `ASTNode::typeAnnotation` از طریق یک پیاده‌سازی جدید از `ASTVisitor` بدون نیاز به تغییر در Lexer یا Parser.
