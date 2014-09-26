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
result = "";

/* For the purpose of these tests, we have no actual console with
   which to test output, so create a dummy console and capture to
   'result', which is examined by the test framework post eval. */
var console = {
	log: function(arg) { result += "LOG: " + arg; },
	debug: function(arg) { result += "DEBUG: " + arg; },
	warn: function(arg) { result += "WARN: " + arg; },
	error: function(arg) { result += "ERROR: " + arg; },
	dir: function(arg) { result += "DIR: " + arg; }
};
(function() {
	var fn = function(arg) {
		return "fn saw arg '" + arg + "'.";
	}
	var str = "wooo";
	console.debug(str + "some \\ dodgy \" characters *$!?//" + (1+2) + (~2) + fn(str));
	console.error(str + "some \\ dodgy \" characters *$!?//" + (1+2) + (~2) + fn(str));
	
	// from ticket http://bugs.dojotoolkit.org/ticket/8549
	console.log("hello :) world!");
	console.log(" anything /) with a paren ");
	console.log(/^\)$/.test("hi"));

	//It would be interesting to see how this comes out:
	if(true)
	    console.log("foo");
	else
	   var two = "two";
	
	var bar;
	true ? console.log("bizarre") : (bar = true);
	true ? (bar = true) : console.log("bizarre");
	
	(function() {
		return console.debug("Debug return statement.");
	})();
	(function() {
		return console.error("Error return statement.");
	})();
})();