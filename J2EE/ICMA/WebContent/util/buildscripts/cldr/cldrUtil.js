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
(function(){
	// monkey patch fromJson to avoid Rhino bug in eval: https://bugzilla.mozilla.org/show_bug.cgi?id=471005
	var fromJson = dojo.fromJson;
	dojo.fromJson = function(json){
		json = json.replace(/[\u200E\u200F\u202A-\u202E]/g, function(match){
			return "\\u" + match.charCodeAt(0).toString(16);
		})
		return json ? fromJson(json) : ""; //TODO: json value passed in shouldn't be empty
	}
})();

function isLocaleAliasSrc(prop, bundle){
	if(!bundle){ return false; }
	var isAlias = false;
	var LOCALE_ALIAS_MARK = '@localeAlias';

	for(x in bundle){
		if(x.indexOf(LOCALE_ALIAS_MARK) > 0){
			var prefix = x.substring(0,x.indexOf(LOCALE_ALIAS_MARK));
			if(prop.indexOf(prefix) == 0){
				isAlias = true;
			}
		}
	}
	return isAlias;
}

function getNativeBundle(filePath){
	// summary:
	//		get native bundle content with utf-8 encoding.
	//		native means the content of this bundle is not flattened with parent.
	//		returns empty object if file not found.
	try{
		var content = readFile(filePath, "utf-8");
		return (!content || !content.length) ? {} : dojo.fromJson(content);
	}catch(e){
		return {};
	}
}

function compare(a/*String or Array*/, b/*String or Array*/){
	// summary:
	//		simple comparison
	if(dojo.isArray(a) && dojo.isArray(b)){
		for(var i = 0; i < a.length; i++){
			if(a[i] != b[i]){ return false; }
		}
		return true;
	}
	return a==b;
}
