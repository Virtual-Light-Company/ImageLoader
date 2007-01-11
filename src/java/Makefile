#*********************************************************************
#
#  (C) 2002 Justin Couch
#  http://www.vlc.com.au/~justin/
# 
# Makefile for the native code libraries
#
# Author: Justin Couch
# Version: $Revision: 1.3 $
#
#*********************************************************************

# Names of packages to be compiled. These are declared in the order that
# they are to be compiled in. We create one at a time
BUILD_ORDER = \
  vlc.net.content.image \
  vlc.net.content 

# List of packages that have native code libraries in use and require
# JNI header generation
NATIVE_PACKAGES = vlc.net.content.image

# Names of packages that we want included in the javadoc
JAVADOC_PACKAGES = \
  vlc.net.content.image \
  vlc.net.content 

# A version number we tack onto the JAR file name for eclipse versioning
# purposes.
JAR_VERSION=1.1.0

# Names of JAR files we want to create
JARS = imageloader.jar

# 3rd party libraries that need including for JAVADOC generation
JARS_JAVADOC=

#*********************************************************************
#
# Javadoc
#
#*********************************************************************

WINDOWTITLE = 'VLC Content Handlers Specification'
DOCTITLE    = 'VLC ImageLoader API'
HEADER      = 'VLC ImageLoader API'
BOTTOM      = '<font size=2>Copyright &copy; 2000 - 2007 The Virtual Light Company \
		   <a href=\"http://www.vlc.com.au\">VLC</a></font>'

# A list of the local directories that contain other Javadoc that we want
# to link to. For example we link to the JDK, JAXP, SAI and JUnit Javadocs
# here. Note that these *must* be full URLs and not just local directory 
# references. If your directory path contains a space in it, you may be in
# trouble. We haven't tested it with that. Try using double quotes inside
# the single quotes to get the space treated in the file name
#LINK_URLS   = 'file:///c:/java/jaxp-1.1/docs/api/' \
#              'file:///c:/java/junit3.5/javadoc/' \
#			  'file:///c:/java/j3d/1.2.1/html/'

include $(PROJECT_ROOT)/make/Makefile.jav 
