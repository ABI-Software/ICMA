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
	"dojo/_base/declare",
	"dojo/_base/lang",
	"dojo/window",
	"dojo/dom-construct",
	"dijit/Dialog",
	"dijit/form/Button",
	"dijit/ProgressBar"
], function(declare, lang, win, domConstruct, Dialog, Button,ProgressBar){

	// module:
	//		ABI/lib/Widgets/ProgressIndicator

	return declare("ABI.lib.Widgets.ProgressIndicator", null,{
		// summary:
		//		An alert dialog
		maximum : 1.0,
		headline: "",
		constructor: function(params){
			dojo.mixin(this, params);
			var vs = win.getBox();
			this.dlg = new Dialog({
				position: "absolute",
				top : "0px",
				left : "0px",
				width : vs.w +"px",
				height : vs.h +"px",
				backgroundColor : "gray",
				opacity : 0.9,
				onExecute: function(){
					this.dlg.hide();
				}
			});
			
			this.dlg.titleBar.style.display='none';
			
			var stDiv = domConstruct.create("div",{
			    "position": "fixed",
			    "top":"50%",
			    "left":"50%",
			    "margin-left":"-70px",
			    "margin-top":"-50px" 
			}, this.dlg.containerNode);
			
			domConstruct.create("div", {innerHTML:this.headline}, stDiv);
			
			this.progressDiv = domConstruct.create("div",{}, this.dlg.containerNode);
			
			this.progressBar = new ProgressBar({
				style : "width: "+(vs.w/2)+"px",
				maximum : this.maximum
			});
			this.progressBar.placeAt(stDiv);
		},
		
		show: function(){
			this.dlg.show();
		},
		
		hide: function(){
			this.dlg.hide();
		},
		
		setMax: function(value){
			this.progressBar.set("maximum",value);
		},
		
		setValue: function(value){
			this.progressBar.set("value",value);
		},
		
		getValue: function(){
			return this.progressBar.get("value");
		}
	});
	   
});
