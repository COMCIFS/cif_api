/*
 * compat.h
 *
 * Copyright (C) 2015 John C. Bollinger
 *
 * All rights reserved.
 *
 * This is an internal CIF API header file, intended for use only in building the CIF API library.  It is not needed
 * to build client applications, and it is intended to *not* be installed on target systems.
 */

#ifndef INTERNAL_COMPAT_H
#define INTERNAL_COMPAT_H

#ifdef __MINGW32__
#define __USE_MINGW_ANSI_STDIO 1
#endif

#endif

