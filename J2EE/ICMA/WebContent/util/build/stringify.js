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
	"dojo/json",
	"./buildControlBase"
], function(json, bc){
var
	spaces = "					 ",
	indentFactor = 2,

	setIndentFactor = function(factor){
		indentFactor = factor;
	},

	indent = function(n, factor){
		n = n * (factor || indentFactor);
		while(spaces.length<n) spaces+= spaces;
		return spaces.substring(0, n);
	},

	propName = function(name){
		return /^[\w\$]+$/.test(name) ?
			name + ":" :
			"'" + name + "':";
	},

	text,
	unsolved,

	split = function(
		text //(string) Text to split into lines.
	){
		///
		// Split text into a vector lines as given by new-line indicators contained in text
		///
		// Three-step algorithm:
		//	 * turn CR-LF or LF-CR into simple LF
		//	 * turn lone CR into LF
		//	 * then split on LF
		//
		// This should be platform-independent
		return text.replace(/(\r\n)|(\n\r)/g, "\n").replace(/\r/, "\n").split("\n");
	},

	stringify = function(it, level){
		if(!level){
			text = "";
			unsolved = false;
			level = 1;
		}else{
			level++;
		}
		var temp, space, p, i, newline = bc.newline;
		switch(typeof it){
			case "undefined":
				text+= "undefined";
				break;

			case "boolean":
				text+= (it ? "true" : "false");
				break;

			case "number":
				text+= it.toString();
				break;

			case "string":
				text+= json.stringify(it);
				break;

			case "object":
				if(it===null){
					text+= "null";
				}else if(it instanceof RegExp){
					text+= RegExp.toString();
				}else if(it instanceof Array){
					if(it.length>1){
						text+= "[" + newline;
						for(i = 0; i<it.length-1; i++){
							text+= indent(level);
							stringify(it[i], level);
							text+= "," + newline;
						}
						text+= indent(level);
						stringify(it[i], level);
						text+= newline + indent(level-1) + "]";
					}else if(it.length){
						text+= "[";
						stringify(it[0], level);
						text+= "]";
					}else{
						text+= "[]";
					}
				}else{
					temp = [];
					for(p in it) temp.push(p);
					temp.sort();
					if(temp.length>1){
						text+= "{" + newline;
						for(i = 0; i<temp.length-1; i++){
							text+= indent(level) + propName(temp[i]);
							stringify(it[temp[i]], level);
							text+= "," + newline;
						}
						text+= indent(level) + propName(temp[i]);
						stringify(it[temp[i]], level);
						text+= newline;
						text+= indent(level-1) + "}";
					}else if(temp.length){
						text+= "{" + propName(temp[0]);
						stringify(it[temp[0]], level);
						text+= "}";
					}else{
						text+= "{}";
					}
				}
				break;

			case "function":
				space = indent(level);
				// the V8 engine seems to strip the leading space from the first line of the function?!?
				var
					functionText = split(it.toString()),
					firstLine = functionText.shift(),
					minSpaces = Number.MAX_VALUE;
				functionText.forEach(function(line){
					// ignoring lines that have no non-space chars
					var match = line.match(/(\s*)\S/);
					if(match) minSpaces = Math.min(minSpaces, match[1].length);
				});
				if(minSpaces==Number.MAX_VALUE){
					//every line started without any spaces and we never got a match
					minSpaces = 0;
				}
				functionText.unshift(indent(minSpaces, 1) + firstLine);
				//every line has at least minSpaces indentation...
				text+= newline + functionText.map(function(line){ return space + line.substring(minSpaces); }).join(newline);
				break;

			default:
				text+= "undefined /* unsolved */";
				unsolved = true;
		}
		text.unsolved = unsolved;
		return text;
	};

stringify.setIndentFactor = setIndentFactor;
stringify.split = split;
return stringify;

});
