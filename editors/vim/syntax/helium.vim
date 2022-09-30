" Vim syntax file

if exists("b:current_syntax")
  finish
endif

let s:cpo_save = &cpo
set cpo&vim

let s:helium_syntax_keywords = {
    \   'heliumConditional' :["if"
    \ ,                    ]
    \ , 'heliumRepeat' :["while"
    \ ,               ]
    \ , 'heliumExecution' :["return"
    \ ,                  ]
    \ , 'heliumBoolean' :["true"
    \ ,                 "false"
    \ ,                ]
    \ , 'heliumKeyword' :["fn"
    \ ,                 "c_fn"
    \ ,                ]
    \ , 'heliumVarDecl' :["var"
    \ ,                 "let"
    \ ,                ]
    \ , 'heliumStorageClass' :["mut"
    \ ,                     ]
    \ , 'heliumType' :["c_string"
    \ ,              "i8"
    \ ,              "i16"
    \ ,              "i32"
    \ ,              "i64"
    \ ,              "u8"
    \ ,              "u16"
    \ ,              "u32"
    \ ,              "u64"
    \ ,              "f32"
    \ ,              "f64"
    \ ,              "bool"
    \ ,              "usize"
    \ ,              "void"
    \ ,              "c_char"
    \ ,              "c_uchar"
    \ ,              "c_short"
    \ ,              "c_ushort"
    \ ,              "c_int"
    \ ,              "c_uint"
    \ ,              "c_long"
    \ ,              "c_ulong"
    \ ,              "c_longlong"
    \ ,              "c_ulonglong"
    \ ,              "c_float"
    \ ,              "c_double"
    \ ,              "c_void"
    \ ,             ]
    \ , 'heliumStructure' :["enum"
    \ ,                     "struct"
    \ ,                     "union"
    \ ,                     "variant"
    \ ,                   ]
    \ , 'heliumVisModifier': ["pub"
    \ ,                    ]
    \ , }

function! s:syntax_keyword(dict)
  for key in keys(a:dict)
    execute 'syntax keyword' key join(a:dict[key], ' ')
  endfor
endfunction

call s:syntax_keyword(s:helium_syntax_keywords)

syntax match heliumDecNumber display   "\v<\d%(_?\d)*"
syntax match heliumHexNumber display "\v<0x\x%(_?\x)*"
syntax match heliumOctNumber display "\v<0o\o%(_?\o)*"
syntax match heliumBinNumber display "\v<0b[01]%(_?[01])*"

syntax match heliumOperator display "\V\[-+/*=^&?|!><%~:;,]"

syntax match heliumFunction /\w\+\s*(/me=e-1,he=e-1
syntax match heliumMacro /@\w\+\s*/lc=1

syntax match heliumEnumDecl /enum\s\+\w\+/lc=4
syntax match heliumStructDecl /struct\s\+\w\+/lc=6
syntax match heliumUnionDecl /union\s\+\w\+/lc=5

syntax region heliumBlock start="{" end="}" transparent fold

syntax region heliumCommentLine start="//" end="$"

function! HeliumSubexpressionSyntax(filetype,start,end,subexpressionSyntaxHighlight) abort
  let ft=toupper(a:filetype)
  let group='textGroup'.ft
  if exists('b:current_syntax')
    let s:current_syntax=b:current_syntax
    unlet b:current_syntax
  endif
  execute 'syntax include @'.group.' syntax/'.a:filetype.'.vim'
  try
    execute 'syntax include @'.group.' after/syntax/'.a:filetype.'.vim'
  catch
  endtry
  if exists('s:current_syntax')
    let b:current_syntax=s:current_syntax
  else
    unlet b:current_syntax
  endif
  execute 'syntax region subexpressionSyntax'.ft.'
  \ matchgroup='.a:subexpressionSyntaxHighlight.'
  \ keepend
  \ start="'.a:start.'" end="'.a:end.'"
  \ contains=@'.group
endfunction

call HeliumSubexpressionSyntax('c', 'inline_c', ';', 'Macro')

syntax region heliumString matchgroup=heliumStringDelimiter start=+"+ skip=+\\\\\|\\"+ end=+"+ oneline contains=heliumEscape
syntax region heliumChar matchgroup=heliumCharDelimiter start=+'+ skip=+\\\\\|\\'+ end=+'+ oneline contains=heliumEscape
syntax match heliumEscape        display contained /\\./

highlight default link heliumDecNumber heliumNumber
highlight default link heliumHexNumber heliumNumber
highlight default link heliumOctNumber heliumNumber
highlight default link heliumBinNumber heliumNumber

highlight default link heliumStructDecl heliumType
highlight default link heliumEnumDecl heliumType
highlight default link heliumUnionDecl heliumType

highlight default link heliumKeyword Keyword
highlight default link heliumType Type
highlight default link heliumCommentLine Comment
highlight default link heliumSingleLineInlineC Macro
highlight default link heliumString String
highlight default link heliumStringDelimiter String
highlight default link heliumSingleInlineCDelimiter String
highlight default link heliumChar String
highlight default link heliumCharDelimiter String
highlight default link heliumEscape Special
highlight default link heliumBoolean Boolean
highlight default link heliumNumber Number
highlight default link heliumOperator Operator
highlight default link heliumStructure Structure
highlight default link heliumExecution Special
highlight default link heliumMacro Macro
highlight default link heliumConditional Conditional
highlight default link heliumRepeat Repeat
highlight default link heliumVarDecl Type 
highlight default link heliumStorageClass StorageClass
highlight default link heliumFunction Function
highlight default link heliumVisModifier Label

delfunction s:syntax_keyword

let b:current_syntax = "helium"

let &cpo = s:cpo_save
unlet! s:cpo_save
