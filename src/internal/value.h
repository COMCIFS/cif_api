/*
 * value.h
 *
 * Copyright 2014, 2015 John C. Bollinger
 *
 *
 * This file is part of the CIF API.
 *
 * The CIF API is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * The CIF API is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the CIF API.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * This is an internal CIF API header file, intended for use only in building the CIF API library.  It is not needed
 * to build client applications, and it is intended to *not* be installed on target systems.
 */

#ifndef INTERNAL_VALUE_H
#define INTERNAL_VALUE_H

/* Unicode code points for select characters: */

/* horizontal tab */
#define UCHAR_TAB      0x09

/* newline */
#define UCHAR_NL       0x0a

/* vertical tab */
#define UCHAR_VT       0x0b

/* form feed */
#define UCHAR_FF       0x0c

/* carriage return */
#define UCHAR_CR       0x0d

/* space */
#define UCHAR_SP       0x20

/* double quote */
#define UCHAR_DQ       0x22

/* the hash symbol */
#define UCHAR_HASH     0x23

/* the dollar sign */
#define UCHAR_DOLLAR   0x24

/* single quote */
#define UCHAR_SQ       0x27

/* opening parenthesis */
#define UCHAR_OPEN     0x28

/* closing parenthesis */
#define UCHAR_CLOSE    0x29

/* plus sign */
#define UCHAR_PLUS     0x2b

/* hyphen / minus sign */
#define UCHAR_MINUS    0x2d

/* decimal point */
#define UCHAR_DECIMAL  0x2e

/* digit 0 */
#define UCHAR_0        0x30

/* digit 9 */
#define UCHAR_9        0x39

/* colon */
#define UCHAR_COLON    0x3a

/* semicolon */
#define UCHAR_SEMI     0x3b

/* query (question mark) */
#define UCHAR_QUERY    0x3f

/* uppercase Latin E */
#define UCHAR_E        0x45

/* opening square bracket */
#define UCHAR_OBRK     0x5b

/* backslash */
#define UCHAR_BSL      0x5c

/* closing square bracket */
#define UCHAR_CBRK     0x5d

/* the underscore */
#define UCHAR_UNDER    0x5f

/* lowercase Latin e */
#define UCHAR_e        0x65

/* opening curly brace */
#define UCHAR_OBRC     0x7b

/* closing curly brace */
#define UCHAR_CBRC     0x7d

/* byte-order mark */
#define UCHAR_BOM      0xfeff

#endif

