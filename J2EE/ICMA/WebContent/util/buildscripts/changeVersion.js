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
// Changes the Dojo version in a file. Used during the release process.

var
	version = new String(arguments[0]),
	revision = new String(arguments[1]),
	filename = new String(arguments[2]),

	writeFile= function(filename, contents){
		var
			outFile = new java.io.File(filename),
			outWriter;
		outWriter = new java.io.OutputStreamWriter(new java.io.FileOutputStream(outFile), "UTF-8");
		var os = new java.io.BufferedWriter(outWriter);
		try{
			os.write(contents);
		}finally{
			os.close();
		}
	},


	changeVersion = function(/*String*/version, /*String*/fileContents){
		// summary:
		//		Changes the version number for dojo. Input should be the fileContents
		//		of a file that contains the version number.

		//Set version number.
		//First, break apart the version string.
		var verSegments = version.match(/^(\d*)\.?(\d*)\.?(\d*)\.?(.*)$/);
		var majorValue = verSegments[1] || 0;
		var minorValue = verSegments[2] || 0;
		var patchValue = verSegments[3] || 0;
		var flagValue  = verSegments[4] || "";

		// Do the final version replacement.
		if(/package/.test(filename)){
			fileContents = fileContents.replace(
				/['"](version|dojo|dijit)['"]\s*\:\s*['"][\w\.\-]+?["']/g,
				'"$1":"' + version + '"'
			);
		}else{
			fileContents = fileContents.replace(
				/major:\s*\d*,\s*minor:\s*\d*,\s*patch:\s*\d*,\s*flag:\s*".*?"\s*,/g,
				"major: " + majorValue + ", minor: " + minorValue + ", patch: " + patchValue + ", flag: \"" + flagValue + "\","
			);
			fileContents = fileContents.replace(/\$Rev(: \d+ |)\$/, "$Rev: " + revision + " $");
		}

		return fileContents; //String
	};

print(version);
print(filename);
var fileContents = readFile(filename, "utf-8");
fileContents = changeVersion(version, fileContents);
writeFile(filename, fileContents);
