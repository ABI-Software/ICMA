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
define(["dojo/json", "../fs"], function(json, fs){
	return {
		start:function(
			mid,
			referenceModule,
			bc
		){
			// mid may contain a pragma (e.g. "!strip"); remove
			mid = mid.split("!")[0];

			var textPlugin = bc.amdResources["dojo/text"],
			moduleInfo = bc.getSrcModuleInfo(mid, referenceModule, true),
			textResource = bc.resources[moduleInfo.url];

			if (!textPlugin){
				throw new Error("text! plugin missing");
			}
			if (!textResource){
				throw new Error("text resource (" + moduleInfo.url + ") missing");
			}

			var result = [textPlugin];
			if(bc.internStrings && !bc.internSkip(moduleInfo.mid, referenceModule)){
				result.push({
					module:textResource,
					pid:moduleInfo.pid,
					mid:moduleInfo.mid,
					deps:[],
					getText:function(){
						var text = this.module.getText ? this.module.getText() : this.module.text;
						if(text===undefined){
							// the module likely did not go through the read transform; therefore, just read it manually
							text= fs.readFileSync(this.module.src, "utf8");
						}
						return json.stringify(text+"");
					},
					internStrings:function(){
						return ["url:" + this.mid, this.getText()];
					}
				});
			}
			return result;
		}
	};
});
