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
define(["require"], function(moduleRequire){
"use strict";
var cssCache = window.cssCache || (window.cssCache = {});
/*
 * RequireJS css! plugin
 * This plugin will load and wait for css files.  This could be handy when
 * loading css files as part of a layer or as a way to apply a run-time theme. This
 * module checks to see if the CSS is already loaded before incurring the cost
 * of loading the full CSS loader codebase
 */
/* 	function search(tag, href){
		var elements = document.getElementsByTagName(tag);
		for(var i = 0; i < elements.length; i++){
			var element = elements[i];
			var sheet = alreadyLoaded(element.sheet || element.styleSheet, href)
			if(sheet){
				return sheet;
			}
		}
	}
	function alreadyLoaded(sheet, href){
		if(sheet){
			var importRules = sheet.imports || sheet.rules || sheet.cssRules;
			for(var i = 0; i < importRules.length; i++){								
				var importRule = importRules[i];
				if(importRule.href){
					sheet = importRule.styleSheet || importRule;
					if(importRule.href == href){
						return sheet;
					}
					sheet = alreadyLoaded(sheet, href);
					if(sheet){
						return sheet;
					}
				}
			}
		}
	}
	function nameWithExt (name, defaultExt) {
		return name.lastIndexOf('.') <= name.lastIndexOf('/') ?
			name + '.' + defaultExt : name;
	}*/
 	return {
		load: function (resourceDef, require, callback, config) {
			var url = require.toUrl(resourceDef);
			if(cssCache[url]){
				return createStyleSheet(cssCache[url]);
			}
/*			var cssIdTest = resourceDef.match(/(.+)\?(.+)/);
			if(cssIdTest){*/
				// if there is an id test available, see if the referenced rule is already loaded,
				// and if so we can completely avoid any dynamic CSS loading. If it is
				// not present, we need to use the dynamic CSS loader.
				var docElement = document.documentElement;
				var testDiv = docElement.insertBefore(document.createElement('div'), docElement.firstChild);
				testDiv.id = require.toAbsMid(resourceDef).replace(/\//g,'-').replace(/\..*/,'') + "-loaded";  //cssIdTest[2];
				var displayStyle = (testDiv.currentStyle || getComputedStyle(testDiv, null)).display;
				docElement.removeChild(testDiv);
				if(displayStyle == "none"){
					return callback();
				}
				//resourceDef = cssIdTest[1];
			//}
			// use dynamic loader
			/*if(search("link", url) || search("style", url)){
				callback();
			}else{*/
			moduleRequire(["./load-css"], function(load){
				load(url, callback);
			});
		},
		pluginBuilder: "xstyle/css-builder"

	};
});
