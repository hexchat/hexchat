## RPM spec file designed for Fedora Core 4, 5 or 6 ##

# set this to 0 for FC4
%define fedora5_or_newer 1

Summary: Graphical IRC (chat) client
Summary(fr): Client IRC (chat) avec interface graphique
Summary(de): IRC-Client (Chat) mit grafischer Oberfläche
Name: xchat
Version: 2.8.2
Release: 0
Epoch: 1
Group: Applications/Internet
License: GPL
URL: http://xchat.org
Source: http://xchat.org/files/source/2.8/xchat-%{version}.tar.bz2
Buildroot: %{_tmppath}/%{name}-%{version}-root
Requires: gtk2 openssl
BuildRequires: gettext openssl-devel gtk2-devel dbus-devel

%description
A GUI IRC client with DCC file transfers, C plugin interface, Perl
and Python scripting capability, mIRC color, shaded transparency,
tabbed channels and more.

%package perl
Summary: XChat Perl plugin
Group: Applications/Internet
Requires: xchat >= 2.0.9
# Ensure that a compatible libperl is installed
Requires: perl(:MODULE_COMPAT_%(eval "`%{__perl} -V:version`"; echo $version))
%description perl
Provides Perl scripting capability to XChat.

%package python
Summary: XChat Python plugin
Group: Applications/Internet
Requires: xchat >= 2.0.9
Requires: python >= 2.3.0
%description python
Provides Python scripting capability to XChat.

%package tcl
Summary: XChat TCL plugin
Group: Applications/Internet
Requires: xchat >= 2.0.0
Requires: tcl
%description tcl
Provides TCL scripting capability to XChat.

%prep
%setup -q

%build
%configure --disable-dependency-tracking
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT%{_datadir}/applications $RPM_BUILD_ROOT%{_datadir}/pixmaps $RPM_BUILD_ROOT%{_libdir}/xchat/plugins
%makeinstall
%{__mkdir_p} $RPM_BUILD_ROOT%{_libdir}/xchat/plugins
strip -R .note -R .comment $RPM_BUILD_ROOT%{_libdir}/perl.so
strip -R .note -R .comment $RPM_BUILD_ROOT%{_libdir}/python.so
strip -R .note -R .comment $RPM_BUILD_ROOT%{_libdir}/tcl.so
mv $RPM_BUILD_ROOT%{_libdir}/perl.so $RPM_BUILD_ROOT%{_libdir}/xchat/plugins
mv $RPM_BUILD_ROOT%{_libdir}/python.so $RPM_BUILD_ROOT%{_libdir}/xchat/plugins
mv $RPM_BUILD_ROOT%{_libdir}/tcl.so $RPM_BUILD_ROOT%{_libdir}/xchat/plugins
rm -f $RPM_BUILD_ROOT%{_libdir}/*.la

%find_lang %name

%post
%if %{fedora5_or_newer}
# Install schema
export GCONF_CONFIG_SOURCE=`gconftool-2 --get-default-source`
SCHEMAS="apps_xchat_url_handler.schemas"
for S in $SCHEMAS; do
  gconftool-2 --makefile-install-rule /etc/gconf/schemas/$S > /dev/null
done
unset GCONF_CONFIG_SOURCE
%endif

%files -f %{name}.lang
%defattr(-,root,root)
%doc README ChangeLog faq.html plugins/plugin20.html plugins/perl/xchat2-perldocs.html
%{_bindir}/xchat
%{_datadir}/applications/xchat.desktop
%{_datadir}/pixmaps/xchat.png
%if %{fedora5_or_newer}
%{_sysconfdir}/gconf/schemas/apps_xchat_url_handler.schemas
%endif

%files perl
%{_libdir}/xchat/plugins/perl.so

%files python
%{_libdir}/xchat/plugins/python.so

%files tcl
%{_libdir}/xchat/plugins/tcl.so

%clean
rm -rf $RPM_BUILD_ROOT
