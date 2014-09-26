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
//The functions in this file assume that buildUtil.js have been loaded.
var buildUtilXd = {};

buildUtilXd.setXdDojoConfig = function(/*String*/fileContents, /*String*/url){
	// summary:
	//		sets sets up xdomain loading for a particular URL.
	// fileContents:
	//		be a built dojo.js (can be uncompressed or compressed).
	// url:
	//		value should be the url to the directory that contains the dojo,
	//		dijit and dojox directories.
	//		Example: "http://some.domain.com/dojo090" (no ending slash)

	//This function will inject some contents after the dojo.registerModulePath() definition.
	//The contents of fileName should have been a dojo.js that includes the contents
	//of loader_xd.js (specify loader=xdomain in the build command).

	//This code is not very robust. It will break if dojo.registerModulePath definition
	//changes to anything more advanced.
	var match = fileContents.match(/(dojo\.registerModulePath\s*=\s*function.*\{)/);
	
	//Find the next two } braces and in inject code after that.
	var endIndex = fileContents.indexOf("}", match.index);
	endIndex = fileContents.indexOf("}", endIndex + 1);
	if(fileContents.charAt(endIndex + 1) == ";"){
		endIndex += 1;
	}
	endIndex +=1;

	var lineSeparator = fileUtil.getLineSeparator();
	return fileContents.substring(0, endIndex)
		+ lineSeparator
		+ "if(typeof dojo.config[\"useXDomain\"] == \"undefined\"){"
		+ "dojo.config.useXDomain = true;};\ndojo.registerModulePath(\"dojo\", \""
		+ url + "/dojo"
		+ "\");\ndojo.registerModulePath(\"dijit\", \""
		+ url + "/dijit"
		+ "\");\ndojo.registerModulePath(\"dojox\", \""
		+ url + "/dojox"
		+ "\");"
		+ lineSeparator
		+ fileContents.substring(endIndex, fileContents.length);
}

buildUtilXd.xdgen = function(
	/*String*/prefixName,
	/*String*/prefixPath,
	/*Array*/prefixes,
	/*RegExp*/optimizeIgnoreRegExp,
	/*Object*/kwArgs
){
	// summary:
	//		generates the .xd.js files for a build.
	var jsFileNames = fileUtil.getFilteredFileList(prefixPath, /\.js$/, true);
	
	//Construct a regexp to avoid xd generating loader_xd.js, since shrinksafe
	//does not like the resulting javascript that is generated, because of
	//bug http://trac.dojotoolkit.org/ticket/2766
	var loaderIgnoreRegExp = /dojo\/_base\/_loader\/loader_xd/;

	for(var i = 0; i < jsFileNames.length; i++){
		var jsFileName = jsFileNames[i];

		//Some files, like the layer files, have already been xd
		//processed, so be sure to skip those.
		if(!jsFileName.match(optimizeIgnoreRegExp) && !jsFileName.match(loaderIgnoreRegExp)){
			var xdFileName = jsFileName.replace(/\.js$/, ".xd.js");
			var fileContents = fileUtil.readFile(jsFileName);

			//Files in nls directories, except for the ones that have multiple
			//bundles flattened (therefore have a dojo.provide call),
			//need to have special xd contents.
			if(jsFileName.match(/\/nls\//) && fileContents.indexOf("dojo.provide(") == -1){
				var xdContents = buildUtilXd.makeXdBundleContents(prefixName, prefixPath, jsFileName, fileContents, prefixes, kwArgs);
			}else{
				xdContents = buildUtilXd.makeXdContents(fileContents, prefixes, kwArgs);
			}
			fileUtil.saveUtf8File(xdFileName, xdContents);
		}
	}
}

//Function that generates the XD version of the module file's contents
buildUtilXd.makeXdContents = function(fileContents, prefixes, kwArgs){
	var dependencies = [];

	//Use the regexps to find resource dependencies for this module file.
	var depMatches = buildUtil.removeComments(fileContents).match(buildUtil.globalDependencyRegExp);
	if(depMatches){
		for(var i = 0; i < depMatches.length; i++){
			var partMatches = depMatches[i].match(buildUtil.dependencyPartsRegExp);
			var depCall = partMatches[1];
			var depArgs = partMatches[2];

			if(depCall == "requireLocalization"){
				//Need to find out what locales are available so the dojo loader
				//only has to do one script request for the closest matching locale.
				var reqArgs = i18nUtil.getRequireLocalizationArgsFromString(depArgs);
				if(reqArgs.moduleName){
					//Find the list of locales supported by looking at the path names.
					var locales = i18nUtil.getLocalesForBundle(reqArgs.moduleName, reqArgs.bundleName, prefixes);

					//Add the supported locales to the requireLocalization arguments.
					if(!reqArgs.localeName){
						depArgs += ", null";
					}

					depCall = "requireLocalization";
					depArgs += ', "' + locales.join(",") + '"';
				}else{
					//Malformed requireLocalization call. Skip it. May be a comment.
					continue;
				}
			}
	
			dependencies.push('"' + depCall + '", ' + depArgs);
		}
	}

	//Build the xd file contents.
	var xdContentsBuffer = [];
	var scopeArgs = kwArgs.xdScopeArgs || "dojo, dijit, dojox";

	//Start the module function wrapper.
	xdContentsBuffer.push((kwArgs.xdDojoScopeName || "dojo") + "._xdResourceLoaded(function(" + scopeArgs + "){\n");

	//See if there are any dojo.loadInit calls
	var loadInitCalls = buildUtilXd.extractLoadInits(fileContents);
	if(loadInitCalls){
		//Adjust fileContents since extractLoadInits removed something.
		fileContents = loadInitCalls[0];

		//Add any loadInit calls to an array passed _xdResourceLoaded
		for(i = 1; i < loadInitCalls.length; i++){
			xdContentsBuffer.push(loadInitCalls[i] + ";\n");
		}
	}

	xdContentsBuffer.push("return {");

	//Add in dependencies section.
	if(dependencies.length > 0){
		xdContentsBuffer.push("depends: [");
		for(i = 0; i < dependencies.length; i++){
			if(i > 0){
				xdContentsBuffer.push(",\n");
			}
			xdContentsBuffer.push("[" + dependencies[i] + "]");
		}
		xdContentsBuffer.push("],");
	}
	
	//Add the contents of the file inside a function.
	//Pass in module names to allow for multiple versions of modules in a page.
	xdContentsBuffer.push("\ndefineResource: function(" + scopeArgs + "){");
	//Remove requireLocalization calls, since that will mess things up.
	//String() part is needed since fileContents is a Java object.
	xdContentsBuffer.push(String(fileContents).replace(/dojo\.(requireLocalization|i18n\._preloadLocalizations)\([^\)]*\)/g, ""));
	xdContentsBuffer.push("\n}};});");

	return xdContentsBuffer.join("");
}


buildUtilXd.makeXdBundleContents = function(prefix, prefixPath, srcFileName, fileContents, prefixes, kwArgs){
	//logger.info("Flattening bundle: " + srcFileName);

	var bundleParts = i18nUtil.getBundlePartsFromFileName(prefix, prefixPath, srcFileName);
	if(!bundleParts){
		return null;
	}
	var moduleName = bundleParts.moduleName;
	var bundleName = bundleParts.bundleName;
	var localeName = bundleParts.localeName;
	
	//logger.trace("## moduleName: " + moduleName + ", bundleName: " + bundleName + ", localeName: " + localeName);
	
	//If this is a dojo bundle, it will have already been flattened via the normal build process.
	//If it is an external bundle, then we didn't flatten it during the normal build process since
	//right now, we don't make copies of the external module source files. Need to figure that out at some
	//point, but for now, need to get flattened contents for external modules.
	fileContents = (prefix.indexOf("dojo") == 0) ? fileContents : i18nUtil.makeFlatBundleContents(prefix, prefixPath, srcFileName);

	//Final XD file contents.
	fileContents = 'dojo.provide("' + moduleName + '.nls.' + (localeName ? localeName + '.' : '') + bundleName + '");'
	                 + 'dojo._xdLoadFlattenedBundle("' + moduleName + '", "' + bundleName
                   + '", "' + localeName + '", ' + fileContents + ');';

	//Now make a proper xd.js file out of the content.
	return buildUtilXd.makeXdContents(fileContents, prefixes, kwArgs);
}

buildUtilXd.loadInitRegExp = /dojo\.loadInit\s*\(/g;
buildUtilXd.extractLoadInits = function(/*String*/fileContents){
	return buildUtil.extractMatchedParens(buildUtilXd.loadInitRegExp, fileContents);
}
