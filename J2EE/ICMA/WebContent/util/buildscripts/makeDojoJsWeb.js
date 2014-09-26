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
//START of the "main" part of the script.
//This is the entry point for this script file.
load("buildUtil.js");
load("buildUtilXd.js");

var result = "ERROR";

var buildDirectory = new String(arguments[0]);
var depList = new String(arguments[1]);
var provideList = new String(arguments[2]);
var version = new String(arguments[3]);
var xdDojoUrl = new String(arguments[4]);

depList = depList.split(",");

//Check if there were no provideList (the caller can send string "null"
//to indicate that the command line parameter is empty. We need some string
//to make sure all the arguments are in the right spot.
if(provideList == "null"){
	provideList = "[]";
}else{
	provideList = provideList.split(",");
	provideList = '["' + provideList.join('","') + '"]';
}

var dependencyResult;
eval('dependencyResult = {depList: ["' + depList.join('","') + '"], provideList: ' + provideList + '};');

//Make sure we are dealing with JS files and that the paths are not outside
//of the working area. Do this to discourage fetching arbitrary files from
//the server.
var deps = dependencyResult.depList;
var isInputOk = true;
for(var i = 0; i < deps.length; i++){
	var matches = deps[i].match(/\.\./g);
	if((matches && matches.length > 1) || !deps[i].match(/\.js$/) || deps[i].indexOf(0) == '/'){
		print("Error: Invalid file set.");
		isInputOk = false;
		break;
	}
}

if(isInputOk){
	//Load dojo (needed for string interning)
	djConfig={
		baseRelativePath: "../"
	};
	load('../dojo.js');
	dojo.require("dojo.string.extras");
	dojo.require("dojo.crypto.MD5");

	var buildSigDir = dojo.crypto.MD5.compute(depList.sort().join(","), dojo.crypto.outputTypes.Hex);
	try{
		//xxx createLayerContents is broken: need kwargs? it is optional but probably should provide it.
		var contents = buildUtil.createLayerContents(dependencyResult.depList, dependencyResult.provideList, version);
		var compressedContents = "";
		var prefixes = [["dojo", "src"]];
	
		//Make sure any dojo.requireLocalization calls are modified
		//so that they inform the loader of valid locales that can be loaded.
		contents = buildUtil.modifyRequireLocalization(contents, prefixes);

		//Intern strings.
		contents = buildUtil.interningRegexpMagic("xdomain", contents, djConfig.baseRelativePath, prefixes, [], true);
		
		//Set the xdomain dojo url
		if(xdDojoUrl){
			contents = buildUtilXd.setXdDojoConfig(contents, xdDojoUrl);
		}

		//Compress code.
		compressedContents = buildUtil.optimizeJs("dojo.js", contents, "", "shrinksafe");

		//Add copyright
		var copyright = fileUtil.readFile("copyright.txt");
		var buildNotice = fileUtil.readFile("build_notice.txt");
		contents = copyright + buildNotice + contents;
		compressedContents = copyright + buildNotice + compressedContents;
		
		//Prep cache location
		var buildCachePath = buildDirectory + "/" + buildSigDir + "/";

		//Create needed directories for cache location.
		var dirFile = new java.io.File(buildCachePath + "compressed");
		var dirsOk = dirFile.mkdirs();

		result = "dirFile: " + dirFile.getAbsolutePath() + ", dirsOK: " + dirsOk;
		
		//Create build contents file
		var buildContents = dependencyResult.provideList.sort().join("\n");

		//Save files to disk
		fileUtil.saveUtf8File(buildCachePath + "dojo.js", contents);
		fileUtil.saveUtf8File(buildCachePath + "compressed/dojo.js", compressedContents);
		fileUtil.saveUtf8File(buildCachePath + "build.txt", buildContents);
		
		result = "OK";
	}catch(e){
		result = "ERROR: " + e;
	}

	print(result);
}
