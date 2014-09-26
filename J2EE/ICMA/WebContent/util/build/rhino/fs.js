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
define([], function() {
	var
		readFileSync = function(filename, encoding) {
			if (encoding=="utf8") {
				// convert node.js idiom to rhino idiom
				encoding = "utf-8";
			}
			return readFile(filename, encoding || "utf-8");
		},

		writeFileSync = function(filename, contents, encoding){
			var
				outFile = new java.io.File(filename),
				outWriter;
			if (encoding=="utf8") {
				// convert node.js idiom to java idiom
				encoding = "UTF-8";
			}
			if(encoding){
				outWriter = new java.io.OutputStreamWriter(new java.io.FileOutputStream(outFile), encoding);
			}else{
				outWriter = new java.io.OutputStreamWriter(new java.io.FileOutputStream(outFile));
			}

			var os = new java.io.BufferedWriter(outWriter);
			try{
				os.write(contents);
			}finally{
				os.close();
			}
		};

	return {
		statSync:function(filename) {
			return new java.io.File(filename);
		},

		mkdirSync:function(filename) {
			var dir = new java.io.File(filename);
			if (!dir.exists()) {
				dir.mkdirs();
			}
		},

		readFileSync:readFileSync,

		readdirSync:function(path) {
			// the item+"" is necessary because item is a java object that doesn't have the substring method
			return (new java.io.File(path)).listFiles().map(function(item){ return (item.name+""); });
		},



		readFile:function(filename, encoding, cb) {
			var result = readFileSync(filename, encoding);
			if (cb) {
				cb(0, result);
			}
		},

		writeFileSync:writeFileSync,

		writeFile:function(filename, contents, encoding, cb) {
			if (arguments.length==3 && typeof encoding!="string") {
				cb = encoding;
				encoding = 0;
			}
			writeFileSync(filename, contents, encoding);
			if (cb) {
				cb(0);
			};
		}
	};
});
