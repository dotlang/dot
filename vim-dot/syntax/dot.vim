" credit goes to https://robots.thoughtbot.com/writing-vim-syntax-plugins
" syntax/dot.vim
" Match TODO comments
syntax keyword dotTodos TODO XXX FIXME NOTE DOTDO

" Match language specific keywords
syntax keyword dotKeywords
      \ int
      \ char
      \ float
      \ bool


" Match all dot number types
syntax match dotNumber "\v<\d+>"
syntax match dotNumber "\v<\d+\.\d+>"
syntax match dotNumber "\v<\d*\.?\d+([Ee]-?)?\d+>"
syntax match dotNumber "\v<0x\x+([Pp]-?)?\x+>"
syntax match dotNumber "\v<0b[01]+>"
syntax match dotNumber "\v<0o\o+>"


highlight default link dotTodos Todo
highlight default link dotNumber Number
highlight default link dotKeywords Keyword

" highlight default link dotShebang Comment
" highlight default link dotComment Comment
" highlight default link dotMarker Comment
" highlight default link dotString String
" highlight default link dotInterpolatedWrapper Delimiter
" highlight default link dotBoolean Boolean
" highlight default link dotOperator Operator
" highlight default link dotAttributes PreProc
" highlight default link dotStructure Structure
" highlight default link dotType Type
" highlight default link dotImports Include
" highlight default link dotPreprocessor PreProc
