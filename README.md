ympd
====

Standalone MPD Web GUI written in C, utilizing Websockets and Bootstrap/JS

http://www.ympd.org


Dependencies
------------
 - libwebsockets master branch: http://git.libwebsockets.org/cgi-bin/cgit/libwebsockets
 - libmpdclient 2: http://www.musicpd.org/libs/libmpdclient/
 - cmake 2.6: http://cmake.org/
 - Optional: OpenSSL for SSL support in libwebsockets webserver

Unix Build Instructions
-----------------------

1. Install Dependencies, cmake, openssl and libmpdclient are available from all major distributions.
2. create build directory ```cd /path/to/src; mkdir build; cd build```
3. create makefile ```cmake ..  -DCMAKE_INSTALL_PREFIX:PATH=/usr```
4. build ```make```
5. install ```sudo make install``` or build debian package ```cpack -G DEB; sudo dpkg -i ympd*.deb```

Copyright
---------

2013-2014 <andy@ndyk.de>
