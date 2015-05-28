/*
 * compat.h
 *
 * Copyright (C) 2015 John C. Bollinger
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

#ifndef INTERNAL_COMPAT_H
#define INTERNAL_COMPAT_H

#ifdef __MINGW32__
#define __USE_MINGW_ANSI_STDIO 1
#endif

#endif

