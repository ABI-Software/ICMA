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
define(["./fs"], function(fs){
	var cached = {};

	return function(contents, replacement){
		var encoding = "utf8";
		if(replacement instanceof Array){
			// replacement is a vector of replacement instructions; maybe the first item is an encoding
			if(typeof replacement[0]=="string"){
				encoding = replacement[0];
				replacement = replacement.slice(1);
			}
		}else{
			// replacement is a single replacement [search, replacement, type] triple
			replacement = [replacement];
		}
		// at this point, encoding has been determined and replacement is a vector of [search, replacement, type] triples

		replacement.forEach(function(item){
			var
				searchText = item[0],
				replacementText = item[1],
				type = item[2];
			if(type=="file"){
				// replacementText gives a filename that holds the replacement text
				// TODO add type AMD module
				replacementText = (cached[filename] = cached[filename] || fs.readFileSynch(replacementText, encoding));
			}
			if(searchText instanceof RegExp){
				contents = contents.replace(searchText, replacementText);
			}else if(typeof searchText=="function"){
				contents = searchText(contents);
			}else{
				// replace all occurences of searchText with replacementText
				var
					searchTextLength = searchText.length,
					replacementTextLength = replacementText.length,
					start = contents.indexOf(searchText);
				while(start!=-1){
					contents = contents.substring(0, start) + replacementText + contents.substring(start + searchTextLength);
					start = contents.indexOf(searchText, start + replacementTextLength);
				}
			}
		});
		return contents;
	};
});

