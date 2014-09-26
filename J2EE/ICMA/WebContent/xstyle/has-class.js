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
define(["dojo/has"], function(has){
	var tested = {};
	return function(){
		var test, args = arguments;
		for(var i = 0; i < args.length; i++){
			var test = args[i];
			if(!tested[test]){
				tested[test] = true;
				var parts = test.match(/^(no-)?(.+?)((-[\d\.]+)(-[\d\.]+)?)?$/), // parse the class name
					hasResult = has(parts[2]), // the actual has test
					lower = -parts[4]; // lower bound if it is in the form of test-4 or test-4-6 (would be 4)
				if((lower > 0 ? lower <= hasResult && (-parts[5] || lower) >= hasResult :  // if it has a range boundary, compare to see if we are in it
						!!hasResult) == !parts[1]){ // parts[1] is the no- prefix that can negate the result
					document.documentElement.className += ' has-' + test;
				}
			}
		}
	}
});