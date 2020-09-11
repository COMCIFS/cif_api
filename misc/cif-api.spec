Name:           cif-api
Version:        0.4.3
Release:        1%{?dist}
Summary:        The CIF API runtime library and standard programs

Group:          System Environment / Libraries
License:        LGPLv3+
URL:            https://github.com/COMCIFS/cif_api
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  pkgconfig(icu-io)
BuildRequires:  sqlite-devel >= 3.6.19

%global _hardened_build 1


%description
The Crystallographic Information File (CIF) format is a text-based data
archiving and exchange format widely used in the field of Crystallography.
The CIF API is one of several libraries useful to software developers for 
adding CIF import and export capabilities to their programs.  It is
distinguished by being one of the first to support version 2.0 of CIF;
it supports all known earlier versions as well, and can distinguish
between them automatically.

This package contains the CIF API runtime library, and standard programs
cif2_syncheck and cif_linguist.


%package devel
Summary:        The CIF API header files and development library
Group:          Development / Libraries

Requires:       %{name}%{?_isa} = %{version}-%{release}
Requires:       pkgconfig(icu-io)
Requires:       sqlite3-devel >= 3.6.19


%description devel
This package contains the CIF API public header files, development library,
and API documentation.


%prep
%setup -q


%build

# Note: configure --without-docs because docs get installed in docdir/html,
#       which rpmbuild subsequently clobbers.  We kludge around this
#       rpmbuild misbehavior in the %%install and %%files scriptlets.
%configure \
  --with-examples \
  --with-linguist \
  --without-docs \
  --disable-maintainer-mode \
  --disable-dependency-tracking

# Suppress inclusion of an RPATH in the library
sed -i 's|^hardcode_libdir_flag_spec=.*|hardcode_libdir_flag_spec=""|g' libtool
sed -i 's|^runpath_var=LD_RUN_PATH|runpath_var=DIE_RPATH_DIE|g' libtool

make %{?_smp_mflags}


%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}

cp -pR dox-html html

# These example programs are useful only for demonstration purposes:
rm %{buildroot}%{_bindir}/cif2_addauthor
rm %{buildroot}%{_bindir}/cif2_table1
rm %{buildroot}%{_bindir}/cif2_table3

# Also not wanted in the package:
rm %{buildroot}%{_libdir}/libcif.la

%check

# LD_LIBRARY_PATH is necessary because we have suppressed RPATH from
# being embedded in the executables, including the tests.
export LD_LIBRARY_PATH=`pwd`/src/.libs

make check


%clean
rm -rf %{buildroot}


%files
%defattr(-,root,root,-)
%doc AUTHORS ChangeLog COPYING COPYING.LESSER README ReleaseNotes extras
%{_bindir}/cif2_syncheck
%{_bindir}/cif_linguist
%{_libdir}/libcif.so.*


%files devel
%defattr(-,root,root,-)
#%{_datadir}/doc/%{name}-%{version}/html
%doc html
%{_includedir}/cif.h
%{_includedir}/cif_error.h
%{_libdir}/libcif.so
%{_libdir}/pkgconfig


%post -p /sbin/ldconfig


%postun -p /sbin/ldconfig


%changelog
* Fri Sep 11 2020 John Bollinger <John.Bollinger@StJude.org> 0.4.3-1
- Updated to version 0.4.3
- Modified ICU BR and RR

* Mon Jan 11 2016 John Bollinger <John.Bollinger@StJude.org> 0.4.2-1
- Updated to version 0.4.2

* Fri Oct 30 2015 John Bollinger <John.Bollinger@StJude.org> 0.4.1-1
- Initial spec (Autoconf template)

