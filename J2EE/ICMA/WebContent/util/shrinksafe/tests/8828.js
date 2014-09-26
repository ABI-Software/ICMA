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
var result = [], string_tests;

(function(){
	// testing string munging

	var n = "c";
	result.push("a" + "b", "a" + n);

	var ll = "+";
	result.push(ll);

	var color = "red";
	var f = "The" + "Quick" + color + "Fox";
	result.push(f);

	var h = 4;
	var multiline = "this" +
		"is" + "a" + "test"
		+ "spanning" +
		h + "lines"
	;
	result.push(multiline);

	// aliases. all "bar"
	var a = "bar", b = 'bar', c = a;

	// a multi-line string outside of the array
	var ml = "testing" +
		"multiple" +
		"lines";

	var val = [
		"test" + "ing",
		"test" + a + "simple",
		"testing" + "combined" + b + "variables",
		"test \"+" + "weird syntax",
		'test' + 'basic',
		'test "mixed"',
		ml,
		'test "mixed" and' + 'munge',
		"t" + "e" + "s" + "t",
		"t" + "e" + "s" + c + "t",
		// weirdest example imaginable?:
		'"slightly"+"off"',
		// fail:
		!"a" + "b",
		(!"a") + "b",
		!("a") + "b"
	];

	string_tests = function(){
		return val;
	}

})();