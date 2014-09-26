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
define(["../buildControl"], function(bc){
	//
	// For and feature, "x", in bc.staticHasFeatures, replace the string has("x") with the value as provided by bc.staticHasFeatures.
	//
	// For example, consider the following code
	//
	// if(has("feature-a")){
	//   //...
	// }
	//
	// if(has("feature-b")>=7){
	//   //...
	// }
	//
	// if(has("feature-c")){
	//   //...
	// }
	//
	// if(has("feature-d")){
	//   //...
	// }
	//
	// if(has("feature-e")=="goodbye"){
	//   //...
	// }
	//
	// Then, given bc.staticHasFeatures of
	//
	//   {
	//		"feature-a":undefined,
	//		"feature-b":7,
	//		"feature-c":true,
	//		"feature-d":false,
	//		"feature-e":"'hello, world'"
	//   }
	//
	// The source is modified as follows:
	// if(undefined){
	//   //...
	// }
	//
	// if(7>=7){
	//   //...
	// }
	//
	// if(true){
	//   //...
	// }
	//
	// if(false){
	//   //...
	// }
	//
	// if('hello, world'=="goodbye"){
	//   //...
	// }
	//
	// Recall that has.add has a now parameter that causes the test to be executed immediately and return the value.
	// If a static has value is truthy, then using || causes the first truthy operand to be returned...the truthy static value.
	// If a static value is falsy, then using && causes the first falsy operand to be returned...the falsy static value.
	//
	// For example:
	//
	// if(has.add("feature-a", function(global, doc, element){
	//   //some really long, painful has test
	// }, true)){
	//   //...
	// }
	//
	// Consider static has values for feature-a of undefined, 7, true, false, "'hello, world'"
	//
	//
	// if(undefined && has.add("feature-a", function(global, doc, element){ // NOTE: the value of the if conditional is static undefined
	//   //some really long, painful has test
	// }, true)){
	//   //...
	// }
	//
	// if(7 || has.add("feature-a", function(global, doc, element){ // NOTE: the value of the if conditional is static 7
	//   //some really long, painful has test
	// }, true)){
	//   //...
	// }
	//
	// if(true || has.add("feature-a", function(global, doc, element){ // NOTE: the value of the if conditional is static true
	//   //some really long, painful has test
	// }, true)){
	//   //...
	// }
	//
	// if(false && has.add("feature-a", function(global, doc, element){ // NOTE: the value of the if conditional is static false
	//   //some really long, painful has test
	// }, true)){
	//   //...
	// }
	//
	// if('hello, world' || has.add("feature-a", function(global, doc, element){ // NOTE: the value of the if conditional is static 'hello, world'
	//   //some really long, painful has test
	// }, true)){
	//   //...
	// }
	//
	// This technique is employed to avoid attempting to parse and find the boundaries of "some really long painful has test" with regexs.
	// Instead, this is left to an optimizer like the the closure compiler or uglify etc.

	function stringifyString(s){
		return typeof s === "string" ? '"' + s + '"' : s;
	}

	return function(resource){
		resource.text = resource.text.replace(/([^\w\.])has\s*\(\s*["']([^'"]+)["']\s*\)/g, function(match, prefix, featureName){
			if(featureName in bc.staticHasFeatures){
				return prefix + " " + bc.staticHasFeatures[featureName] + " ";
			}else{
				return match;
			}
		}).replace(/([^\w\.])((has.add\s*\(\s*)["']([^'"]+)["'])/g, function(match, prefix, hasAdd, notUsed, featureName){
			if(featureName in bc.staticHasFeatures){
				return prefix + " " + stringifyString(bc.staticHasFeatures[featureName]) + (bc.staticHasFeatures[featureName] ? " || " : " && " ) + hasAdd;
			}else{
				return match;
			}
		});
		return 0;
	};

});
