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
	"dojo/parser",
	"dojo/_base/lang",
	"dojo/_base/window",
	"dojo/aspect",
	"dojo/dom",
	"dojo/dom-attr",
    "dojo/dom-construct",
    "dojo/dom-style",
    "dojo/dom-geometry",
	"dojo/on",
	"dojo/query",
	"dojo/topic",
	"dojo/store/JsonRest",
	"dijit/dijit",
	"dojox/widget/Standby",
//	"dijit/_base/manager", //dijit.byId
	"dijit/layout/BorderContainer",
	"dijit/layout/ContentPane",
	"dijit/layout/TabContainer",
	"dojox/layout/TableContainer",
	"dijit/_Contained",
	"dijit/_Container",
	"dijit/_WidgetBase",
	"dojox/mobile",
	"dojox/mobile/Button",
	"dojox/mobile/TextBox",
	"dojox/mobile/Overlay",
	"dgrid/Grid",
    "dgrid/Selection",
    "dgrid/Keyboard",
    "dgrid/extensions/Pagination",
    "ABI/app/ICMA/PatientModels",
    "ABI/app/ICMA/PatientScans",
    "ABI/app/ICMA/PatientMRIScans",
	"ABI/lib/Widgets/Heading",
	"ABI/lib/Widgets/PatientBanner",
	"ABI/lib/Widgets/Password"
	], function(declare, parser, lang, win, aspect, dom, domAttr, domConstruct, 
			domStyle, domGeom, on, query, topic, JsonRest, dijit, 
			StandBy, BorderContainer, ContentPane, TabContainer, 
			TableContainer, Contained, Container, WidgetBase, dm, Button, 
			TextBox, Overlay, Grid, Selection, Keyboard, Pagination,
			PatientModels, PatientScans, PatientMRIScans, Heading, PatientBanner,Password){
  
	// module:
	//		ABI/app/ICMA/PatientView
	// summary:
	//	 PatientView
	

	return declare("ABI.app.ICMA.PatientView", [WidgetBase, Container, Contained],{
		
		server: "",
		isMobile: false,
		patient: null,
		debug: false,
		ustabalive: true,
		showingPatientData: false,
		usscansready: false,
		mriscansready: false,
		
		
		constructor: function(params){
			dojo.mixin(this, params);
		},
		
		startup: function(){
			if(this.debug)
				console.log("PatientView startup()");
			
			this._patientView.startup();
			this.bc.resize();
			this.bcCenter.resize();

			//?? I don't know why this doesn't work. Using topic instead
			//aspect.after(this.heading, 'onSearchButtonClick', lang.hitch(this, this.onSearchButtonClick), true);
			topic.subscribe("ABI/lib/Widgets/Heading/onSearchButtonClick", lang.hitch(this, this.onSearchButtonClick));
			topic.subscribe("ABI/lib/Widgets/Heading/onLogout", lang.hitch(this, this.onLogout));
			topic.subscribe("ABI/lib/Widgets/Heading/onChangePassword", lang.hitch(this, this.onChangePassword));
			topic.subscribe("ABI/lib/Widgets/ustabalive", lang.hitch(this, this.showustab));
			topic.subscribe("ABI/lib/Widgets/mritabalive", lang.hitch(this, this.showmritab));
			topic.subscribe("ABI/lib/Widgets/modelstabalive", lang.hitch(this, this.showmodelstab));
			topic.subscribe("ABI/lib/Widgets/ustabhide", lang.hitch(this, this.hideustab));
			topic.subscribe("ABI/lib/Widgets/mritabhide", lang.hitch(this, this.hidemritab));
			topic.subscribe("ABI/lib/Widgets/modelstabhide", lang.hitch(this, this.hidemodelstab));
			topic.subscribe("ABI/lib/Widgets/busy", lang.hitch(this, this.showBusy));
			topic.subscribe("ABI/lib/Widgets/scansready", lang.hitch(this, this.uscansPageReady));
			topic.subscribe("ABI/lib/Widgets/mriready", lang.hitch(this, this.mriscansPageReady));
			topic.subscribe("ABI/lib/Widgets/ready", lang.hitch(this, this.hideBusy));
			
			this.patientStore = new JsonRest({
				target: this.server + "/SXMPatients", 
				idProperty:"patientid"
			});
			
			aspect.after(ABI.app.ICMA.PatientManagerSingleton, "onPatientUpdate", lang.hitch(this, this.setTabs));
			
			//Links selectstudy event in models tab to the ustab
			topic.subscribe("/link", lang.hitch(this, this.selectStudy));
			//Without the call to patient search the patient list is not loaded  			
			this.patientSearch();
			//Handle refresh events
			
		},
		
		buildRendering: function(){
			if(this.debug)
				console.log("PatientView buildRendering()");
			
			// The layout is a View with a BorderContainer specifying Top, Center and Bottom regions
			// This gives a fixed size header (logged in header) and footer (Top & Bottom) regions
			// The Center region is then similarly divided up so as to provide a secondary header (Patient Banner header) & footer regions
			// (To start with both footer regions are empty - zero height)
			
			this.domNode = this.containerNode = this.srcNodeRef || win.doc.createElement("div");
			this.inherited(arguments);
			
			this._patientView = new dm.View({
				id: "PatientView",
				selected: true,
				style: "height: 100%;  display: ",
				resize: lang.hitch(this,this.pvresize)
			}, domConstruct.create("div", null, win.body()));
			
			aspect.after(this._patientView, "onAfterTransitionIn", lang.hitch(this, this.onAfterPatientViewTransitionIn));
			aspect.after(this._patientView, "resize", lang.hitch(this, this.resize));
			        
			//
			// Outer BorderContainer
			//
			this.bc = new BorderContainer({ gutters: false, style: "width:100%; height:100%; border:0px solid white;z-index:100" });
			this.bc.placeAt(this._patientView.domNode);
			
			var cpTop = new ContentPane({ 
				region: 'top', 
				style: "background:transparent; border:0px solid white; padding: 0px; height: auto;" 
			}).placeAt(this.bc);

			this.cpCenter = new ContentPane({ 
				region: 'center', 
			//	innerHTML:"center",
				style: "border:0px solid white;  padding: 0px; overflow: hidden;" /* background:red; */ 
			}).placeAt(this.bc);
			
			

			var cpBottom = new ContentPane({ 
				region: 'bottom', 
				//innerHTML:"bottom",
				style: "background:transparent; border:0px solid white; padding: 0px; overflow: hidden; height: auto;" /* background:green; */ 
			}).placeAt(this.bc);
		
			this.heading = new Heading({
				//label: "ICMA v1.0",
				style : "height:auto; width:auto",
				searchButton: true
			});
			this.heading.placeAt(cpTop.domNode);
			
			//
			// BorderContainer in Center pane
			//
			this.bcCenter = new BorderContainer({ gutters: false, style: "width:100%; height:100%; border:0px solid white;" });
			this.bcCenter.placeAt(this.cpCenter.domNode);
			
			var cpCenterTop = new ContentPane({ 
				region: 'top', 

				//innerHTML:"centertop",
				style: "background:transparent; border:0px solid white; padding: 0px;  height: auto;" 
			}).placeAt(this.bcCenter);
			
			var cpCenterCenter = new ContentPane({ 
				region: 'center', 
				//innerHTML:"centerCenter",
				style: "background:transparent; border:0px solid white; padding: 0px; overflow: hidden; height: auto;" /* background:red; */ 
			}).placeAt(this.bcCenter);
			
			this.addPatientBanner(cpCenterTop);	
			this.addScansModelsTabs(cpCenterCenter);				

			var cpCenterBottom = new ContentPane({ 
				region: 'bottom', 
				//innerHTML:"Centerbottom",
				style: " background:transparent; border:0px solid white; padding: 0px; overflow: hidden; height: auto;" /* background:green; */
			}).placeAt(this.bcCenter);			
		},
		
		addPatientBanner: function(node){
			this.patientBanner = new PatientBanner({
				patientManager: ABI.app.ICMA.PatientManagerSingleton,
				onOpen: lang.hitch(this, function(){
					this.bc.resize();
				}),
				onClose: lang.hitch(this, function(){
					this.bc.resize();
				}),
				open: false
			});
			node.addChild(this.patientBanner);
			this.bc.resize();
		},
		
		addScansModelsTabs: function(node){
			this.tcScansModels = new TabContainer({
	            style: "height: 110%; width: 100%;"
	        });
			node.addChild(this.tcScansModels);
			
	        this.tcScansModels.watch("selectedChildWidget", function(name,oval,nval){
	        	//console.debug("Calling from selectedChildWidget ",nval);
	        	var title = nval.title;
	        	if(title=="US Scans"){
	        		topic.publish("/USViewLoaded");
	        	}else if(title=="MRI Scans"){
	        		topic.publish("/MRIViewLoaded");
	        	}else if(title=="Models"){
	        		topic.publish("/ModelsViewLoaded");
	        	}
	        });
			
			//this.standBy = new StandBy({target:this.tcScansModels.domNode});
			this.standBy = new StandBy({target:this.bc.domNode});
			node.addChild(this.standBy);
			this.standBy.startup();

            this.usscansTab = new ContentPane({
                 title: "US Scans",
                 style: "width: 100%; height: 100%;"
            });
            this.tcScansModels.addChild(this.usscansTab);
            
            this.mriscansTab = new ContentPane({
                title: "MRI Scans",
                style: "width: 100%; height: 100%;"
           });
           this.tcScansModels.addChild(this.mriscansTab);
            
	        this.modelsTab = new ContentPane({
	             title: "Models"
	        });
	        this.tcScansModels.addChild(this.modelsTab);
            
	        this.tcScansModels.startup();	
	        
	        this.patientScans = new PatientScans({
	        	server: this.server,
	        	patientManager: ABI.app.ICMA.PatientManagerSingleton
	        });
	        this.patientScans.placeAt(this.usscansTab.domNode);
	        
	        this.patientMRIScans = new PatientMRIScans({
	        	server: this.server,
	        	patientManager: ABI.app.ICMA.PatientManagerSingleton
	        });
	        this.patientMRIScans.placeAt(this.mriscansTab.domNode);
	        
	        
	        this.patientModels = new PatientModels({
	        	server: this.server,
	        	patientManager: ABI.app.ICMA.PatientManagerSingleton
	        });
	        this.patientModels.placeAt(this.modelsTab.domNode);
        
	        this.modelsTab.set("onShow", lang.hitch(this, function(){
	        		if(this.debug)
                    	console.log("PatientView modelsTab onShow");
                    try{
                    	this.patientModels.tcModels.selectedChildWidget.analysisTab.onShow();
                    }catch(e){
                        //This will get called on startup as there will not be a selected model yet
                    }
                    try{
                        if(this.patientModels.hasShown && this.patientModels.needsReloaded){
                            this.patientModels.onShow();
                        }
                    }catch(e){
                        //This will get called on startup as there will not be a selected model yet
                    }
             }));
	        
		},

		
		pvresize: function(){
			if(this.patientSearchOverlay){
				var height = this._patientView.domNode.clientHeight - this.heading.domNode.clientHeight - 3;
				domStyle.set(this.patientSearchOverlay.domNode,"height",height+"px");
                var footer = query(".dgrid-footer", this.grid.domNode)[0];    
                var footerSize = domGeom.position(footer);
                var scroller = query(".dgrid-scroller", this.grid.domNode);
                scroller.style("height", height - footerSize.h + "px");
                domStyle.set(this.grid.domNode, "height",  height - footerSize.h + "px");
			}
		},
		
		resize: function(){
			this.bc.resize();
		},
		
		patientSearch: function(){

            //Change to the scans tab prior to searching to accommodate for the lack of windowless mode - effectively removing plugins while searching
			if(this.ustabalive){
				this.tcScansModels.selectChild(this.usscansTab);
				//console.debug("US selected");
				topic.publish("/USViewLoaded");
			}
			else{
				this.tcScansModels.selectChild(this.mriscansTab);
				//console.debug("MRI selected");
				topic.publish("/MRIViewLoaded");
			}
            //console.debug(this.heading.domNode.clientHeight);
			
			//console.debug(window);
            //var height = window.innerHeight - this.heading.domNode.clientHeight - 3;
			
			
            var height = this._patientView.domNode.clientHeight - this.heading.domNode.clientHeight - 3;
			if(!this.patientSearchOverlay){
				this.patientSearchOverlay = new Overlay({
					innerHTML: "Patients",
					style: "height: " + height + "px; z-index:300"
					//style: "height: 93%"
				});
				this.patientSearchOverlay.placeAt(this._patientView.domNode);
				
				var columns = [
		           {
		               field: "patientName",
		               label: "Name"
		           },
		           {
		               field: "patientid",
		               label: "Patient Id"
		           },
		           {
		               field: "dateOfBirth",
		               label: "Birth Date"
		           }
				];
				
				var cp = new ContentPane({ 
					style: "border:0px solid white; padding: 0px ;" 
				}).placeAt(this.patientSearchOverlay.domNode);
	
				var CustomGrid = declare([ Grid, Pagination, Keyboard, Selection ]);
				
				this.grid = new CustomGrid({
					columns: columns,
					//query: "?search=nasa",
					store: this.patientStore,
		            selectionMode: "single", 
		            cellNavigation: false,
		            pagingLinks: 1,
		            pagingTextBox: true,
		            firstLastArrows: true,
		            minRowsPerPage: 20,
		            rowsPerPage: 20,
		            pageSizeOptions: [20, 50, 100]
				}, cp.domNode);
	
				this.grid.startup();
				
				this.grid.on("dgrid-select", lang.hitch(this, this.onPatientSelect));

                var footer = query(".dgrid-footer", this.grid.domNode)[0];    
                var footerSize = domGeom.position(footer);
                var scroller = query(".dgrid-scroller", this.grid.domNode);
                scroller.style("height", height - footerSize.h + "px");
                domStyle.set(this.grid.domNode, "height",  height - footerSize.h + "px");
					
			}else{
				domStyle.set(this.patientSearchOverlay.domNode, "height", height + "px");
				
				var footer = query(".dgrid-footer", this.grid.domNode)[0];    
                var footerSize = domGeom.position(footer);
                var scroller = query(".dgrid-scroller", this.grid.domNode);
                scroller.style("height", height - footerSize.h + "px");
                domStyle.set(this.grid.domNode, "height",  height - footerSize.h + "px");
				
			}
			this.patientSearchOverlay.show(this._patientView.domNode);
		},
		
		onAfterPatientViewTransitionIn: function(){
			if(!this.patient){
				this.showingPatientData = false;
				this.patientSearch();
			}
		},
		
		onSearchButtonClick: function(){
			if(this.debug)
				console.log("PatientView onSearchButtonClick");
			//this.patientSearch();
			//this.grid.filter({search: patientSearchBox.get('value')});
			this.grid.set("query",'?search='+this.heading.patientSearchBox.get('value'));
			this.heading.patientSearchBox.set('value',"");
			this.showingPatientData = false;
			if(this.ustabalive)
				this.tcScansModels.selectChild(this.usscansTab);
			else
				this.tcScansModels.selectChild(this.mriscansTab);
            this.patientSearchOverlay.show(this._patientView.domNode);
		},
		
		onLogout: function(){
			//Do some checks
			
			var url = this.server + "/logout.jsp"; // or var url = listItem.url;
			window.open(url,'_self','resizable,location,menubar,toolbar,scrollbars,status');
		},
		
		onChangePassword: function(){
			myPassword = new Password({
				service:this.server+"/changePassword"
			});
			myPassword.PasswordShow();
		},
		
		onPatientSelect: function(event){
			var patient = event.rows[0].data;
		    //console.log("Patient selected: ", patient);
		    ABI.app.ICMA.PatientManagerSingleton.loadPatient(patient.patientid, this.server);

/*            this.patientSearchOverlay.hide();
            //this.patientSearchOverlay.domNode.style.display = "none";
*/			this.bc.resize();
		    this.showingPatientData = true;
		},
		
		selectStudy: function(study){
			if(this.debug)
				console.log("study: " + study);
			this.tcScansModels.selectChild(this.usscansTab);
			this.bc.resize();
		},
		
		showBusy: function(){
/*			this.showtimestart = new Date().getTime();
			console.debug("Show busy called");*/
			document.body.style.cursor  = 'wait';  
            this.standBy.show();
		},

		uscansPageReady: function(){
			this.usscansready = true;
			if(this.mriscansready)
				this.doTabSelection();
		},
		
		mriscansPageReady: function(){
			this.mriscansready=true;
			if(this.usscansready)
				this.doTabSelection();
		},
		
		doTabSelection: function(){
			//Do this else emplty us tab will be shown
			if(this.ustabalive)
				this.tcScansModels.selectChild(this.usscansTab);
			else{
				this.tcScansModels.selectChild(this.mriscansTab);
			}
		},
		
		
		hideBusy: function(){
/*			var dur = new Date().getTime() - this.showtimestart;
			console.debug("Hide busy called "+ dur/1000);
*/
			document.body.style.cursor  = 'default';
			this.patientSearchOverlay.hide();
			this.standBy.hide();
		},
		
		setTabs: function(){
			if(this.ustabalive)
				this.tcScansModels.selectChild(this.usscansTab);
		},
		
		showustab: function(){
			this.usscansTab.controlButton.disabled = false;
			this.usscansTab.resize();
			this.ustabalive = true;
		},
		
		showmritab: function(){
			this.mriscansTab.controlButton.disabled = false;
			this.mriscansTab.resize();
		},
		
		showmodelstab: function(){
			this.modelsTab.controlButton.disabled = false;
			this.modelsTab.resize();
		},
		
		hideustab: function(){
			this.usscansTab.controlButton.disabled = true;
			this.ustabalive = false;
		},
		
		hidemritab: function(){
			this.mriscansTab.controlButton.disabled = true;
		},
		
		hidemodelstab: function(){
			this.modelsTab.controlButton.disabled = true;
		}
	});
});
                