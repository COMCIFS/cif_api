/*
 * value.h
 *
 * Copyright (C) 2013 John C. Bollinger
 *
 * All rights reserved.
 *
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

/* backslash */
#define UCHAR_BSL      0x5c

/* lowercase Latin e */
#define UCHAR_e        0x65

/* byte-order mark */
#define UCHAR_BOM      0xfeff

#endif

