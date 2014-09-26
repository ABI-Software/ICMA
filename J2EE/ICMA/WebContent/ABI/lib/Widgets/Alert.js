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
	"dojo/dom-construct",
	"dijit/Dialog",
	"dijit/form/Button"
], function(declare, lang, domConstruct, Dialog, Button){

	// module:
	//		ABI/lib/Widgets/Alert

	var Alert = declare("ABI.lib.Widgets.Alert", null,{
		// summary:
		//		An alert dialog
		// description:
		//		A singleton pattern alert dialog
		// usage:
		//		ABI.lib.Widgets.AlertSingleton.alert("My title", "My message");

		constructor: function(params){
			dojo.mixin(this, params);
			
			this.alertDlg = new Dialog({
				title: "",
				onExecute: function(){
					this.alertDlg.hide();
				}
			});

			this.messageDiv = domConstruct.create("div", {
				"class": "",
				innerHTML: ""
			}, this.alertDlg.containerNode);
			
			var actionBarDiv = domConstruct.create("div", {
				"class":"dijitDialogPaneActionBar"
			}, this.alertDlg.containerNode);

			var OKButton = new Button({
				label: "OK",
				"class": "",
				onClick: lang.hitch(this, this.alertHide)
			});
			OKButton.placeAt(actionBarDiv);
		},
		
		alertShow: function(){
			this.alertDlg.show();
		},
		
		alertHide: function(){
			this.alertDlg.hide();
		},
		
		alert: function(title, message){
			this.alertDlg.set("title", title);
			this.messageDiv.innerHTML = message;
			this.alertShow();
		}
		
	});
	

    ABI.lib.Widgets._alert = null;

    Alert.getSingleton =ABI.lib.Widgets.alert = function () {
        if (!ABI.lib.Widgets._alert) {
        	ABI.lib.Widgets._alert = new ABI.lib.Widgets.Alert();
        }
        return ABI.lib.Widgets._alert;
    };

    ABI.lib.Widgets.AlertSingleton = Alert.getSingleton();

    return Alert;
    
});
