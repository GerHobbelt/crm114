Summary: CRM114 Bayesian Spam Detector
Name: crm114
Version: 20031215RC12
Release: 1
URL: http://crm114.sourceforge.net/
License: GPL
Group: Applications/CPAN
Source0: http://crm114.sourceforge.net/%{name}-%{version}.src.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-root
BuildPreReq: tre-devel

%description

CRM114 is a system to examine incoming e-mail, system log streams,
data files or other data streams, and to sort, filter, or alter the
incoming files or data streams according to the user's wildest
desires. Criteria for categorization of data can be by satisfaction of
regexes, by sparse binary polynomial matching with a Bayesian Chain
Rule evaluator, or by other means.

%prep

%setup -q -n %{name}-%{version}.src

%build
make INSTALL_DIR=$RPM_BUILD_ROOT%{_bindir}

%clean
rm -rf $RPM_BUILD_ROOT

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT%{_bindir}
make BINDIR=${RPM_BUILD_ROOT}%{_bindir} install

[ -x /usr/lib/rpm/brp-compress ] && /usr/lib/rpm/brp-compress

%files
%defattr(-,root,root)
%{_bindir}/*
%doc *.txt *.recipe

%changelog
* Mon Dec 15 2003 Bill Yerazunis <wsy@merl.com>
- removed -RCx stuff, now version contains it.
- updated for version 20031215-RC12
- License is GPL, not Artistic, so I corrected that.

* Sat Dec 13 2003 Kevin Fenzi <kevin-crm114@tummy.com>
- Converted line endings from dos format to unix.
- Changed BuildPreReq to be 'tre-devel'
- Fixed install to install into rpm build root.
- tested on redhat 9 with latest tre.

* Tue Oct 22 2003 Nico Kadel-Garcia <nkadel@merl.com>
- Created RedHat compatible .spec file
- Added libtre dependency to avoid building second package
- Hard-coded "INSTALL_DIR" in build/install setups
