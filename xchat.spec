Summary: Graphical IRC (chat) client
Name: xchat
Version: 1.9.8
Release: 0
Epoch: 1
Group: Applications/Internet
License: GPL
URL: http://xchat.org
Source: http://xchat.org/files/source/1.9/xchat-%{version}.tar.bz2
Buildroot: %{_tmppath}/%{name}-%{version}-root
Requires: gtk2 >= 2.0.6
Requires: openssl >= 0.9.6b

%description
A GUI IRC client with DCC file transfers, C plugin interface, Perl
and Python scripting capability, mIRC color, shaded transparency,
tabbed channels and more.

%package perl
Summary: XChat Perl plugin
Group: Applications/Internet
Requires: xchat >= 1.9.5
Requires: perl = 5.8.0
%description perl
Provides Perl scripting capability to XChat.

%package python
Summary: XChat Python plugin
Group: Applications/Internet
Requires: xchat >= 1.9.5
Requires: python = 2.2.1
%description python
Provides Python scripting capability to XChat.

%package tcl
Summary: XChat TCL plugin
Group: Applications/Internet
Requires: xchat >= 1.9.8
Requires: tcl
%description python
Provides TCL scripting capability to XChat.

%prep
%setup -q

%build
%configure --disable-textfe --enable-ipv6
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT%{_datadir}/applications $RPM_BUILD_ROOT%{_datadir}/pixmaps $RPM_BUILD_ROOT%{_libdir}/xchat/plugins
%makeinstall
strip -R .note -R .comment $RPM_BUILD_ROOT%{_libdir}/perl.so
strip -R .note -R .comment $RPM_BUILD_ROOT%{_libdir}/python.so
strip -R .note -R .comment $RPM_BUILD_ROOT%{_libdir}/tcl.so
mv $RPM_BUILD_ROOT%{_libdir}/perl.so $RPM_BUILD_ROOT%{_libdir}/xchat/plugins
mv $RPM_BUILD_ROOT%{_libdir}/python.so $RPM_BUILD_ROOT%{_libdir}/xchat/plugins
mv $RPM_BUILD_ROOT%{_libdir}/tcl.so $RPM_BUILD_ROOT%{_libdir}/xchat/plugins

%find_lang %name

%files -f %{name}.lang
%defattr(-,root,root)
%doc README ChangeLog faq.html COPYING plugins/plugin20.html plugins/perl/xchat-perl18.html
%{_bindir}/xchat
%{_datadir}/applications/xchat.desktop
%{_datadir}/pixmaps/xchat.png
#%{_libdir}/xchat/plugins/perl.so
#%{_libdir}/xchat/plugins/python.so
#%{_libdir}/xchat/plugins/tcl.so

%files perl
%{_libdir}/xchat/plugins/perl.so

%files python
%{_libdir}/xchat/plugins/python.so

%files tcl
%{_libdir}/xchat/plugins/tcl.so

%clean
rm -rf $RPM_BUILD_ROOT
