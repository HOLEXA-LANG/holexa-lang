" HOLEXA syntax for Neovim/Vim
" Place in ~/.config/nvim/syntax/holexa.vim

if exists("b:current_syntax")
  finish
endif

syn keyword holexaKeyword let mut fn return if else while for in loop break continue
syn keyword holexaKeyword struct impl trait import use pub match enum async await spawn const static self new
syn keyword holexaBoolean true false none
syn keyword holexaType int float String bool void byte char i8 i16 i32 i64 u8 u16 u32 u64 f32 f64 Array Map

syn region holexaString start='"' end='"' contains=holexaEscape
syn match  holexaEscape '\\.' contained
syn match  holexaNumber '\b[0-9]\+\(\.[0-9]\+\)\?\b'
syn match  holexaComment '//.*$'
syn region holexaComment start='/\*' end='\*/'
syn match  holexaFunction '\b[a-z_][a-zA-Z0-9_]*\ze\s*('

hi def link holexaKeyword   Keyword
hi def link holexaBoolean   Boolean
hi def link holexaType      Type
hi def link holexaString    String
hi def link holexaEscape    SpecialChar
hi def link holexaNumber    Number
hi def link holexaComment   Comment
hi def link holexaFunction  Function

let b:current_syntax = "holexa"
