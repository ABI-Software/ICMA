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
	"dojo/aspect",
	"dojo/date/locale",
	"dojo/dom",
	"dojo/dom-class",
	"dojo/dom-construct",
	"dojo/dom-style",
	"dojo/fx", // fxUtils.wipeIn fxUtils.wipeOut
	"dojo/topic",
	"dojox/mobile/Button",
	"dojox/mobile/TextBox",
	"dojox/layout/TableContainer",
	"dijit/TitlePane",
	"dijit/Dialog",
	"dojox/widget/TitleGroup",//Required by the help pages	
], function(declare, lang, aspect, locale, dom, domClass, domConstruct, domStyle, fxUtils,
		topic, Button, TextBox, TableContainer, TitlePane, Dialog){

	// module:
	//		ABI/lib/Widgets/PatientBanner

	return declare("ABI.lib.Widgets.PatientBanner", [TitlePane],{
		// summary:
		//		A widget that represents a Patient Banner for a clinical application.
		// description:
		//		Heading is a widget that represents a Patient Banner, which
		//		usually appears at the top of an application.
		
		open: false,
		
		activeView: "US",
		
		patientManager: null,
		
		
		constructor: function(params){
			dojo.mixin(this, params);

			aspect.after(this.patientManager, "onPatientUpdate", lang.hitch(this, this.update));
			topic.subscribe("ABI/lib/Widgets/PatientBanner/Hide", lang.hitch(this, this.onHide));
			topic.subscribe("/MRIViewLoaded", lang.hitch(this, function(){
				this.activeView = "MRI";
			}));
			topic.subscribe("/USViewLoaded", lang.hitch(this, function(){
				this.activeView = "US";
			}));
			topic.subscribe("/ModelsViewLoaded", lang.hitch(this, function(){
				this.activeView = "Model";
			}));
		},
		
		startup: function(){
			//console.log("Heading startup");
			if(this._started){ return; }
			this.inherited(arguments);
		},
		
		postCreate: function(){
			this.inherited(arguments);
			
			this.helpButton = new Button({
				baseClass: "helpButton",
				style: "float: right;margin-right: 10px;",
	            onClick: lang.hitch( this, function(){ 
	               //Load context sensitive help based on active window
	            	//Popup a help window
	            	//ABI.lib.Widgets.AlertSingleton.alert("Active", this.activeView);
	            	var uri = "./ABI/helpdocs/ushelp.html";
	            	if(this.activeView=="MRI")
	            		uri = "./ABI/helpdocs/mrihelp.html";
	            	else if(this.activeView=="Model")
	            		uri = "./ABI/helpdocs/modelhelp.html";
	            	
	            	var hdialog = new Dialog({
	            		style: "width: 640px;",
	            		href: uri
	            	});
	            	hdialog.show();
	            })
			}); 
			domConstruct.place(this.helpButton.domNode, this.titleBarNode, "before");
			
			
			//this.phoneButton = domConstruct.create("a", {
				//href: "callto://+",
			this.phoneButton = new Button({
				baseClass: "phoneButton",
				style: "float: right; margin-right: 10px;",
                onClick: lang.hitch( this, function(){ 
                	if(this.contactPhone.innerHTML.indexOf("Unavailable")>0)
                		alert("The contact number will be available when this system is integrated with the hospital directory");
                })
            }); 

/*            this.phoneButtonImg = domConstruct.create("img", {
                src: "ABI/lib/images/telephone-20x20.jpg"
            }, this.phoneButton); */
			
			domConstruct.place(this.phoneButton.domNode, this.titleBarNode, "before");

			this.emailButton = new Button({
				baseClass: "emailButton",
				//href: "mailto://+",
				style: "float: right; margin-right: 10px",
                onClick: lang.hitch( this, function(){ 
                	if(this.contactAddress.innerHTML.indexOf("Unavailable")>0)
                		alert("The email address will be available when this system is integrated with the hospital directory");
                })
			}); 
			domConstruct.place(this.emailButton.domNode, this.titleBarNode, "before");
			
		
			this.referringPhysician = domConstruct.create("span", {
				innerHTML: "",
				style: "float: right; margin-right: 10px:margin-top: 10px;"
			}); 
			domConstruct.place(this.referringPhysician, this.titleBarNode, "before");

			this.referringPhysicianLabel = domConstruct.create("span", {
				innerHTML: "Referring Physician:",
				style: "font-style: italic; float: right; margin-right: 5px:margin-top: 10px;"
			}); 
			domConstruct.place(this.referringPhysicianLabel, this.titleBarNode, "before");
			
			this.detailsTable = domConstruct.create("table", {
				style: "width: 100%; font-style: italic;"
			}); 
			domConstruct.place(this.detailsTable, this.containerNode);

			var tr = domConstruct.create("tr", { }); 
			domConstruct.place(tr, this.detailsTable);

			this.contactAddress = domConstruct.create("td", {}); 
			domConstruct.place(this.contactAddress, tr);

			this.contactPhone = domConstruct.create("td", {
				style: "color:red"
			}); 
			domConstruct.place(this.contactPhone, tr);
			
			this.gender = domConstruct.create("td", {}); 
			domConstruct.place(this.gender, tr);

			this.height = domConstruct.create("td", {}); 
			domConstruct.place(this.height, tr);
			
			tr = domConstruct.create("tr", {}); 
			domConstruct.place(tr, this.detailsTable);
			
			this.contactAddress2 = domConstruct.create("td", {}); 
			domConstruct.place(this.contactAddress2, tr);

			this.contactPhone2 = domConstruct.create("td", {}); 
			domConstruct.place(this.contactPhone2, tr);

			this.dateOfBirth = domConstruct.create("td", {}); 
			domConstruct.place(this.dateOfBirth, tr);

			this.weight = domConstruct.create("td", {}); 
			domConstruct.place(this.weight, tr);
			
			// setup open/close animations
			// overriding because we want to notify onEnd
			var hideNode = this.hideNode, wipeNode = this.wipeNode;
			
			this._wipeIn = fxUtils.wipeIn({
				node: wipeNode,
				duration: this.duration,
				beforeBegin: function(){
					hideNode.style.display="";
					
				},
				onEnd:  lang.hitch(this, function(){
					this.onOpen();
				})
			});
			this._wipeOut = fxUtils.wipeOut({
				node: wipeNode,
				duration: this.duration,
				onEnd: lang.hitch(this, function(){
					hideNode.style.display="none";
					this.onClose();
				})
			});
		},
		
		onHide: function(){
			var hideNode = this.hideNode, wipeNode = this.wipeNode;
			this._wipeOut = fxUtils.wipeOut({
				node: wipeNode,
				duration: this.duration,
				onEnd: lang.hitch(this, function(){
					hideNode.style.display="none";
					this.onClose();
				})
			});
		},
		
		update: function(){
			this.patient = this.patientManager.getCurrentPatient();
			this.set("title", "Patient Id: " + this.patient.patientid + ",  Patient Name: " + this.patient.patientName);
			if(this.patient.ReferringPhysician)
				this.referringPhysician.innerHTML = this.patient.ReferringPhysician.name;
			else
				this.referringPhysician.innerHTML = "Not available";
			this.contactAddress.innerHTML = this.patient.patientMetaData.contactAddress == '"Not available"' ? "Contact Address Unavailable" :  this.patient.patientMetaData.contactAddress;
			if(this.patient.patientMetaData.contactAddress.indexOf('Not available')>0)
				domStyle.set(this.emailButton.domNode,'display','none');
			else
				domStyle.set(this.emailButton.domNode,'display','block');
			this.contactPhone.innerHTML = this.patient.patientMetaData.contactPhone == '"Not available"' ? "Contact Phone Number Unavailable" :  this.patient.patientMetaData.contactPhone;;
			if(this.patient.patientMetaData.contactPhone.indexOf('Not available')>0)
				domStyle.set(this.phoneButton.domNode,'display','none');
			else
				domStyle.set(this.phoneButton.domNode,'display','block');
			//this.phoneButton.href = "callto://+" + this.patient.patientMetaData.contactPhone;
			this.phoneButton.href = "callto:" + this.patient.ReferringPhysician.phone;
			this.emailButton.href = "mailto:" + this.patient.ReferringPhysician.email;
			this.gender.innerHTML = this.patient.patientMetaData.gender;
			this.height.innerHTML = this.patient.patientMetaData.size;
			this.dateOfBirth.innerHTML = this.patient.patientMetaData.dateOfBirth;
			this.weight.innerHTML = this.patient.patientMetaData.weight + " kg";
			this._setOpenAttr(false, false); //The banner is closed by default
		},
		
		onOpen: function(){
			// stub for notification of opened
		},
		
		onClose: function(){
			// stub for notification of closed
		},
		
		resize: function(){
		}

	});
});
