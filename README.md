`spreadsheet` is the 1st generation of my personal solution to pretty-print my
monthly finances. Has been superseded by [`tabulate`](https://github.com/levirak/tabulate).

# Motivation

When I started keeping track of my income and expenses, I had a problem: it
took 30 whole second to start LibreOffice Calc. My flow was to open the
spreadsheet, add a single entry, and close the document. With such a simple
flow, I wasn't spending much longer in the program than I was waiting for load
in the first place.

# Solution

I'm a terminal guy. I love my plaintext, and I love my plaintext editor. As a
result, I'm no stranger to the ugly-editor and pretty-printer model, and so I
decided to embrace that. I migrated my spreadsheets over to tab-separated
values files and wrote this program to pretty-print those files to the
terminal. As it did so it would read cells starting with `=` as expressions to
evaluate.

# Language Choice

C is my favorite language because it doesn't do anything for me. I enjoy
working with it because it necessitates me to engage with systems that are
usually abstracted away in other languages, and in turn I find that this helps
me appreciate those other languages more. For similar reasons I have chosen for
my build system to be a hand written makefile.

I usually write for the most recent version of GNU C because I usually have a
newer `gcc` installed. On top of that, I write in a personal dialect of C that
I don't let get too alien. As a taste of that dialect, I use `s32` in place of
`int` and `u64` in place of `unsigned long`, and I have added the keyword
`NotImplemented` as a debug trap and place holder. All additions I write with
are placed in [`main.h`](src/main.h), which I include in every source file.
