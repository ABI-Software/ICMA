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
define([], function(){
	function Generate(domNode){
		return {
			layout: function(layout, rule){
				return function(domNode){
					for(var i = 0; i < layout.length; i++){
						var rule = layout[i];
						var selector = rule.selector;
						var target = document.createElement("div");
						// create the appropriate additions to the elements based on the selectors
						selector.replace(/\.([\w-]+)/, function(t, className){
							target.className = className;
						});
						selector.replace(/#([\w-]+)/, function(t, id){
							target.id = id;
						});
						rule.apply(target);
						domNode.appendChild(target);
					}
				}
			},
			content: function(content, rule){
				return function(domNode){
					domNode.innerHTML = eval(content);
				}
			},
			role: "layout"
		};
	}
	var def = new Generate({});
	Generate.layout = def.layout;
	Generate.content = def.content;
	Generate.role = def.role;
	return Generate; 
})