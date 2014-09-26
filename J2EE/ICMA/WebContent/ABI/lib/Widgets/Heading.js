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
	"dojo/date/locale",
	"dojo/dom",
	"dojo/dom-class",
	"dojo/dom-construct",
	"dojo/dom-style",
	"dojo/topic",
	"dojox/mobile/ToolBarButton",
	"dojox/mobile/Heading",
	"dojox/layout/TableContainer",
	"dijit/layout/BorderContainer",
	"dijit/layout/ContentPane",
	"dijit/form/TextBox",
	"dijit/form/DropDownButton", 
	"dijit/DropDownMenu", 
	"dijit/MenuItem"
], function(declare, lang, locale, dom, domClass, domConstruct, domStyle, topic, 
		ToolBarButton, Heading, TableContainer, BorderContainer, ContentPane, DijitTextBox,
		DropDownButton, DropDownMenu, MenuItem){

	// module:
	//		ABI/lib/Widgets/Heading

	return declare("ABI.lib.Widgets.Heading", [Heading],{
		// summary:
		//		A widget that represents a navigation bar for a clinical application.
		// description:
		//		Heading is a widget that represents a navigation bar, which
		//		usually appears at the top of an application.
		// 	    This widget inherits from the dojox.mobile.heading for navigational elements etc
		//      and adds ABI logo, and clinical information like logged in user and location

		logoImage: "ABI/lib/images/abi-logo.png",
		//logoImage: "ABI/lib/images/TestLargeImage.png",
		searchButton: false, // Add patient search button
		
		loggedInUserName: "",
		loggedInLocation: "",
		loggedInTime: "",
		
		constructor: function(params){
			dojo.mixin(this, params);
			//console.debug("username @ heading "+username);
			loggedInUserName = username;
			loggedInTime = new Date();
		},
		
		startup: function(){
			//console.log("Heading startup");
			if(this._started){ return; }
			this.inherited(arguments);
		},
		
		postCreate: function(){

			var headerLogoDiv = domConstruct.create("div", {
				"class":"mblHeadingDivTitle", 
				style:"z-index:-1"
			}); 
			this.containerNode.appendChild(headerLogoDiv);
			
			var headerLogo = domConstruct.create("img", {
				src: this.logoImage,
			},headerLogoDiv); 
			
			if(this.searchButton){
				var parentNode = this;
				this.patientSearchBox = new DijitTextBox({
					name : "search",
					value: "",
					placeHolder: "Find patient",
					style: "float: right; margin-top: 12px; margin-right: 20px",
					onKeyPress :function(e) {
						if (e.keyCode == dojo.keys.ENTER) {
							parentNode.onSearchButtonClick();
						}
					}
				},"Search");
				this.containerNode.appendChild(this.patientSearchBox.domNode);
			}
			
			var loggedInDetails = domConstruct.create("div", {
				style: "float: left; margin-left: 10px"
			});
			this.containerNode.appendChild(loggedInDetails);
			
			this.loggedinUserDiv = domConstruct.create("div", {
				//innerHTML: this.loggedInUserName,
				//style: "margin-left: 10px; margin-top: 5px; line-height: 20px;"
			//}, tableContainer.domNode);
			},loggedInDetails);
			
			
			var menu = new DropDownMenu({ style: "display: none;"});
		    var menuItem1 = new MenuItem({
		            label: "Logout",
		            iconClass : "dijitCommonIcon dijitIconUsers",
		            onClick: function(){ topic.publish("ABI/lib/Widgets/Heading/onLogout"); },
		            style: "z-index:9002"
		    });
		    menu.addChild(menuItem1);

		    var menuItem2 = new MenuItem({
		        label: "Change Password",
		        iconClass : "dijitCommonIcon dijitIconKey",
		        onClick: function(){ topic.publish("ABI/lib/Widgets/Heading/onChangePassword"); },
		    	style: "z-index:9001"
		    });
		    menu.addChild(menuItem2);
		    
		    var button = new DropDownButton({
		            label: loggedInUserName,
		            name: "userAuth",
		            dropDown: menu,
		            style: "z-index:9000"
		            //id: "userAuthButton"
		    });
		    button.startup();
		    this.loggedinUserDiv.appendChild(button.domNode);
			

/*			var tableContainer = new TableContainer({
		    });
		    tableContainer.placeAt(loggedInDetails);*/
			
/*			this.loggedInLocationDiv = domConstruct.create("div", {
				innerHTML: this.loggedInLocation,
				style: "font-style:italic; float: left; margin-left: 10px; margin-top: 5px; line-height: 20px;"
			}, tableContainer.domNode);*/

			this.loggedInTimeDiv = domConstruct.create("div", {
				//innerHTML: this.loggedInTime,
				style: "float: left; margin-left: 10px; margin-top: 5px; line-height: 20px;"
			//}, tableContainer.domNode);
			},loggedInDetails);
			
			{
				this.loggedInTimeDiv.innerHTML = locale.format(loggedInTime,{
				    formatLength: "short",
				    datePattern: "MMM d y,",
					timePattern: "hh:mm a"
				});
			}
		},
	
		_setLoggedInUserNameAttr: function(loggedInUserName){
			this.loggedInUserName = loggedInUserName;
			//this.loggedinUserDiv.innerHTML = loggedInUserName;
		},
		
		_setLoggedInLocationAttr: function(loggedInLocation){
			this.loggedInLocation = loggedInLocation;
			this.loggedInLocationDiv.innerHTML = loggedInLocation;
		},

		_setLoggedInTimeAttr: function(loggedInTime){
			this.loggedInTime = loggedInTime;
/*			this.loggedInTimeDiv.innerHTML = locale.format(loggedInTime,{
			    formatLength: "short",
			    datePattern: "MMM d y,",
				timePattern: "h:m a"
			});*/
		},

		onLoggedInUserChange: function(user){
/*			this._setLoggedInUserNameAttr(user.userName);
			this._setLoggedInLocationAttr(user.loginlocation);
			this._setLoggedInTimeAttr(user.loginTime);*/
			//this._setLoggedInUserNameAttr(loggedInUserName);
			//this._setLoggedInTimeAttr(new Date());
		},
		
		onSearchButtonClick: function(){
			//console.log("Heading onSearchButtonClick");
			topic.publish("ABI/lib/Widgets/Heading/onSearchButtonClick",this.patientSearchBox);
		}

	});
});
