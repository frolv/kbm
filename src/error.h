/*
 * error.h
 * Copyright (C) 2016-2017 Alexei Frolov
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KBM_ERROR_H
#define KBM_ERROR_H

#include "parser.h"

/* functions to print out parser notes/warnings/errors */

void err_unterm(struct lexer *lex);
void err_generic(struct lexer *lex, const char *err);
void err_invkey(struct lexer *lex);
void err_selfmod(struct lexer *lex);
void err_eof(struct lexer *lex);
void warn_literal(struct lexer *lex, size_t lim, int quote);
void note_duplicate(struct lexer *lex);

#endif
