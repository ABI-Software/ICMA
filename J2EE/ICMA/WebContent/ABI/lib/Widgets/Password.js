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
	"dijit/form/Button",
	"dojox/layout/TableContainer",
	"dijit/form/ValidationTextBox",
	"dojox/encoding/digests/SHA1",
	"dojo/request",
], function(declare, lang, domConstruct, Dialog, 
		Button,TableContainer,ValidationTextBox, SHA1, request){

	// module:
	//		ABI/lib/Widgets/Password

	return declare("ABI.lib.Widgets.Password", null,{
		// summary:
		//		An Password dialog
		newPass: null,
		
		constructor: function(params){
			dojo.mixin(this, params);
			
			this.PasswordDlg = new Dialog({
				title: "Change password",
				onExecute: function(){
					this.PasswordDlg.hide();
				}
			});

			var tbc = new TableContainer({    
                customClass: "viewProperties",
            },this.PasswordDlg.containerNode);
			
			this.oldPassword = new ValidationTextBox({
                label: "Old Password",
                type: "password"
            });
			this.newPassword = new ValidationTextBox({
                label: "New Password",
                type: "password"
            });
			this.verifyPassword = new ValidationTextBox({
                label: "Verify Password",
                type: "password"
            });			
			this.oldPassword.placeAt(tbc.containerNode);
			this.newPassword.placeAt(tbc.containerNode);
			this.verifyPassword.placeAt(tbc.containerNode);
            tbc.startup();
			
			
		
			var actionBarDiv = domConstruct.create("div", {
				"class":"dijitDialogPaneActionBar"
			}, this.PasswordDlg.containerNode);

			var OKButton = new Button({
				label: "OK",
				"class": "",
				onClick: lang.hitch(this, this.checkPassword)
			});
			OKButton.placeAt(actionBarDiv);
			
			var CancelButton = new Button({
				label: "Cancel",
				"class": "",
				onClick: lang.hitch(this, this.PasswordHide)
			});
			CancelButton.placeAt(actionBarDiv);
		},
		
		PasswordShow: function(){
			this.PasswordDlg.show();
		},
		
		PasswordHide: function(){
			this.PasswordDlg.hide();
		},
		
		checkPassword: function(){
			var oldPassword = this.oldPassword.get("value");
			var newPassword = this.newPassword.get("value");
			var verifyPassword = this.verifyPassword.get("value");
			if(newPassword!=verifyPassword){
				alert("New password does not match with its verification");
				return;
			}
			var encold = SHA1(oldPassword);
			var encnew = SHA1(newPassword);
			var reqdata = new Object();
					
			reqdata["newpassword"] = encnew;
			reqdata["oldpassword"] = encold;
			reqdata["username"] = username;
			
/*			request.post(this.service, {
        		data : dojo.toJson(reqdata),
        	}).then(lang.hitch(this, function(data){
        		alert("Password change was successful");
        		this.PasswordHide();
            }), function(err){
        		alert("Password change was unsuccessful "+err);
            });*/
			
						
			var xhrArgs = {
	                url: this.service+"?username="+username+"&oldpassword="+encold+"&newpassword="+encnew,
	                handleAs: "text",	
	                pDialog : this,
	                load: function(data){
	                	if(data.indexOf("success")>-1){
		                	alert("Password change was successful");
		                	this.pDialog.PasswordHide();
	                	}else{
	                		alert("Password change was unsuccessful "+data);
	                	}
	                },
	                error: function(error){
	                	if(error.response.data.indexOf("Password Incorrect")){
	                		alert("Password change was unsuccessful- password incorrect");
	                	}else
	                		alert("Password change was unsuccessful ");
	                }
	            };
	            var deferred = dojo.xhrPost(xhrArgs);
			
		}
		
	});
    
});
