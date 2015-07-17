/*
 * cif_errlist.h
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

#ifndef MESSAGES_H
#define MESSAGES_H

#ifdef _WIN32
#ifdef DLL_EXPORT
#define DECLSPEC __declspec(dllexport)
#else
#ifndef LIBCIF_STATIC
#define DECLSPEC __declspec(dllimport)
#endif
#endif
#endif

#ifndef DECLSPEC
#define DECLSPEC
#endif

/**
 * @file
 *
 * @addtogroup return_codes
 * @{
 */

/**
 * @brief A table of short descriptions of the errors indicated by the various
 *        CIF API error codes
 */
DECLSPEC extern const char cif_errlist[][80];

/**
 * @brief The number of entries in @c cif_errlist
 */
DECLSPEC extern const int cif_nerr;

/**
 * @}
 */

#endif
