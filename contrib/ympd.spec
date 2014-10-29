#
# spec file for package ympd
#
# Copyright (c) 2014 Markus S. <kamikazow@web.de>
#
# This file is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

Name:           ympd
Version:        1.2.2
Release:        0%{?dist}
Summary:        ympd is a lightweight MPD (Music Player Daemon) web client
Group:          Applications/Multimedia
License:        GPL
URL:            http://www.ympd.org/

# For this spec file to work, the ympd sources must be located in a directory
# named ympd-1.2.2 (with "1.2.2" being the version number defined above).
# If the sources are compressed in another format than ZIP, change the
# file extension accordingly.
Source0:        %{name}-%{version}.zip

# Package names only verified with Fedora.
# Should the packages in your distro be named dirrerently,
# see http://en.opensuse.org/openSUSE:Build_Service_cross_distribution_howto
# %if 0%{?fedora}
BuildRequires:  cmake
BuildRequires:  unzip
BuildRequires:  libmpdclient-devel
Requires:       libmpdclient
# %endif

%description
ympd is a lightweight MPD (Music Player Daemon) web client that runs without
a dedicated webserver or interpreters like PHP, NodeJS or Ruby.
It's tuned for minimal resource usage and requires only very litte dependencies.


%prep
%setup -q

%build
mkdir build
pushd build
%cmake .. -DCMAKE_INSTALL_PREFIX_PATH=%{_prefix}
make PREFIX=%{_prefix} %{?_smp_mflags}
popd

%install
pushd build
%{make_install}
popd

%files
%defattr(-,root,root,-)
%doc LICENSE README.md
%{_bindir}/%{name}
%{_mandir}/man[^3]/*


%changelog
