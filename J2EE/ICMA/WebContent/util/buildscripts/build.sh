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
#!/bin/sh
usage() {
cat <<-__EOF__;
NAME
     build.sh - a convenience wrapper around the Dojo Build Application

SYNOPSIS
     path/to/build.sh [--help] [--bin environment] [build system options]

DESCRIPTION
     build.sh is a shell script that wraps the Dojo Build Application located at /util/build/main.js
     to simplify executing the application in various, selectable, Javascript environments. Currently
     both node.js and Java are supported.

OPTIONS
     --help     print the help message
     
     --bin      environment
                Specifies the Javascript environment to use. Defaults to node, if available, java otherwise.
     
                node             use node.js, if available, automatic downgrade to java
                node-debug         same as node, with the --debug argument
                node-debug-brk     same as node with the --debug-brk argument
                java             use java
     
     Note: the alternative syntax bin=option is supported but deprecated.

__EOF__
}

if [ "$#" = "0" ]; then
 usage
fi

while [ -n "$1" ]
do
	arg="$1"
    case $arg in
    --help)
        usage
        ba="$ba $arg"
        ;;
    bin=node)
        use_node=0
        ;;
    bin=node-debug)
        use_node=0
        debug_node="--debug"
		;;
    bin=node-debug-brk)
        use_node=0
        debug_node="--debug-brk"
        ;;
    bin=java)
        use_node=1
        ;;
    bin=*)
        echo "Invalid bin= option: only node/java is supported"
        exit 1
        ;;
    *)
		if [ "$arg" = "--bin" ]; then
			case $2 in
			node)
				use_node=0
				;;
			node-debug)
				use_node=0
				debug_node="--debug"
				;;
			node-debug-brk)
				use_node=0
				debug_node="--debug-brk"
				;;
			java)
				use_node=1
				;;
			*)
		        echo "Invalid --bin option: only node/java is supported"
				exit 1
		        ;;
			esac
			shift
		else
	        ba="$ba $arg"
		fi
        ;;
    esac
    shift
done

if [ -z "$use_node" ]; then
    which node > /dev/null 2>&1
    use_node=$?
fi

if [ "$use_node" = "0" ]; then
    cmd="node $debug_node"
    cmdflags="`dirname $0`/../../dojo/dojo.js"
else
    cmd="java"
    cmdflags="-Xms256m -Xmx256m -cp `dirname $0`/../shrinksafe/js.jar:`dirname $0`/../closureCompiler/compiler.jar:`dirname $0`/../shrinksafe/shrinksafe.jar org.mozilla.javascript.tools.shell.Main  `dirname $0`/../../dojo/dojo.js baseUrl=`dirname $0`/../../dojo"
fi

$cmd $cmdflags load=build $ba
