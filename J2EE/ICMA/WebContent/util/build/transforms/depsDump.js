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
	"dojo/_base/lang",
	"dojo/_base/array",
	"dojo/json"
], function(bc, fileUtils, fs, lang, array, json){
	return function(resource, callback){
		if(!bc.depsDumpDotFilename && !bc.depsDumpFilename){
			return 0;
		}

		var dotModules = 0, traceForDot = {}, traceForDotDone = {};
		if(bc.dotModules){
			dotModules = {};
			array.forEach(bc.dotModules.split(","), function(module){
				dotModules[lang.trim(module)] = traceForDot;
			});
		}

		var modules = [],
			midToId = {},
			i = 0,
			dotOutput = "digraph {\n",
			r, p, destFilename;
		for(p in bc.resources){
			r = bc.resources[p];
			if(r.deps){
				if(!dotModules || dotModules[r.mid]){
					dotModules[r.mid] = traceForDotDone;
					r.deps.forEach(function(module){
						dotOutput += '"' + r.mid + '" -> "' + module.mid + '";\n';
						if (dotModules[module.mid]!==traceForDotDone){
							dotModules[module.mid] = traceForDot;
						}
					});
				}
				r.uid = i;
				midToId[bc.resources[p].mid] = i;
				modules.push(r);
				i++;
			}
		}

		if(bc.depsDumpDotFilename){
			var foundOne = dotModules;
			while(foundOne){
				foundOne = false;
				for(p in bc.resources){
					r = bc.resources[p];
					if(dotModules[r.mid]==traceForDot){
						foundOne = true;
						dotModules[r.mid] = traceForDotDone;
						if(r.deps){
							r.deps.forEach(function(module){
								dotOutput += '"' + r.mid + '" -> "' + module.mid + '";\n';
								if (dotModules[module.mid]!==traceForDotDone){
									dotModules[module.mid] = traceForDot;
								}
							});
						}
					}
				}
			}
			dotOutput += "}\n";

			var filename = fileUtils.computePath(bc.depsDumpDotFilename, bc.destBasePath);
			fileUtils.ensureDirectory(fileUtils.getFilepath(filename));
			fs.writeFileSync(filename, dotOutput, "ascii");
		}

		if(bc.depsDumpFilename){
			var depsTree = modules.map(function(module){
					return module.deps.map(function(item){ return item.uid; });
				}),
				idTree = {},
				getItem = function(parts, bag){
					var part = parts.shift();
					if(!(part in bag)){
						bag[part] = {};
					}
					if(parts.length){
						return getItem(parts, bag[part]);
					}else{
						return bag[part];
					}
				};
			modules.forEach(function(item, i){
				var parts = item.mid.split("/");
				getItem(parts, idTree)["*"] = i;
			});

			filename = fileUtils.computePath(bc.depsDumpFilename, bc.destBasePath);
			fileUtils.ensureDirectory(fileUtils.getFilepath(filename));
			fs.writeFileSync(filename, json.stringify({depsTree:depsTree, idTree:idTree}), "ascii");
		}

		return 0;
	};
});
