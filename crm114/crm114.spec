Name:           crm114
Version:        20081111
%define	vtag BlameBarack
%define svnver Ger-4653
Release:        4%{?dist}
Summary:        CRM114 - the Controllable Regex Mutilator

Group:          Productivity/Text/Utilities
License:        GPLv2
URL:            http://crm114.sourceforge.net
Source0:        http://hebbut.net/Public.Offerings/crm114/downloads/crm114-%{version}-%{vtag}-%{svnver}.src.tar.bz2
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  tre-devel, emacs-el
Requires:       tre, emacs-el

%description
CRM114 is a system to examine incoming e-mail, system log streams, data
files or other data streams, and to sort, filter, or alter the incoming
files or data streams according to the user's wildest desires. Criteria
for categorization of data can be via a host of methods, including
regexes, approximate regexes, a Hidden Markov Model, Bayesian Chain
Rule Orthogonal Sparse Bigrams, Winnow, Correlation, KNN/Hyperspace,
Bit Entropy, CLUMP, SVM, Neural Networks ( or by other means- it's all
programmable).

%prep
%setup -n %{name}-%{version}-%{vtag}

%build
%configure
make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT


%clean
rm -rf $RPM_BUILD_ROOT

%check
make check
#make test_basics
#make test_mailfilter
#make test_mailreaver
#make megatest_ng

%files
%defattr(-,root,root,-)
%doc %{_docdir}/%{name}/*
%{_bindir}/*
%{_libexecdir}/%{name}/*
%{_prefix}/share/emacs/site-lisp/crm114-mode.el

%changelog
* Mon Feb 23 2009 Ger Hobbelt <ger@hobbelt.com> - 20081111-Ger-4631
- added 'make check' as that's the big one doing all the software tests, including a (segmented) version of the vanilla megatest[_ng]
- merged with the GerH distro and autoconf scripts
* Thu Jan 29 2009 Jimmy Tang <jtang@tchpc.tcd.ie> - 20080630-4
- updated the summary line 
- added some emacs dependancies
- turned on the megatest_ng checks
* Thu Jan 29 2009 Jimmy Tang <jtang@tchpc.tcd.ie> - 20080630-2
- depend on tre and tre-devel from rpmforge instead of the libtre package from suse
* Thu Jan 29 2009 Jimmy Tang <jtang@tchpc.tcd.ie> - 20080630-1
- Initial package from http://hebbut.net/Public.Offerings/crm114.html
