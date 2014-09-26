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
define(["dojo/regexp"], function(dojoRegExp) {
	return {
		start:function(
			id,
			referenceModule,
			bc
		) {
			var
				getHasPluginDependency = function(){
					var hasPlugin = bc.amdResources["dojo/has"];
					if(!hasPlugin){
						bc.log("dojoHasMissingPlugin");
						return [];
					}else{
						return [hasPlugin];
					}
				},

				has = function(featureId) {
					var value = bc.staticHasFeatures[featureId];
					return (value===undefined || value==-1) ? undefined : value;
				},

				tokens = id.match(/[\?:]|[^:\?]*/g),

				i = 0,

				get = function(skip){
					var operator, term = tokens[i++];
					if(term == ":"){
						// empty string module name; therefore, no dependency
						return "";
					}else{
						// postfixed with a ? means it is a feature to branch on, the term is the name of the feature
						if(tokens[i++] == "?"){
							var hasResult = has(term);
							if(hasResult===undefined){
								return undefined;
							}else if(!skip && hasResult){
								// matched the feature, get the first value from the options
								return get();
							}else{
								// did not match, get the second value, passing over the first
								get(true);
								return get(skip);
							}
						}
						// a module
						return term===undefined ? "" : term;
					}
				},

				resolvedId = get();

			// we only need the plugin if we need to resolve at run time
			if(resolvedId===undefined){
				bc.log("dojoHasUnresolvedMid", ["plugin resource id", id, "reference module id", referenceModule && referenceModule.mid]);
				return getHasPluginDependency();
			}

			var regex = new RegExp("((dojo\\/)|([./]+))has\\!" + dojoRegExp.escapeString(id));
			if(!resolvedId){
				// replace the unneeded module with a module that's guaranteed available
				// this keeps the module order, and therefore, argument order to the factory correct
				referenceModule.text = referenceModule.text.replace(regex, "require");
				return [];
			}else{
				var
					moduleInfo = bc.getSrcModuleInfo(resolvedId, referenceModule),
					module = bc.amdResources[moduleInfo.mid];
				if(module){
					referenceModule.text = referenceModule.text.replace(regex, resolvedId);
					return [module];
				}else{
					bc.log("dojoHasMissingMid", ["plugin resource id", id, "resolved plugin resource id", moduleInfo.mid, "reference module id", referenceModule && referenceModule.mid]);
					return getHasPluginDependency();
				}
			}
		}
	};
});
