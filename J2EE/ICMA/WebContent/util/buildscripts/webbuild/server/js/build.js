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
function load(/*String*/fileName){
	// summary:
	//		opens the file at fileName and evals the contents as JavaScript.
	
	//Read the file
	var fileContents = readFile(fileName);

	//Eval the contents.
	var Context = Packages.org.mozilla.javascript.Context;
	var context = Context.enter();
	try{
		return context.evaluateString(this, fileContents, fileName, 1, null);
	}finally{
		Context.exit();
	}
}

function readFile(/*String*/path, /*String?*/encoding){
	// summary:
	//		reads a file and returns a string
	encoding = encoding || "utf-8";
	var file = new java.io.File(path);
	var lineSeparator = "\n";
	var input = new java.io.BufferedReader(new java.io.InputStreamReader(new java.io.FileInputStream(file), encoding));
	try {
		var stringBuffer = new java.lang.StringBuffer();
		var line = "";
		while((line = input.readLine()) !== null){
			stringBuffer.append(line);
			stringBuffer.append(lineSeparator);
		}
		//Make sure we return a JavaScript string and not a Java string.
		return new String(stringBuffer.toString()); //String
	} finally {
		input.close();
	}
}

//TODO: inlining this function since the new shrinksafe.jar is used, and older
//versions of Dojo's buildscripts are not compatible.
function optimizeJs(/*String fileName*/fileName, /*String*/fileContents, /*String*/copyright, /*String*/optimizeType, /*String*/stripConsole){
	// summary:
	//		either strips comments from string or compresses it.
	copyright = copyright || "";

	//Use rhino to help do minifying/compressing.
	var context = Packages.org.mozilla.javascript.Context.enter();
	try{
		// Use the interpreter for interactive input (copied this from Main rhino class).
		context.setOptimizationLevel(-1);

		// the "packer" type is now just a synonym for shrinksafe
		if(optimizeType.indexOf("shrinksafe") == 0 || optimizeType == "packer"){
			//Apply compression using custom compression call in Dojo-modified rhino.
			fileContents = new String(Packages.org.dojotoolkit.shrinksafe.Compressor.compressScript(fileContents, 0, 1, stripConsole));
			if(optimizeType.indexOf(".keepLines") == -1){
				fileContents = fileContents.replace(/[\r\n]/g, "");
			}
		}else if(optimizeType == "comments"){
			//Strip comments
			var script = context.compileString(fileContents, fileName, 1, null);
			fileContents = new String(context.decompileScript(script, 0));
			
			//Replace the spaces with tabs.
			//Ideally do this in the pretty printer rhino code.
			fileContents = fileContents.replace(/    /g, "\t");

			//If this is an nls bundle, make sure it does not end in a ;
			//Otherwise, bad things happen.
			if(fileName.match(/\/nls\//)){
				fileContents = fileContents.replace(/;\s*$/, "");
			}
		}
	}finally{
		Packages.org.mozilla.javascript.Context.exit();
	}


	return copyright + fileContents;
}

build = {
	make: function(
		//The path to this file. Assumes dojo builds under it.
		/*String*/builderPath,
		
		//"1.1.1" or "1.3.2": used to choose directory of dojo to use.
		/*String*/version,
		
		//"google" or "aol"
		/*String*/cdnType,
		
		//comma-separated list of resource names. No double-quotes or quotes around values.
		/*String*/dependencies,
		
		//comments, shrinksafe, none
		/*String*/optimizeType){


		//Validate.
		if(version != "1.3.2"){
			return "invalid version";
		}
		if(cdnType != "google" && cdnType != "aol"){
			return "invalide CDN type";
		}
		if(optimizeType != "comments" && optimizeType != "shrinksafe"
			&& optimizeType != "none" && optimizeType != "shrinksafe.keepLines"){
			return "invalid optimize type";
		}
		if(!dependencies.match(/^[\w\-\,\s\.]+$/)){
			return "invalid dependency list";
		}
		
		//Set up full CDN path.
		var xdDojoPath = "http://ajax.googleapis.com/ajax/libs/dojo/";
		if(cdnType == "aol"){
			xdDojoPath = "http://o.aolcdn.com/dojo/";
		}
		xdDojoPath += version;

		//Directory that holds dojo source distro. Direct child under the helma dir
		var dojoDir = builderPath + version + "/";
		
		//Normalize the dependencies so that have double-quotes
		//around each dependency.
		var normalizedDependencies = dependencies || "";
		if(normalizedDependencies){
			normalizedDependencies = '"' + normalizedDependencies.split(",").join('","') + '"';
		}

		var buildscriptDir = dojoDir + "util/buildscripts/";
		
		//Load the libraries to help in the build.
		load(dojoDir + "util/buildscripts/jslib/logger.js");
		load(dojoDir + "util/buildscripts/jslib/fileUtil.js");
		load(dojoDir + "util/buildscripts/jslib/buildUtil.js");
		load(dojoDir + "util/buildscripts/jslib/buildUtilXd.js");
		load(dojoDir + "util/buildscripts/jslib/i18nUtil.js");
		
		//Set up the build args.
		var kwArgs = buildUtil.makeBuildOptions([
			"loader=xdomain",
			"version=" + version,
			"xdDojoPath=" + xdDojoPath,
			"layerOptimize=" + optimizeType
		]);
		
		//Specify the basic profile for build.
		var profileText = 'dependencies = {'
			+ 'layers: ['
			+ '	{'
			+ '		name: "dojo.js",'
			+ '		dependencies: ['
			+         normalizedDependencies
			+ '		]'
			+ '	}'
			+ '],'
		
			+ 'prefixes: ['
			+ '	[ "dojo", "' + dojoDir + 'dojo" ],'
			+ '	[ "dijit", "' + dojoDir + 'dijit" ],'
			+ '	[ "dojox", "' + dojoDir + 'dojox" ]'
			+ ']'
		+ '}';
		
		//Bring the profile into existence
		var profileProperties = buildUtil.evalProfile(profileText, true);
		kwArgs.profileProperties = profileProperties;
		
		//Set up some helper variables.
		dependencies = kwArgs.profileProperties.dependencies;
		var prefixes = dependencies.prefixes;
		var lineSeparator = fileUtil.getLineSeparator();
		var layerLegalText = fileUtil.readFile(buildscriptDir + "copyright.txt")
			+ lineSeparator
			+ fileUtil.readFile(buildscriptDir + "build_notice.txt");
		
		//Manually set the loader on the dependencies object. Ideally the buildUtil.loadDependencyList() function
		//and subfunctions would take kwArgs directly.
		dependencies.loader = kwArgs.loader;
		
		//Build the layer contents.
		var depResult = buildUtil.makeDojoJs(buildUtil.loadDependencyList(kwArgs.profileProperties, null, buildscriptDir), kwArgs.version, kwArgs);
		
		//Grab the content from the "dojo.xd.js" layer.
		var layerName = depResult[1].layerName;
		var layerContents = depResult[1].contents;
		
		//Burn in xd path for dojo if requested, and only do this in dojo.xd.js.
		if(layerName.match(/dojo\.xd\.js/) && kwArgs.xdDojoPath){
			layerContents = buildUtilXd.setXdDojoConfig(layerContents, kwArgs.xdDojoPath);
		}
		
		//Intern strings
		if(kwArgs.internStrings){
			prefixes = dependencies["prefixes"] || [];
			var skiplist = dependencies["internSkipList"] || [];
			layerContents = buildUtil.interningRegexpMagic(layerName, layerContents, dojoDir, prefixes, skiplist);
		}
		
		//Minify the contents
		return optimizeJs(layerName, layerContents, layerLegalText, kwArgs.layerOptimize, "");

	}
};
