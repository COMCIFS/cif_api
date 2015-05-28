/*
 * assert_doubles.h
 *
 * Functions for testing equality assertions about doubles
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

#ifndef ASSERT_DOUBLES_H
#define ASSERT_DOUBLES_H

#include <math.h>
#include <float.h>

static int assert_doubles_equal(double d1, double d2, unsigned fuzz_ulps);
static int doubles_equal_helper(double ref_frac, int ref_exp, double other, unsigned fuzz_ulps);

/*
 * Asserts that the two specified doubles are approximately equal.  Specifically, it asserts that the values do not
 * differ by more than 'fuzz_ulps' ULPs relative to the one with the smaller absolute value.
 */
static int assert_doubles_equal(double d1, double d2, unsigned fuzz_ulps) {
    int exp1, exp2;
    double frac1 = frexp(d1, &exp1);
    double frac2 = frexp(d2, &exp2);

    /* NOTE: if exp1 == exp2 then it makes no difference with respect to which argument the ULP defined */
    return (exp2 > exp1)
            ? (doubles_equal_helper(frac1, exp1, d2, fuzz_ulps))
            : (doubles_equal_helper(frac2, exp2, d1, fuzz_ulps));
}

/*
 * NOTE: this implementation assumes that FLT_RADIX is 2
 */

static int doubles_equal_helper(double ref_frac, int ref_exp, double other, unsigned fuzz_ulps) {
    double ref_int = ldexp(ref_frac, DBL_MANT_DIG);
    double window_min = ldexp(ref_int - fuzz_ulps, ref_exp - DBL_MANT_DIG);
    double window_max = ldexp(ref_int + fuzz_ulps, ref_exp - DBL_MANT_DIG);

    return ((window_min <= other) && (other <= window_max));
}

#endif
