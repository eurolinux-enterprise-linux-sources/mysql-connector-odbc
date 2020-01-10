Summary: ODBC driver for MySQL
Name: mysql-connector-odbc
Version: 5.2.5
Release: 7%{?dist}
Group: System Environment/Libraries
URL: http://dev.mysql.com/downloads/connector/odbc/
# exceptions allow library to be linked with most open source SW,
# not only GPL code.
License: GPLv2 with exceptions

# Upstream has a mirror redirector for downloads, so the URL is hard to
# represent statically.  You can get the tarball by following a link from
# http://dev.mysql.com/downloads/connector/odbc/
Source0: %{name}-%{version}-src.tar.gz

# mysql-connector-odbc expects to be able to get at a bunch of utility
# functions inside libmysqlclient.  Now that we are restricting what symbols
# are exported by libmysqlclient, that doesn't work.  Instead, copy those
# functions here.  mysql and mysql-connector-odbc are published under the same
# license, so this creates no legal issues.  (Note: we don't worry about
# polluting application namespace in mysql-connector-odbc, since it's never
# directly statically linked with any application code.)
Source1: mysql-mysys.tar.gz

Patch1: myodbc-add-mysys.patch
Patch2: myodbc-shutdown.patch
Patch3: myodbc-64bit.patch
Patch4: myodbc-maria.patch
Patch5: myodbc-movecmpfunc.patch
Patch6: myodbc-libname.patch
Patch7: myodbc-libdir.patch
Patch8: mysql-connector-odbc-buffsize.patch
Patch9: mysql-connector-odbc-cleartext.patch
Patch10: mysql-connector-odbc-cond.patch
Patch11: mysql-connector-odbc-5.2.5-aarch64.patch
Patch12: 0001-Memory-leak-in-SQLPrepare-with-queries-that-use-para.patch

BuildRequires: mysql-devel unixODBC-devel
BuildRequires: cmake
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

%description
An ODBC (rev 3) driver for MySQL, for use with unixODBC.

%prep
%setup -q -n %{name}-%{version}-src

tar xfz %{SOURCE1}

%patch1 -p1
%patch2 -p1
%patch3 -p1
%patch4 -p1
%patch5 -p1
%patch6 -p1
%patch7 -p1
%patch8 -p1
%patch9 -p1
%patch10 -p1
%patch11 -p1
%patch12 -p1

%build
# mysql 5.5.10 has stopped #define'ing THREAD in its exports, and has
# started #define'ing MY_PTHREAD_FASTMUTEX, and neither of those changes
# sit well with mysql-connector-odbc 5.1.10.  Revisit need for these hacks
# when updating to newer release.
export CFLAGS="%{optflags} -fno-strict-aliasing -DTHREAD=1 -UMY_PTHREAD_FASTMUTEX -DDBUG_OFF"

cmake . -G "Unix Makefiles" \
        -DWITH_UNIXODBC=1 \
        -DRPM_BUILD=1 \
        -DCMAKE_INSTALL_PREFIX="%{_prefix}"

make %{?_smp_mflags} VERBOSE=1

%install
export tagname=CC
make DESTDIR=$RPM_BUILD_ROOT install

# Remove stuff not to be packaged (possibly reconsider later)
rm -f $RPM_BUILD_ROOT%{_bindir}/myodbc-installer

rm -f $RPM_BUILD_ROOT/usr/COPYING
rm -f $RPM_BUILD_ROOT/usr/README
rm -f $RPM_BUILD_ROOT/usr/README.debug
rm -f $RPM_BUILD_ROOT/usr/ChangeLog
rm -f $RPM_BUILD_ROOT/usr/INSTALL
rm -f $RPM_BUILD_ROOT/usr/Licenses_for_Third-Party_Components.txt

# Create a symlink for library to offer name that users are used to
ln -sf libmyodbc5w.so $RPM_BUILD_ROOT%{_libdir}/libmyodbc5.so

# Upstream provides a test suite with functional and regression tests.
# However, some tests fail, so it would deserve some more investigation.
# We don't include the test suite until it works fine.
rm -rf $RPM_BUILD_ROOT/usr/test

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%doc README README.debug ChangeLog COPYING
%{_libdir}/lib*so

%changelog
* Fri Oct 13 2017 Augusto Caringi <acaringi@redhat.com> - 5.2.5-7
- Add patch to fix memory leak in SQLPrepare.
  Resolves: #1425160

* Fri Jan 24 2014 Daniel Mach <dmach@redhat.com> - 5.2.5-6
- Mass rebuild 2014-01-24

* Wed Jan 15 2014 Honza Horak <hhorak@redhat.com> - 5.2.5-5
- Rebuild for mariadb-libs
  Related: #1045013

* Wed Jan  8 2014 Marcin Juszkiewicz <mjuszkiewicz redhat com> - 5.2.5-4
- Build failed because whether to use lib64 or not is done by checking
  list of known 64-bit architectures. So added AArch64 to that list.
  Resolves: #1041348

* Fri Dec 27 2013 Daniel Mach <dmach@redhat.com> - 5.2.5-3
- Mass rebuild 2013-12-27

* Mon Jun 17 2013 Honza Horak <hhorak@redhat.com> - 5.2.5-2
- Avoid potential segfault
  Resolves: #974794

* Fri May 24 2013 Honza Horak <hhorak@redhat.com> - 5.2.5-1
- Update to 5.2.5
- Enlarge buffer size for query string when getting info about tables
  Related: #948619

* Wed Apr  3 2013 Honza Horak <hhorak@redhat.com> - 5.2.4-2
- Fix libdir in cmake for ppc64

* Tue Mar  5 2013 Honza Horak <hhorak@redhat.com> - 5.2.4-1
- Update to 5.2.4

* Fri Mar  1 2013 Honza Horak <hhorak@redhat.com> - 5.1.11-3
- Fix data types for mariadb

* Thu Feb 14 2013 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 5.1.11-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_19_Mass_Rebuild

* Sun Jul 22 2012 Tom Lane <tgl@redhat.com> 5.1.11-1
- Update to 5.1.11

* Fri Jul 20 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 5.1.10-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_18_Mass_Rebuild

* Sun Feb  5 2012 Tom Lane <tgl@redhat.com> 5.1.10-1
- Update to 5.1.10

* Tue Jan 10 2012 Tom Lane <tgl@redhat.com> 5.1.9-1
- Update to 5.1.9
- Add --with-unixODBC-libs to configure command for safer multilib behavior
Related: #757088

* Wed Mar 23 2011 Tom Lane <tgl@redhat.com> 5.1.8-3
- Rebuild for libmysqlclient 5.5.10 soname version bump

* Tue Feb 08 2011 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 5.1.8-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_15_Mass_Rebuild

* Thu Dec 23 2010 Tom Lane <tgl@redhat.com> 5.1.8-1
- Update to 5.1.8
- Deal with mysql packaging changes that prevent us from using mysys
  utility functions directly

* Wed Jan 20 2010 Tom Lane <tgl@redhat.com> 5.1.5r1144-7
- Correct Source: tag and comment to reflect how to get the tarball

* Fri Aug 21 2009 Tomas Mraz <tmraz@redhat.com> - 5.1.5r1144-6
- rebuilt with new openssl

* Sat Jul 25 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 5.1.5r1144-5
- Rebuilt for https://fedoraproject.org/wiki/Fedora_12_Mass_Rebuild

* Wed Feb 25 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 5.1.5r1144-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_11_Mass_Rebuild

* Fri Feb 20 2009 Tom Lane <tgl@redhat.com> 5.1.5r1144-3
- Fix some 64-bitness issues with unixODBC 2.2.14.

* Fri Feb 20 2009 Tom Lane <tgl@redhat.com> 5.1.5r1144-2
- Rebuild for unixODBC 2.2.14.
- Fix problem with null username/password specifications

* Thu Jan 22 2009 Tom Lane <tgl@redhat.com> 5.1.5r1144-1
- Update to mysql-connector-odbc 5.1.5r1144, to go with MySQL 5.1.x.
  Note the library name has changed from libmyodbc3 to libmyodbc5.

* Tue Aug  5 2008 Tom Lane <tgl@redhat.com> 3.51.26r1127-1
- Update to mysql-connector-odbc 3.51.26r1127

* Tue Mar 25 2008 Tom Lane <tgl@redhat.com> 3.51.24r1071-1
- Update to mysql-connector-odbc 3.51.24r1071

* Tue Feb 12 2008 Tom Lane <tgl@redhat.com> 3.51.23r998-1
- Update to mysql-connector-odbc 3.51.23r998

* Wed Dec  5 2007 Tom Lane <tgl@redhat.com> 3.51.14r248-3
- Rebuild for new openssl

* Thu Aug  2 2007 Tom Lane <tgl@redhat.com> 3.51.14r248-2
- Update License tag to match code.

* Fri Apr 20 2007 Tom Lane <tgl@redhat.com> 3.51.14r248-1
- Update to mysql-connector-odbc 3.51.14r248
Resolves: #236473
- Fix build problem on multilib machines

* Mon Jul 17 2006 Tom Lane <tgl@redhat.com> 3.51.12-2.2
- rebuild

* Mon Mar 27 2006 Tom Lane <tgl@redhat.com> 3.51.12-2
- Remove DLL-unload cleanup call from connection shutdown (bz#185343)

* Fri Feb 10 2006 Jesse Keating <jkeating@redhat.com> - 3.51.12-1.2.1
- bump again for double-long bug on ppc(64)

* Tue Feb 07 2006 Jesse Keating <jkeating@redhat.com> - 3.51.12-1.2
- rebuilt for new gcc4.1 snapshot and glibc changes

* Fri Dec 09 2005 Jesse Keating <jkeating@redhat.com>
- rebuilt

* Fri Nov 11 2005 Tom Lane <tgl@redhat.com> 3.51.12-1
- New package replacing MyODBC.
