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
	"./fs",
	"./buildControlBase",
	"dojo/has"
], function(fs, bc, has){
	var
		getFilename = function(filename){
			if(/\//.test(filename)){
				return filename.match(/^.*\/([^\/]+)$/)[1];
			}
			return filename;
		},

		getFilepath = function(filename){
			if(/\//.test(filename)){
				var result = filename.match(/^(.*)\/[^\/]+$/)[1];
				// if result=="", then must have been something like "/someFile"
				return result.length ? result : "/";
			}
			return "";
		},

		getFiletype = function(filename, trimDot){
			var match = filename.match(/(\.([^\/]*))$/);
			return (match && (trimDot ? match[2] : match[1])) || "";
		},

		cleanupPath = function(path){
			// change any falsy to ""
			path = path || "";

			// change all backslashes to forward slashes for those with bad habits from windows
			path = path.replace(/\\/g, "/");

			// remove any trailing "/" to be less sensitive to careless user input
			// but remember "/" is not a trailing slash--it's the root
			if(path.length>1 && path.charAt(path.length-1)=="/"){
				path = path.substring(0, path.length-1);
			}
			return path;
		},

		catPath = function(lhs, rhs){
			if(arguments.length>2){
				for(var args = [], i = 1; i<arguments.length; args.push(arguments[i++]));
				return catPath(lhs, catPath.apply(this, args));
			}else if(!rhs || !rhs.length){
				return lhs;
			}else if(!lhs || !lhs.length){
				return rhs;
			}else{
				return (lhs + "/" + rhs).replace(/\/\/\/?/g, "/");
			}
		},

		compactPath = function(path){
			var result = [],
				segment, lastSegment;
			path = path.replace(/\\/g, '/').split('/');
			while(path.length){
				segment = path.shift();
				if(segment==".." && result.length && lastSegment!=".."){
					result.pop();
					lastSegment = result[result.length - 1];
				}else if(segment!="."){
					result.push(lastSegment = segment);
				}// else ignore "."
			}
			return result.join("/");
		},

		isAbsolutePathRe = has("is-windows") ?
			// for windows, starts with "\\" or a drive designator (anything other than "/" or "\" followed by a ":")
			/^((\\\\)|([^\/\\]+\:))/ :
			// for unix, starts with "/"
			/^\//,

		isAbsolutePath = function(path){
			return path && path.length && isAbsolutePathRe.test(path);
		},

		normalize = function(filename){
			return has("is-windows") ? filename.replace(/\//g, "\\") : filename;
		},

		getAbsolutePath = function(src, base){
			src = cleanupPath(src);
			if(!isAbsolutePath(src)){
				src = catPath(base, src);
			}
			return compactPath(src);
		},

		computePath = function(path, base){
			path = cleanupPath(path);
			return compactPath(isAbsolutePath(path) ? path : catPath(base, path));
		},

		getTimestamp = function(ts){
			var f = function(i){ return "-" + (i<10 ? "0" + i : i); };
			return ts.getFullYear() + f(ts.getMonth()+1) + f(ts.getDate()) + f(ts.getHours()) + f(ts.getMinutes()) + f(ts.getSeconds());
		},

		dirExists = function(
			filename
		){
			try{
				return fs.statSync(filename).isDirectory();
			}catch(e){
				return false;
			}
		},

		fileExists = function(
			filename
		){
			try{
				return fs.statSync(filename).isFile();
			}catch(e){
				return false;
			}
		},

		checkedDirectories = {},

		clearCheckedDirectoriesCache = function(){
			checkedDirectories = {};
		},
		ensureDirectory = function(path){
			if(!checkedDirectories[path]){
				if(!dirExists(path)){
					ensureDirectory(getFilepath(path));
					try{
						fs.mkdirSync(path, 0755);
					}catch(e){
						//squelch
					}
				}
				checkedDirectories[path] = 1;
			}
		},

		ensureDirectoryByFilename = function(filename){
			ensureDirectory(getFilepath(filename));
		},

		readAndEval = function(filename, type){
			try{
				if(fileExists(filename)){
					return eval("(" + fs.readFileSync(filename, "utf8") + ")");
				}
			}catch(e){
				bc.log("failedReadAndEval", ["filename", filename, "type", type, "error", e]);
			}
			return {};
		},

		maybeRead = function(filename){
			try{
				if(fileExists(filename)){
					return fs.readFileSync(filename, "utf8");
				}
			}catch(e){
			}
			return 0;
		};


	return {
		getFilename:getFilename,
		getFilepath:getFilepath,
		getFiletype:getFiletype,
		cleanupPath:cleanupPath,
		isAbsolutePath:isAbsolutePath,
		normalize:normalize,
		getAbsolutePath:getAbsolutePath,
		catPath:catPath,
		compactPath:compactPath,
		computePath:computePath,
		getTimestamp:getTimestamp,
		dirExists:dirExists,
		ensureDirectory:ensureDirectory,
		ensureDirectoryByFilename:ensureDirectoryByFilename,
		clearCheckedDirectoriesCache:clearCheckedDirectoriesCache,
		readAndEval:readAndEval,
		maybeRead:maybeRead,
		fileExists:fileExists
	};
});
