#*******************************************************************************
#  Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
#  The contents of this file are subject to the Mozilla Public License
#  Version 1.1 (the "License"); you may not use this file except in
#  compliance with the License. You may obtain a copy of the License at
#  http://www.mozilla.org/MPL/
#
#  Software distributed under the License is distributed on an "AS IS"
#  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
#  License for the specific language governing rights and limitations
#  under the License.
#
#  The Original Code is ICMA
#
#  The Initial Developer of the Original Code is University of Auckland,
#  Auckland, New Zealand.
#  Copyright (C) 2011-2014 by the University of Auckland.
#  All Rights Reserved.
#
#  Contributor(s): Jagir R. Hussan
#
#  Alternatively, the contents of this file may be used under the terms of
#  either the GNU General Public License Version 2 or later (the "GPL"), or
#  the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
#  in which case the provisions of the GPL or the LGPL are applicable instead
#  of those above. If you wish to allow use of your version of this file only
#  under the terms of either the GPL or the LGPL, and not to allow others to
#  use your version of this file under the terms of the MPL, indicate your
#  decision by deleting the provisions above and replace them with the notice
#  and other provisions required by the GPL or the LGPL. If you do not delete
#  the provisions above, a recipient may use your version of this file under
#  the terms of any one of the MPL, the GPL or the LGPL.
#
#
#*******************************************************************************
#!/bin/bash

# only run this in a pristine export of an svn tag.
# It should only be run on unix, more specifically, where sha1sum is available.

#version should be something like 0.9.0beta or 0.9.0, should match the name of the svn export.
version=$1

if [ -z $version ]; then
    echo "Please pass in a version number"
    exit 1
fi

dobuild() {
	./build.sh profile=standard profile=cdn releaseName=$1 cssOptimize=comments.keepLines optimize=closure layerOptimize=closure stripConsole=normal version=$1 copyTests=false mini=true action=release
	mv ../../release/$1 ../../release/$1-cdn
	cd ../../release/$1-cdn
	zip -rq $1.zip $1/*
	sha1sum $1.zip > sha1.txt
	cd $1
	find . -type f -exec sha1sum {} >> ../sha1.txt \;
}

# Generate locale info
cd cldr
ant clean  # necessary until cldr scripts can handle existing AMD files
ant
cd ..

# Setup release area
mkdir -p ../../release/$version-cdn

# Google build
dobuild $version
