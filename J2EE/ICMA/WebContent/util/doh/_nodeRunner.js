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
define(["doh/runner", "require"], function(doh, require){
	/*=====
	return {
		// summary:
		//		Module for running DOH tests in node (as opposed to a browser).
		//		Augments return value from doh/runner.
	};
	=====*/

	doh.debug= console.log;
	doh.error= console.log;

	// Override the doh._report method to make it quit with an
	// appropriate exit code in case of test failures.
	var oldReport = doh._report;
	doh._report = function(){
		oldReport.apply(doh, arguments);
		if(this._failureCount > 0 || this._errorCount > 0){
			process.exit(1);
		}
	};

	console.log("\n"+doh._line);
	console.log("The Dojo Unit Test Harness, $Rev: 23869 $");
	console.log("Copyright (c) 2011, The Dojo Foundation, All Rights Reserved");
	console.log("Running with node.js");
	for (var tests= [], args=doh.config["commandLineArgs"], i= 0, arg; i<args.length; i++) {
		arg= args[i];
		if (arg.length==2 && arg[0]=="test") {
			var test= arg[1];
			console.log("loading test " + test);
			tests.push(test);
		}
	}
	console.log(doh._line, "\n");

	require(tests, function() {
		doh.run();
	});
});
