# CIF API 0.4.2 (public release)

The CIF API provides a standard C interface for reading, writing, and
manipulating Crystallographic Information Files (CIFs).  It is targeted
in particular at CIF 2.0, but it provides support for CIF 1.1 and earlier
as well.

HTML documentation is available online at http://comcifs.github.io/cif_api/

The package includes several example programs to demonstrate API usage;
see `src/examples/`.  These may optionally be built (and installed) with the
API library.  One of them is a text-mode CIF 2.0 file syntax checker, which
might be of some interest.  Also, the tests (`src/tests/`) collectively provide
a fairly comprehensive demonstration of API features, though their
primary purpose is to verify that the library works as intended.

The package also includes EXPERIMENTAL program cif_linguist, which converts
data between various versions of the CIF format.  It is not built or
installed by default on account of its experimental nature, but configure
option --with-linguist will cause it to be included for building and
installation.

Refer to file INSTALL for general installation instructions.  Build details
specific to this package follow:

 * The package depends on the International Components for Unicode (ICU), so
  its headers and development libraries are required to build the CIF API
  library and tests.  At least the ICU development libraries will be required
  to build programs that use the CIF API library.  The CIF API was developed
  and tested against ICU 4.2, and it is expected to be compatible also with
  subsequent versions and some earlier versions (at least back to v3.6, which
  was used in early development).  In particular, it has been successfully
  built and tested against version 55-1.

 * The package depends on SQLite for its CIF storage engine.  It was developed
  and tested against SQLite 3.6.20, and is expected to work with many later
  versions.  It depends on foreign key support, which was introduced in
  version 3.6.19, so versions earlier than that are not supported.

 * Configuring the project for building requires the `pkg-config` program and
  pkg-config data for ICU.

 * The reference C compiler employed for package development was GNU gcc 4.4.7.
  Considerable effort was devoted, however, to restricting implementation code
  to the intersection of standard C90 and standard C99, and additionally to
  avoiding code that has different meaning in C and C++.  With very few
  caveats, any compiler that is compliant with any one of C90, C99, C2011,
  C++98, or C++2011 should be able to build the project.

 * At present, the `--enable-extra-warnings` configuration option is effective
  only for gcc.

 * There are a few other project-specific configuration options, but
  they are adequately explained in `configure`'s help text.

The package has been built and tested successfully on Linux (CentOS 6, Debian);
this is the primary development platform), on OS X 10.8, and on Windows 7.
For OS X builds, the needed dependencies are readily available via the "Fink"
package management system and its public repositories, among other means.
For Windows, the dependencies are available via the MinGW-w64 project, and
further information is provided in file README_Windows.  Those approaches
assume you want to use the provided package build system; other approaches
may be available if you want to build the project with a different toolchain,
such as Visual Studio on Windows or XCode on OS X.  The project has few,
if any, toolchain dependencies, but only limited support is available for
such build strategies.


The principal author of the CIF API software, documentation, and build system
is John C. Bollinger, Ph.D, of Memphis, Tennessee, U.S.A..  E-mail
correspondance intended for him may be directed to
    John &lt;DOT> Bollinger &lt;AT> StJude &lt;DOT> org.


## Intellectual Property Notes

The CIF API comprises copyrighted property of several people and
organizations, as marked on the files themselves or otherwise recorded
among the CIF API files, including this one.  Where not otherwise
specified, CIF API files belong to John C. Bollinger, Ph.D. of Memphis,
Tennessee, U.S.A..  Specific exceptions include, but are not necessarily
limited to, the contents of the 'uthash' directory; those files are
subject to the copyright statement and license in the file 'LICENSE' in
that directory, and it is subject to those terms that they are distributed
to you.  Additionally, many components of the build system are
parts of, or were generated via, the GNU AutoTools suite.  These contain
code belonging to the Free Software Foundation (FSF), and each is marked
with an FSF ownership notice and statements of rights.  The FSF components
are distributed to you under the terms of version 3 of the GNU Public
License (GPL).  A copy of of GPL version 3 is provided in file 'COPYING'.

The International Union of Crystallography holds a patent on the STAR
file format, which is parent to CIF, and it is likely that the CIF API
implementation provided herein practices that patent.  No license 
to do so accompanies or is associated with this package itself, but the
IUCr extends broad and liberal permission, as described in its policy on
its CIF and STAR intellectual property: http://www.iucr.org/iucr-top/ipr.html.
The CIF API author holds no other rights to practice the patent, but to the
best of his knowledge, the CIF API implementation satisfies the IUCr's
terms for licensure.

**The CIF API is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.**

**The CIF API is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.**

**You should have received a copy of the GNU Lesser General Public License
along with the CIF API.  If not, see http://www.gnu.org/licenses/.**

