
                   Content Handlers for Image Loading

                       (c) ADI Limited 1998-1999
                   (c) The Virtual Light Company 1999-2002


Content handlers are Java's method of adding support for different MIMEType
based handling for information coming from a stream. Typically this is used
in conjunction with the java.net URL classes but may be more widely used.
For example, we use it seamlessly with the URI class libraries in our
applications.

Licensing
-----------

ADI (my employer) has allowed us to give away the code as a gesture of
appreciate to the Free Software community. I am the one responsible now
for dealing with everything - they've washed their hands of it in a legal
way. That is the reason you see two (c) lines above.

This code is released under the GNU LGPL. If you wish to redistribute
this code then you must include the license.txt file as is.

The original copy of this code can be found at

http://www.vlc.com.au/imageloader/

This code makes use of other 3rd party libraries that contain a variety of
different licenses. They are all less restrictive than the LGPL in usage.
These are the libraries currently in use:

- libpng  v1.0.2
- libjpeg v6.0a
- NetPBM libraries
- libTIFF from Sam Leffer v3.4beta037


The GNU LGPL can be found in license.txt or at http://www.gnu.org/lgpl.html

#include <std_disclaimer.h>



Installing
-----------

Unzip/untar the archive all this came in. It is composed of the following
top level directories:

image-loader-1.0
  |
  + src
  + lib
  + examples
  + docs

In the lib directory you will find a prebuild JAR file of the Java classes
called imageloader-1.0.jar. There will also be a prebuilt set of libs for
your particular platform. For Win32 users, this will be a DLL. For unix
users this is a .so file. Install these in the appropriate directory that
you keep native libs in. Normally, this should be in the $jre.home/lib/ext
dir.

The software, as delivered, requires JDK 1.2 to run as it returns
java.awt.image.BufferedImage objects to represent an image. By hand modifying
the code you can get it to run with JDK 1.1 (remove comments is all that needs
to be done).

Using
--------

These classes load native libraries. In Java 2 this could cause security
exceptions depending on your setup. If this happens, consult the javadoc for
JDK1.2 (I'll do a writeup on this later).

To use it in your application, you will need to make Java aware of this. You
can use one of 2 options:

 - set the system property java.content.handler.pkgs to point to the name of
 the package (vlc.content).

 - Create a ContentHandlerFactory and get it to return instances of these
 handlers when requested. We have provided a default implementation in
 vlc.content for the lazy.

Make sure that the system knows about the various MIME type mappings for the
file extensions so that it can make use of the loaders. You can edit
$java.home/lib/content-types.properties to add a new type or you can specify
your own FileNameMap and set that in java.net.URLConnection. Note that there
is a bug in URLConnection where it will trash your FileNameMap at the first
time that it reads a file:// URL. The simple workaround is to load some file
right at the start of the app and then set the map.

Now create a URL eg file://usr/users/justin/myimage.png, create a URL and
then fetch the content of that URL. This should return you an Image object
that you are free to use wherever.

Note: These content handlers DO NOT work with java.awt.Toolkit.createImage().
You are much better off just creating a URL and accessing it directly for all
your image loading under this toolkit. It is much faster and much more memory
efficient.

Compiling
----------

All of the native code can be found in src/native. There are
build scripts there for the native code. Also make sure that you view the
readmes.

The code has been tested and compiled under NT4.0/SP3, Solaris 2.6/2.7 and
HP-UX 10.20. If you have any other ports for different platforms - let us know!

The java code can be compiled on the command line as normal.

There are no general build scripts available at this time.

TODO:
------

- Write build scripts :)

- Tidy up documentation (ie Provide some and the web page).

Justin Couch, 20 Jan 2002
justin@vlc.com.au 

