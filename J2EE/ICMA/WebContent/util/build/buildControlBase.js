/*******************************************************************************
 *  Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 *  The contents of this file are subject to the Mozilla Public License
 *  Version 1.1 (the "License"); you may not use this file except in
 *  compliance with the License. You may obtain a copy of the License at
 *  http://www.mozilla.org/MPL/
 *
 *  Software distributed under the License is distributed on an "AS IS"
 *  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 *  License for the specific language governing rights and limitations
 *  under the License.
 *
 *  The Original Code is ICMA
 *
 *  The Initial Developer of the Original Code is University of Auckland,
 *  Auckland, New Zealand.
 *  Copyright (C) 2011-2014 by the University of Auckland.
 *  All Rights Reserved.
 *
 *  Contributor(s): Jagir R. Hussan
 *
 *  Alternatively, the contents of this file may be used under the terms of
 *  either the GNU General Public License Version 2 or later (the "GPL"), or
 *  the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 *  in which case the provisions of the GPL or the LGPL are applicable instead
 *  of those above. If you wish to allow use of your version of this file only
 *  under the terms of either the GPL or the LGPL, and not to allow others to
 *  use your version of this file under the terms of the MPL, indicate your
 *  decision by deleting the provisions above and replace them with the notice
 *  and other provisions required by the GPL or the LGPL. If you do not delete
 *  the provisions above, a recipient may use your version of this file under
 *  the terms of any one of the MPL, the GPL or the LGPL.
 *
 *
 *******************************************************************************/
define([
	"./messages",
	"dojo/text!./copyright.txt",
	"dojo/text!./buildNotice.txt"
], function(messages, defaultCopyright, defaultBuildNotice){
	var bc = {
		// 0 => no errors
		// 1 => messages.getErrorCount()>0 at exist
		exitCode:0,

		// use this variable for all newlines inserted by build transforms
		newline:"\n",

		// user profiles may replace this with a function from string to string that filters newlines
		// however they desire. For example,
		//
		// newlineFilter: function(s){
		//	 // convert all DOS-style newlines to Unix-style newlines
		//	 return s.replace(/\r\n/g, "\n").replace(/\n\r/g, "\n");
		// }
		//
		newlineFilter:function(s, resource, hint){return s;},


		// useful for dojo pragma including/excluding
		built:true,

		startTimestamp:new Date(),

		paths:{},
		destPathTransforms:[],
		packageMap:{},

		// resource sets
		resources:{},
		resourcesByDest:{},
		amdResources:{},

		closureCompilerPath:"../closureCompiler/compiler.jar",
		maxOptimizationProcesses:5,
		buildReportDir:".",
		buildReportFilename:"build-report.txt",

		defaultCopyright:defaultCopyright,
		defaultBuildNotice:defaultBuildNotice
	};
	for(var p in messages){
		bc[p] = messages[p];
	};
	return bc;
});
