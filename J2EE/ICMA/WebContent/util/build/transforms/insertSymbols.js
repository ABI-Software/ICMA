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
	"../buildControl",
	"../fileUtils",
	"../fs",
	"../replace"
], function(bc, fileUtils, fs, replace) {
	var symctr = 1,
		m = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
		len = m.length,

		generateSym = function(name, symtbl){
			var ret = name; //defaults to long symbol
			if(bc.symbol === "short"){
				var s=[], c = symctr;
				while(c){
					s.unshift(m[c%len]);
					c = Math.floor(c/len);
				}
				s = "$D" + s.join('');
				symctr++;
				symtbl[s + "_"] = name;
				ret = s + "_";
			}
			return ret;
		},

		convertSym = function(orig_name, symtbl){
			var name = orig_name.replace(/\./g, "_");
			if(bc.symbol !== "short" && orig_name === name){
				if(name === 'define'){
					//if the function is assigned to a variable named define, use
					//DEFINE instead to prevent messing up other transform steps
					name = 'DEFINE';
				}

				//if the original name does not have dot in it, don't use
				//the name directly: it will mess up the original logic, such as
				//in the following case:
				//		if(some_condition){
				//			var my_special_func=function(){...};
				//		}else{
				//			var my_special_func=function(){...};
				//		}
				//if the two anonymous functions are named my_special_func,
				//no matter what "some_condition" evaluates to, in the built
				//code, my_special_func will always be equal to the
				//implementation in the else branch
				//so we have to append something random to the name
				return name+"__"+Math.floor(Math.random()*10000);
			}
			return generateSym(name, symtbl);
		},

		insertSymbols = function(resource, symtbl){
			var content = resource.getText(),
				prefixes = [],
				addFunctionName = function(str, p1, p2, p3, p4){
					return p1+p2+p3+" "+generateSym(prefixes+p2, symtbl)+p4;
				};

			if(resource.pid){
				prefixes.push(resource.pid);
			}
			if(resource.mid){
				prefixes.push(resource.mid.replace(/\//g,'_'));
			}
			if(!prefixes.length){
				var m = content.match(/dojo\.provide\("(.*)"\);/);
				if(m){
					prefixes.push(m[1].replace(/\./g, "_"));
				}
			}
			if(prefixes.length){
				prefixes = prefixes.join('_').replace(/\.|\-/g,'_')+"_";
				content = content.replace(/^(\s*)(\w+)(\s*:\s*function)\s*(\(.*)$/mg, addFunctionName).
					replace(/^(\s*this\.)(\w+)(\s*=\s*function)\s*(\(.*)$/mg, addFunctionName);
			}
			content = content.replace(/^(\s*)([\w\.]+)(\s*=\s*function)\s*(\(.*)/mg,function(str, p1, p2, p3, p4){
				return p1+p2+p3+" "+convertSym(p2, symtbl)+p4;
			});
			return content;
		},

		warningIssued = 0;

	return function(resource, callback) {
		if(bc.symbol){
			if(resource.tag.report){
				if(bc.symbol === 'short'){
					bc.symbolTable = {};
					resource.reports.push({
						dir:".",
						filename:"symboltable.txt",
						content: function(){
							var symbolText = [], key, symtbl = bc.symbolTable;
							for(key in symtbl){
								symbolText.push(key + ": \"" + symtbl[key] + "\"" + bc.newline);
							}
							return symbolText.join('');
						}
					});
				}
			}else{
				if(!warningIssued){
					warningIssued = 1;
					bc.log("symbolsLeak", []);
				}
				fileUtils.ensureDirectoryByFilename(resource.dest);
				resource.text = insertSymbols(resource, bc.symbolTable);
			}
		}
		return 0;
	};
});
