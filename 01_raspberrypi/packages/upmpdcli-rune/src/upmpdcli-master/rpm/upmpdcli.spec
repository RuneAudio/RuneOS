Summary:        UPnP Media Renderer front-end to MPD, the Music Player Daemon
Name:           upmpdcli
Version:        0.7.1
Release:        1%{?dist}
Group:          Applications/Multimedia
License:        GPLv2+
URL:            http://www.lesbonscomptes.com/updmpdcli
Source0:        http://www.lesbonscomptes.com/upmpdcli/downloads/upmpdcli-%{version}.tar.gz
Requires(pre):  shadow-utils
Requires(post): systemd
Requires(preun): systemd
Requires(postun): systemd
BuildRequires:  libupnp-devel
BuildRequires:  libmpdclient-devel
BuildRequires:  systemd-units
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

%description
Upmpdcli turns MPD, the Music Player Daemon into an UPnP Media Renderer,
usable with most UPnP Control Point applications, such as those which run
on Android tablets or phones.


%prep
%setup -q

%build
%configure
%{__make} %{?_smp_mflags}

%pre
getent group upmpdcli >/dev/null || groupadd -r upmpdcli
getent passwd upmpdcli >/dev/null || \
    useradd -r -g upmpdcli -d /nonexistent -s /sbin/nologin \
    -c "upmpdcli mpd UPnP front-end" upmpdcli
exit 0

%install
%{__rm} -rf %{buildroot}
%{__make} install DESTDIR=%{buildroot} STRIP=/bin/true INSTALL='install -p'
%{__rm} -f %{buildroot}%{_libdir}/libupnpp.a
%{__rm} -f %{buildroot}%{_libdir}/libupnpp.la
install -D -m644 systemd/upmpdcli.service \
        %{buildroot}%{_unitdir}/upmpdcli.service


%clean
%{__rm} -rf %{buildroot}

%files
%defattr(-, root, root, -)
%{_bindir}/%{name}
%{_libdir}/libupnpp-%{version}.so*
%{_libdir}/libupnpp.so
%{_datadir}/%{name}
%{_mandir}/man1/%{name}.1*
%{_unitdir}/upmpdcli.service
%config(noreplace) /etc/upmpdcli.conf

%post
%systemd_post upmpdcli.service

%preun
%systemd_preun upmpdcli.service

%postun
%systemd_postun_with_restart upmpdcli.service 

%changelog
* Mon Jun 09 2014 J.F. Dockes <jf@dockes.org> - 0.7.1
- Implement OpenHome services
* Sun Apr 20 2014 J.F. Dockes <jf@dockes.org> - 0.6.4
- Configuration of UPnP interface and port, MPD password.
* Wed Mar 26 2014 J.F. Dockes <jf@dockes.org> - 0.6.3
- Version 0.6.3 fixes seeking
* Sun Mar 02 2014 J.F. Dockes <jf@dockes.org> - 0.6.2
- Version 0.6.2
* Wed Feb 26 2014 J.F. Dockes <jf@dockes.org> - 0.6.1
- Version 0.6.1
* Thu Feb 13 2014 J.F. Dockes <jf@dockes.org> - 0.5
- Version 0.5
* Wed Feb 12 2014 J.F. Dockes <jf@dockes.org> - 0.4
- Version 0.4
