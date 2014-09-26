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

#version should be something like 0.9.0beta or 0.9.0
name=$1
#The svn revision number to use for tag. Should be a number, like 11203
svnRevision=$2

#If no svnRevision number, get the latest one from he repo.
if [ "$svnRevision" = "" ]; then
    svnRevision=`svn info http://svn.dojotoolkit.org/src/util/trunk/buildscripts/build_release.sh | grep Revision | sed 's/Revision: //'`
fi

svn mkdir -m "Using r$svnRevision to create a branch named $name." https://svn.dojotoolkit.org/src/branches/$name
svn copy -r $svnRevision https://svn.dojotoolkit.org/src/dojo/trunk  https://svn.dojotoolkit.org/src/branches/$name/dojo -m "Using r$svnRevision to create a branch named $name."
svn copy -r $svnRevision https://svn.dojotoolkit.org/src/dijit/trunk https://svn.dojotoolkit.org/src/branches/$name/dijit -m "Using r$svnRevision to create a branch named $name."
svn copy -r $svnRevision https://svn.dojotoolkit.org/src/dojox/trunk https://svn.dojotoolkit.org/src/branches/$name/dojox -m "Using r$svnRevision to create a branch named $name."
svn copy -r $svnRevision https://svn.dojotoolkit.org/src/util/trunk  https://svn.dojotoolkit.org/src/branches/$name/util -m "Using r$svnRevision to create a branch named $name."
svn copy -r $svnRevision https://svn.dojotoolkit.org/src/demos/trunk https://svn.dojotoolkit.org/src/branches/$name/demos -m "Using r$svnRevision to create a branch named $name."

