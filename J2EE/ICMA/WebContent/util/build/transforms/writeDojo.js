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
	"../stringify",
	"./writeAmd",
	"../process",
	"dojo/json",
	"dojo/text!./dojoBoot.js"
], function(bc, fileUtils, fs, stringify, writeAmd, process, json, dojoBootText){
	return function(resource, callback){
		var
			getUserConfig = function(){
				if(!bc.userConfig){
					return "this.dojoConfig || this.djConfig || this.require || {}";
				}
				if(typeof bc.userConfig == "string"){
					return bc.userConfig;
				}
				var result = stringify(bc.userConfig);
				if(result.unsolved){
					bc.log("configUnresolvedValues");
				}
				return result;
			},

			computeLocation = function(basePath, path){
				if(path.indexOf(basePath + "/")==0){
					return "." + path.substring(basePath.length);
				}
				var
					parts = basePath.split("/"),
					prefix = "";
				for(var i = parts.length-1; i>=0; i--){
					 prefix+= (prefix ? "/.." : "..");
					var check = parts.slice(0, i).join("/") + "/";
					if(path.indexOf(check)==0){
						return prefix + path.substring(check.length-1);
					}
				}
				return path;
			},

			getPackage = function(name){
				// the purpose of this somewhat verbose routine is to write a minimal package object for each
				// package, yet allow client code to pass extra (i.e., outside the scope of CJS specs) config
				// information within the package object
				var destPack = bc.destPackages[name],
					result = {};
				result.name = destPack.name;
				if(destPack.main!="main"){
					result.main = destPack.main;
				}
				// everything relative to the dojo dir
				// TODO: making everything relative to dojo needs to be optional
				if(name=="dojo"){
					result.location = ".";
				}else{
					result.location = computeLocation(bc.destBasePath + "/dojo", destPack.location);
				}
				var packageDefaultConfig = bc.defaultConfig && bc.defaultConfig.packages && bc.defaultConfig.packages[name];
				for(var p in packageDefaultConfig){
					result[p] = packageDefaultConfig[p];
				}
				return result;
			},

			getDefaultConfig = function(){
				var p, config = {packages:[], hasCache:{}};
				if(bc.baseUrl){
					config.baseUrl = bc.baseUrl;
				}
				for(p in bc.packages){
					config.packages.push(getPackage(p));
				}
				for(p in bc.defaultConfig){
					if(p!=="packages"){
						// per-package default config was handled above
						config[p] = bc.defaultConfig[p];
					}
				}
				config = stringify(config);
				if(config.unsolved){
					bc.log("configUnresolvedValues");
				}
				return config;
			},

			stampVersion = function(text){
				// summary:
				//		Changes the version number for dojo. Input should be the fileContents
				//		of a file that contains the version number.
				version = bc.version;
				if(version){
					//First, break apart the version string.
					var verSegments = (version+"").match(/^(\d*)\.?(\d*)\.?(\d*)\.?(.*)$/);
					var majorValue = verSegments[1] || 0;
					var minorValue = verSegments[2] || 0;
					var patchValue = verSegments[3] || 0;
					var flagValue  = verSegments[4] || "";
					//Do the final version replacement.
					return text.replace(
							/major:\s*\d*,\s*minor:\s*\d*,\s*patch:\s*\d*,\s*flag:\s*".*?"\s*,/g,
						"major: " + majorValue + ", minor: " + minorValue + ", patch: " + patchValue + ", flag: \"" + flagValue + "\","
					);
				}else{
					return text;
				}
			},

			waitCount = 1, // matches *1*

			errors = [],

			onWriteComplete = function(err){
				if(err){
					errors.push(err);
				}
				if(--waitCount==0){
					callback(resource, errors.length && errors);
				}
			},

			doWrite = function(filename, text){
				fileUtils.ensureDirectoryByFilename(filename);
				waitCount++;
				fs.writeFile(filename, bc.newlineFilter(text, resource, "writeDojo"), "utf8", onWriteComplete);
			};

		// the writeDojo transform...
		try{
			var
				loaderText = resource.getText(),

				// the default application to the loader constructor is replaced with purpose-build user and default config values
				configText = "(" + getUserConfig() + ", " + getDefaultConfig() + ");",

				// the construction of the layer is slightly different than standard, so don't pass a module to getLayerText
				dojoLayerText = stampVersion(writeAmd.getLayerText(resource, "")),

				// 1.6 compat chunk
				dojoLayerCompat = (resource.layer.compat=="1.6" && resource.layer.include.length) ? "require(" + json.stringify(resource.layer.include) + ");" + bc.newline : "";

			// assemble and write the dojo layer
			resource.layerText = loaderText + configText + dojoLayerText + (bc.dojoBootText || dojoBootText) + dojoLayerCompat;
			doWrite(writeAmd.getDestFilename(resource), resource.layer.copyright + resource.layerText);

			//write any bootstraps; boots is a vector of resources that have been marked as bootable by the discovery process
			resource.boots.forEach(function(item){
				// don't process the dojo layer, which is === resource
				if(item!==resource){
					// each item is a hash of include, exclude, boot, bootText
					var compat = (item.layer.compat=="1.6" && item.layer.include.length) ? "require(" + json.stringify(item.layer.include) + ");" + bc.newline : "";
					item.layerText= loaderText + configText + dojoLayerText + writeAmd.getLayerText(item, false) + (item.bootText || bc.dojoBootText || dojoBootText) + dojoLayerCompat + compat;
					doWrite(writeAmd.getDestFilename(item), resource.layer.copyright + item.layerText);
				}
			});
			onWriteComplete(0); // matches *1*
		}catch(e){
			if(waitCount){
				// can't return the error since there are async processes already going
				errors.push(e);
				return 0;
			}else{
				return e;
			}
		}
		return callback;
	};
});
