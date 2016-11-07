" Vim syntax file
" Language: kbm
" Maintainer: Alexei Frolov
" Latest Revision: 5 November 2016

if exists("b:current_syntax")
    finish
endif

syn keyword kbm_operation click rclick exec toggle quit
syn keyword kbm_operation jump nextgroup=kbm_number skipwhite
syn keyword kbm_operation key nextgroup=kbm_keydef skipwhite
syn keyword kbm_qualifier norepeat
syn match kbm_arrow /->\>/

syn keyword kbm_todo contained TODO XXX NOTE
syn match kbm_comment /#.*$/ contains=kbm_todo

syn match kbm_number /\<\d\+\>/ nextgroup=kbm_number skipwhite

syn match kbm_modifier /\c\v(control|ctrl)-/
syn match kbm_modifier /\c\v(shift)-/
syn match kbm_modifier /\c\v(super|command|cmd|win|windows)-/
syn match kbm_modifier /\c\v(alt|meta|option)-/
syn match kbm_modifier /[!@~^]/
" single character keys
syn match kbm_keydef /\<[a-zA-z0-9]\>/
syn match kbm_keydef /\c\v(F[1-9]|F10|F11|F12)>/
syn match kbm_keydef /\c\v(zero|one|two|three|four|five|six|seven|eight|nine)>/
syn match kbm_keydef /\c\v(backtick|grave|minus|dash|equals|leftbracket)>/
syn match kbm_keydef /\c\v(leftsq|leftsquare|rightbracket|rightsq|rightsquare)>/
syn match kbm_keydef /\c\v(backslash|semicolon|quote|apostrophe|comma|period)>/
syn match kbm_keydef /\c\v(dot|slash|space|esc|escape|backspace|tab|caps)>/
syn match kbm_keydef /\c\v(capslock|enter|return|printscreen|scrolllock|pause)>/
syn match kbm_keydef /\c\v(insert|ins|delete|del|home|end|pageup|pgup|pagedown)>/
syn match kbm_keydef /\c\v(pgdn|left|right|up|down|numlock|numdiv|numdivide)>/
syn match kbm_keydef /\c\v(numslash|nummult|nummultiply|numasterisk|numtimes)>/
syn match kbm_keydef /\c\v(numminus|numplus|numenter|numdel|numdelete|numins)>/
syn match kbm_keydef /\c\v(numinsert|numend|numdown|numpgdn|numpagedown|numleft)>/
syn match kbm_keydef /\c\v(numclear|numright|numhome|numup|numpgup|numpageup)>/
syn match kbm_keydef /\c\v(numdecimal|numdec|num[0-9])>/

syn match kbm_escaped /\\./ contained
syn region kbm_string start='"' skip=/\\./ end='"' contains=kbm_escaped

let b:current_syntax = "kbm"

hi def link kbm_operation Function
hi def link kbm_qualifier Statement
hi def link kbm_arrow Operator
hi def link kbm_todo Todo
hi def link kbm_comment Comment
hi def link kbm_escaped SpecialChar
hi def link kbm_string String
hi def link kbm_number Number
hi def link kbm_modifier Structure
hi def link kbm_keydef Structure
