%define _default_patch_fuzz 2
%define gconf_version 2.14

Summary:   A popular and easy to use graphical IRC (chat) client
Name:      xchat
Version:   2.8.8
Release:   0%{?dist}
Epoch:     1
Group:     Applications/Internet
License:   GPLv2+
URL:       http://www.xchat.org
Source:    http://www.xchat.org/files/source/2.8/xchat-%{version}.tar.bz2
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

# Patches 0-9 reserved for official xchat.org patches

BuildRequires: perl perl(ExtUtils::Embed) python-devel openssl-devel pkgconfig, tcl-devel
BuildRequires: GConf2-devel
BuildRequires: dbus-devel >= 0.60, dbus-glib-devel >= 0.60
BuildRequires: glib2-devel >= 2.10.0, gtk2-devel >= 2.10.0, bison >= 1.35
BuildRequires: gettext /bin/sed
BuildRequires: libtool
BuildRequires: libsexy-devel
BuildRequires: desktop-file-utils >= 0.10
# For gconftool-2:
Requires(post): GConf2 >= %{gconf_version}
Requires(preun): GConf2 >= %{gconf_version}

# Ensure that a compatible libperl is installed
Requires: perl(:MODULE_COMPAT_%(eval "`%{__perl} -V:version`"; echo $version))

Provides: xchat-perl = %{epoch}:%{version}-%{release}
Obsoletes: xchat-perl < %{epoch}:%{version}-%{release}
Provides: xchat-python = %{epoch}:%{version}-%{release}
Obsoletes: xchat-python < %{epoch}:%{version}-%{release}

%description
X-Chat is an easy to use graphical IRC chat client for the X Window System.
It allows you to join multiple IRC channels (chat rooms) at the same time, 
talk publicly, private one-on-one conversations etc. Even file transfers
are possible.

This includes the plugins to run the Perl and Python scripts.

%package tcl
Summary: Tcl script plugin for X-Chat
Group: Applications/Internet
Requires: %{name} = %{epoch}:%{version}-%{release}
%description tcl
This package contains the X-Chat plugin providing the Tcl scripting interface.

%prep
%setup -q

%build
# Remove CVS files from source dirs so they're not installed into doc dirs.
find . -name CVS -type d | xargs rm -rf

export CFLAGS="$RPM_OPT_FLAGS $(perl -MExtUtils::Embed -e ccopts)"
export LDFLAGS=$(perl -MExtUtils::Embed -e ldopts)

%configure --disable-textfe \
           --enable-gtkfe \
           --enable-openssl \
           --enable-python \
           --enable-tcl=%{_libdir} \
           --enable-ipv6 \
           --enable-spell=libsexy \
           --enable-shm

# gtkspell breaks Input Method commit with ENTER

make %{?_smp_mflags}


%install
%{__rm} -rf $RPM_BUILD_ROOT
%{__make} install DESTDIR=$RPM_BUILD_ROOT GCONF_DISABLE_MAKEFILE_SCHEMA_INSTALL=1

# Get rid of libtool archives
%{__rm} -f $RPM_BUILD_ROOT%{_libdir}/xchat/plugins/*.la

# Install the .desktop file properly
%{__rm} -f $RPM_BUILD_ROOT%{_datadir}/applications/xchat.desktop
desktop-file-install --vendor="" \
  --dir $RPM_BUILD_ROOT%{_datadir}/applications \
  --add-category=IRCClient \
  --add-category=GTK xchat.desktop

%find_lang %{name}

# do not Provide plugins .so
%define _use_internal_dependency_generator 0
%{__cat} << \EOF > %{name}.prov
#!%{_buildshell}
%{__grep} -v %{_docdir} - | %{__find_provides} $* \
	| %{__sed} '/\.so\(()(64bit)\)\?$/d'
EOF
%define __find_provides %{_builddir}/%{name}-%{version}/%{name}.prov
%{__chmod} +x %{__find_provides}


%post
# Install schema
export GCONF_CONFIG_SOURCE=`gconftool-2 --get-default-source`
gconftool-2 --makefile-install-rule /etc/gconf/schemas/apps_xchat_url_handler.schemas >& /dev/null || :


%pre
if [ "$1" -gt 1 ]; then
  export GCONF_CONFIG_SOURCE=`gconftool-2 --get-default-source`
  gconftool-2 --makefile-uninstall-rule /etc/gconf/schemas/apps_xchat_url_handler.schemas >& /dev/null || :
fi

%preun
if [ "$1" -eq 0 ]; then
  export GCONF_CONFIG_SOURCE=`gconftool-2 --get-default-source`
  gconftool-2 --makefile-uninstall-rule /etc/gconf/schemas/apps_xchat_url_handler.schemas >& /dev/null || :
fi

%clean
%{__rm} -rf $RPM_BUILD_ROOT

%files -f %{name}.lang
%defattr(-,root,root)
%doc README ChangeLog
%doc plugins/plugin20.html plugins/perl/xchat2-perl.html
%{_bindir}/xchat
%dir %{_libdir}/xchat
%dir %{_libdir}/xchat/plugins
%{_libdir}/xchat/plugins/perl.so
%{_libdir}/xchat/plugins/python.so
%{_datadir}/applications/xchat.desktop
%{_datadir}/pixmaps/*
%{_sysconfdir}/gconf/schemas/apps_xchat_url_handler.schemas
%{_datadir}/dbus-1/services/org.xchat.service.service

%files tcl
%defattr(-,root,root)
%{_libdir}/xchat/plugins/tcl.so

