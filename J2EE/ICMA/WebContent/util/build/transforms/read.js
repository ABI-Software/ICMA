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
	"../replace"
], function(bc, fileUtils, fs, replace){
	var
		getFiletype = fileUtils.getFiletype,

		encodingMap=
			// map from file type to encoding
			(bc.transformConfig.read && bc.transformConfig.read.encoding) || {
				css:"utf8",
				html:"utf8",
				htm:"utf8",
				js:"utf8",
				json:"utf8",
				asc:"utf8",
				c:"utf8",
				cpp:"utf8",
				log:"utf8",
				conf:"utf8",
				text:"utf8",
				txt:"utf8",
				dtd:"utf8",
				xml:"utf8",
				png:undefined,
				jpg:undefined,
				jpeg:undefined,
				gif:undefined
			};

	return function(resource, callback){
		resource.getText = function(){
			if(!this.replacementsApplied){
				this.replacementsApplied = true;
				if(bc.replacements[this.src]){
					this.text = replace(this.text, bc.replacements[this.src]);
				}
			}
			return this.text;
		};

		resource.setText = function(text){
			resource.text = text;
			resource.getText = function(){ return this.text; };
			return text;
		};

		var filetype = getFiletype(resource.src, 1);
		// the expression is a little odd since undefined is a legitimate encodingMap value
		resource.encoding = resource.encoding ||(!(filetype in encodingMap) && "utf8") || encodingMap[filetype];
		fs.readFile(resource.src, resource.encoding, function(err, data){
			if(!err){
				resource.text = data;
			}
			callback(resource, err);
		});
		return callback;
	};
});
