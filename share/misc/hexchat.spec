Summary:   A popular and easy to use graphical IRC (chat) client
Name:      hexchat
Version:   2.9.4
Release:   1%{?dist}
Group:     Applications/Internet
License:   GPLv2+
URL:       http://www.hexchat.org
Source:    https://github.com/downloads/hexchat/hexchat/hexchat-%{version}.tar.xz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires: perl, perl(ExtUtils::Embed), python-devel, tcl-devel, pciutils-devel
BuildRequires: dbus-devel, dbus-glib-devel
BuildRequires: glib2-devel >= 2.10.0, gtk2-devel >= 2.10.0, bison >= 1.35
BuildRequires: libtool, autoconf, gettext-devel, pkgconfig
BuildRequires: libproxy-devel, libsexy-devel, libnotify-devel, openssl-devel
BuildRequires: desktop-file-utils, hicolor-icon-theme
Requires: hwdata

%description
HexChat is an easy to use graphical IRC chat client for the X Window System.
It allows you to join multiple IRC channels (chat rooms) at the same time, 
talk publicly, private one-on-one conversations etc. Even file transfers
are possible.

%package tcl
Summary:  Tcl script plugin for HexChat
Group:    Applications/Internet
Requires: %{name}%{?_isa} = %{version}-%{release}
%description tcl
The HexChat plugin providing the Tcl scripting interface.

%package perl
Summary:  Perl script plugin for HexChat
Group:    Applications/Internet
Requires: %{name}%{?_isa} = %{version}-%{release}
%description perl
The HexChat plugin providing the Perl scripting interface.

%package python
Summary:  Python script plugin for HexChat
Group:    Applications/Internet
Requires: %{name}%{?_isa} = %{version}-%{release}
%description python
The HexChat plugin providing the Python scripting interface.

%prep
%setup -q

%build
./autogen.sh

%configure --enable-ipv6 \
           --enable-spell=libsexy \
           --enable-tcl=%{_libdir} \
           --enable-shm

make %{?_smp_mflags}

%install
%{__rm} -rf $RPM_BUILD_ROOT
%{__make} install DESTDIR=$RPM_BUILD_ROOT

# Add SVG for hicolor
%{__install} -D -m644 share/icons/hexchat.svg $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/scalable/apps/hexchat.svg

# Get rid of libtool archives
%{__rm} -f $RPM_BUILD_ROOT%{_libdir}/hexchat/plugins/*.la

# Remove unused schema
%{__rm} -f $RPM_BUILD_ROOT%{_sysconfdir}/gconf/schemas/apps_hexchat_url_handler.schemas

# Fix opening irc:// links by adding mimetype and editing exec
desktop-file-edit --set-key=Exec --set-value='sh -c "hexchat --existing --url %U || exec hexchat"' \
    $RPM_BUILD_ROOT%{_datadir}/applications/hexchat.desktop

desktop-file-edit --add-mime-type='x-scheme-handler/irc;x-scheme-handler/ircs' \
    $RPM_BUILD_ROOT%{_datadir}/applications/hexchat.desktop

%find_lang %{name}

%clean
%{__rm} -rf $RPM_BUILD_ROOT

%post
gtk-update-icon-cache %{_datadir}/icons/hicolor
update-desktop-database

%postun
gtk-update-icon-cache %{_datadir}/icons/hicolor
update-desktop-database

%files -f %{name}.lang
%defattr(-,root,root)
%{_bindir}/hexchat
%doc share/doc/*
%dir %{_libdir}/hexchat
%dir %{_libdir}/hexchat/plugins
%{_libdir}/hexchat/plugins/checksum.so
%{_libdir}/hexchat/plugins/doat.so
%{_libdir}/hexchat/plugins/fishlim.so
%{_libdir}/hexchat/plugins/sysinfo.so
%{_datadir}/applications/hexchat.desktop
%{_datadir}/icons/hicolor/scalable/apps/hexchat.svg
%{_datadir}/pixmaps/*
%{_datadir}/dbus-1/services/org.hexchat.service.service
%{_mandir}/man1/hexchat.1.gz

%files tcl
%defattr(-,root,root)
%{_libdir}/hexchat/plugins/tcl.so

%files perl
%defattr(-,root,root)
%{_libdir}/hexchat/plugins/perl.so

%files python
%defattr(-,root,root)
%{_libdir}/hexchat/plugins/python.so

%changelog
* Sat Oct 27 2012 TingPing <tingping@tingping.se> - 2.9.4-1
- Version bump to 2.9.4
- Split up python and perl packages

* Fri Oct 19 2012 TingPing <tingping@tingping.se> - 2.9.3-1
- Version bump to 2.9.3
- Added COPYING to doc

* Sat Oct 6 2012 TingPing <tingping@tingping.se> - 2.9.2-1
- Version bump to 2.9.2

* Sat Sep 1 2012 TingPing <tingping@tingping.se> - 2.9.1-1
- first version for hexchat 2.9.1

