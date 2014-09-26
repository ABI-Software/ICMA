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
	"../fs",
	"../fileUtils",
	"dojo/_base/lang",
	"dojo/_base/json"
], function(bc, fs, fileUtils, lang, json){
	return function(resource){
		var
			// Array of MIDs discovered in the declarative resource
			aggregateDeps = [],

			// Looks for "data-dojo-type" and "data-dojo-mids" for declarative modules
			interningAutoRequireRegExp = /\sdata-dojo-(?:type|mids)\s*=\s*["']([^"']+\/[^"']+)["']/gi,

			// Looks for '<script type="dojo/require">' blocks
			interningDeclarativeRequireRegExp = /<script\s+[^>]*type=["']dojo\/require["'][^>]*>([^<]*)<\/script>/gi,

			processAutoRequire = function(){
				// Parses the resource for any MIDs that might need to be built into the layer
				var mids = [],
					str = resource.text,
					match;

				// Run the RegExp over the text
				while(match = interningAutoRequireRegExp.exec(str)){
					match[1].split(/\s*,\s*/).forEach(function(mid){
						mids.push(mid);
					});
				}

				return mids;
			},

			processDeclarativeRequire = function(){
				// Parses the resource for and declarative require script blocks and
				// analyses the bocks for their MIDs to be included in the layer.
				var mids = [],
					str = resource.text,
					match;

				while(match = interningDeclarativeRequireRegExp.exec(str)){
					// Try and convert <script type="dojo/require"> into object
					try{
						var h = json.fromJson("{" + match[1] + "}");
					}catch(e){
						bc.log("declarativeRequireFailed", ["resource", resource.src, "error", e]);
					}
					// Cycle through each key and add module as dependency
					for(var i in h){
						var mid = h[i];
						if(typeof mid == "string"){
							mids.push(mid);
						}else{
							bc.log("userWarn", ["declarative require has invalid value", "resource", resource.src, "key", i, "value", mid]);
						}
					}
				}

				return mids;
			};

		// Intern the resources strings and identify any mids used in declarative syntax
		aggregateDeps = aggregateDeps.concat(processAutoRequire());
		
		// Intern the resources strings and identify any declarative require script blocs and parse out the mids
		aggregateDeps = aggregateDeps.concat(processDeclarativeRequire());


		// Iterate through the layers, identify those that contain this resource.mid, 
		// remove it from the include array and then add this resource's includes
		for(var mid in bc.amdResources){
			if(bc.amdResources[mid].layer){ // This resource is a layer
				var includes = bc.amdResources[mid].layer.include,
					idx = includes.indexOf(resource.mid);
				// Bitwise operator that returns true if the layer contains this resource
				if(~idx){
					// Remove this resources mid from the layer's include array
					includes.splice(idx, 1);
					aggregateDeps.forEach(function(dep){
						// Uniquely add appropriate mids to the layer's include array
						if(!(/^(require|exports|module)$/.test(dep))){
							if(!~includes.indexOf(dep)){
								includes.push(dep);
							}
						}
					});
				}
			}
		}

	};
});