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
define(["../buildControl"], function(bc) {
	var evalPragma= function(code, kwArgs, fileName) {
			return !!eval("(" + code + ")");
		};

	return function(resource) {
		if(!bc.applyDojoPragmas){
			return;
		}
		if(typeof resource.text!="string"){
			return;
		}

		var
			foundIndex = -1,
			startIndex = 0,
			text= resource.text;
		while((foundIndex = text.indexOf("//>>", startIndex)) != -1){
			//Found a conditional. Get the conditional line.
			var lineEndIndex = text.indexOf("\n", foundIndex);
			if(lineEndIndex == -1){
				lineEndIndex = text.length - 1;
			}

			//Increment startIndex past the line so the next conditional search can be done.
			startIndex = lineEndIndex + 1;

			//Break apart the conditional.
			var conditionLine = text.substring(foundIndex, lineEndIndex + 1);
			var matches = conditionLine.match(/(exclude|include)Start\s*\(\s*["'](\w+)["']\s*,(.*)\)/);
			if(matches){
				var type = matches[1];
				var marker = matches[2];
				var condition = matches[3];
				var isTrue = false;
				//See if the condition is true.
				try{
					isTrue = evalPragma(condition, bc, resource.src);
				}catch(e){
					bc.log("dojoPragmaEvalFail", ["module", resource.mid, "expression", conditionLine, "error", e]);
					return;
				}

				//Find the endpoint marker.
				var endRegExp = new RegExp('\\/\\/\\>\\>\\s*' + type + 'End\\(\\s*[\'"]' + marker + '[\'"]\\s*\\)', "g");
				var endMatches = endRegExp.exec(text.substring(startIndex, text.length));
				if(endMatches){

					var endMarkerIndex = startIndex + endRegExp.lastIndex - endMatches[0].length;

					//Find the next line return based on the match position.
					lineEndIndex = text.indexOf("\n", endMarkerIndex);
					if(lineEndIndex == -1){
						lineEndIndex = text.length - 1;
					}

					//Should we include the segment?
					var shouldInclude = ((type == "exclude" && !isTrue) || (type == "include" && isTrue));

					//Remove the conditional comments, and optionally remove the content inside
					//the conditional comments.
					var startLength = startIndex - foundIndex;
					text = text.substring(0, foundIndex)
						+ (shouldInclude ? text.substring(startIndex, endMarkerIndex) : "")
						+ text.substring(lineEndIndex + 1, text.length);

					//Move startIndex to foundIndex, since that is the new position in the file
					//where we need to look for more conditionals in the next while loop pass.
					startIndex = foundIndex;
				}else{
					bc.log("dojoPragmaInvalid", ["module", resource.mid, "expression", conditionLine]);
					return;
				}
			}else if(/^\/\/>>\s*noBuildResolver\s*$/.test(conditionLine)){
				resource.noBuildResolver = 1;
			}
		}
		resource.text= text;
	};
});
