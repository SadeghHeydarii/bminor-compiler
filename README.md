# B-Minor Compiler Front-End & IDE Analysis Tools

پروژه درس اصول طراحی کامپایلر (دانشگاه صنعتی امیرکبیر — پاییز ۱۴۰۵)  
پیاده‌سازی‌شده منحصراً با زبان **C++** و ابزارها و کتابخانه‌های استاندارد.

## ساختار ماژولار پروژه

- `include/` و `src/`: هدرها و پیاده‌سازی ماژول‌های مستقل پروژه (`common`, `lexer`, `ast`, `parser`, `highlighter`, `output`) به همراه نقطه ورود `src/main.cpp`.
- `docs/`: مستندات رسمی قواعد واژگانی، گرامر BNF/EBNF زبان B-Minor و تصمیمات طراحی.
- `tests/`: مجموعه تست‌های ورودی معتبر (`tests/valid`) و نامعتبر (`tests/invalid`).
- `build/`: خروجی‌های کامپایل (در `.gitignore` نادیده گرفته می‌شود).

## کامپایل و اجرا

در محیط Windows (با `mingw32-make` و `g++` استاندارد):

```powershell
mingw32-make
mingw32-make run
mingw32-make clean
```

در محیط‌های Unix-like (با GNU `make` و `g++`):

```bash
make
make run
make clean
```
