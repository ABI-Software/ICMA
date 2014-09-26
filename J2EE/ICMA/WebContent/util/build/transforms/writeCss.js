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
	"../fs"
], function(bc, fileUtils, fs){
	return function(resource, callback){
		var
			waitCount = 0,

			errors = [],

			onWriteComplete = function(err){
				if(err){
					errors.push(err);
				}
				if(--waitCount==0){
					callback(resource, errors.length && errors);
				}
			},

			doWrite = function(filename, text, encoding){
				fileUtils.ensureDirectoryByFilename(filename);
				waitCount++;
				fs.writeFile(filename, bc.newlineFilter(text, resource, "writeCss"), encoding || "utf8", onWriteComplete);
				// this must go *after* the async call
			},

			wroteExterns = 0;

		try{
			doWrite(resource.dest, resource.text);
			if(resource.compactDest!=resource.dest){
				doWrite(resource.compactDest, resource.compactText);
			}

			// only need to tranverse bc.destDirToExternSet once...
			if(wroteExterns){
				return callback;
			}
			wroteExterns = 1;
			// bc.destDirToExternSet is a map from dest directory name to resourceSet;
			// resourceSet is a map from src filename (complete with path) to dest filename (name only)
			// bc.destDirToExternSet[dir][src]= dest implies copy filename src to dir + "/" + dest
			var
				destDirToExternSet = bc.destDirToExternSet,
				dir, resourceSet, src;
			for(dir in destDirToExternSet){
				resourceSet = destDirToExternSet[dir];
				for(src in resourceSet){
					doWrite(dir + "/" + resourceSet[src], bc.resources[src].text, resource.encoding);
				}
			}
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
