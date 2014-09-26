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
	"dojo/_base/array",
	"dojo/_base/event",
	"dojo/_base/lang",
	"dojo/_base/window",
	"dojo/_base/sniff",
	"dojo/aspect",
	"dojo/date/locale",
	"dojo/dom",
	"dojo/dom-class",
	"dojo/dom-construct",
	"dojo/dom-style",
	"dojo/topic",
	"dojo/on",
	"dojo/query",
	"dojo/dom-attr",
	"dojox/mobile/Carousel",
	"dojox/mobile/CarouselItem",
	//"dojox/mobile/Button",
	"dijit/form/Button",
    "dojox/mobile/SwapView",
	"dojox/mobile/TextBox",
	"dojox/mobile/Video",
	"dojox/layout/TableContainer",
	"dojox/uuid/Uuid",
	"dojox/uuid/generateRandomUuid",
	"dijit/Dialog",
	"dijit/TitlePane",
	"dijit/form/TextBox",
	"dijit/form/CheckBox",
	"dijit/form/RadioButton",
	"dijit/form/HorizontalSlider",
	"dijit/layout/BorderContainer",
	"dijit/layout/ContentPane",
	"dijit/layout/TabContainer",
	"dijit/form/Select",
	"dijit/form/SimpleTextarea",
	"dijit/_Contained",
	"dijit/_Container",
	"dijit/_WidgetBase",
	"dojox/charting/Chart", 
	"dojox/charting/action2d/_IndicatorElement",
	"dojox/charting/axis2d/Default", 	
	"dojox/charting/plot2d/Lines",
    "dojox/charting/widget/Legend",
    "dojox/charting/widget/SelectableLegend",
    "dojox/form/CheckedMultiSelect",
    "dojo/data/ItemFileReadStore",
	"ABI/app/ICMA/ZincWidget",
	"ABI/app/ICMA/MRIZincWidget",
	"ABI/lib/Widgets/Alert"
], function(declare, array, event, lang, win, has, aspect, locale, dom, domClass, domConstruct, domStyle, topic, on, query, domAttr, Carousel, CarouselItem, Button, 
        SwapView, 
		TextBox, Video, TableContainer, Uuid, generateRandomUuid, Dialog, TitlePane, 
		DijitTextBox, DijitCheckBox, DijitRadioButton, HorizontalSlider, BorderContainer, 
		ContentPane, TabContainer, Select, Textarea,
		Contained, Container, WidgetBase, 
		Chart, IndicatorElement, Default, Lines, Legend, SelectableLegend,CheckedMultiSelect,ItemFileReadStore,
		ZincWidget, MRIZincWidget, Alert){

	// module:
	//		ABI/app/ICMA/Model
	

	return declare("ABI.app.ICMA.Model", [ContentPane],{
		// summary:
		//		A widget that presents a model
		// description:
		//		
        uuid: "",
        image_elem: "ABI/app/ICMA/Imagetexture_block.exregion",
        rate: 0.5, // player time increment
        playerId: null,
        plotColours: ["","#0000fd","#00a9a9","#00b200","#8f8e0d","#db7700","#dd0000","#048cfc","#04e4e4","#00ff00","#ffff00","#fc8c04","#ff4343","#6dbbfb","#74b774","#fcfc6e","#fb999a","#000000"],
        server: null,
        sxmUpdate: "",
        heartLines: true,
        heartSurface: true,
        heartFibres: false,
        heartNodes: false,
        debug: false,
        pluginsNeedCreated: false,
        maxcapframeid: 18,
        timescalefactor: 1,
        usmodel: true,
		constructor: function(params){
			dojo.mixin(this, params);
			this.maxcapframeid = this.model.Regions.MESH.regionEndIndex -1;

			if(this.model.type=="MR"){
				this.usmodel = false;
				this.timescalefactor = this.maxcapframeid;
			}
			this.uuid = new Uuid(dojox.uuid.generateRandomUuid()).toString();
			
			this.transformation = new Object();    
            this.regionLoaded = new Object();
            
            topic.subscribe("/" + this.uuid + "/Time", lang.hitch( this, this.setTime));
			topic.subscribe("/" + this.uuid + "/PlayStop", lang.hitch( this, this.playStop));
			//Hide the banner to match the ui sizing
			topic.publish("ABI/lib/Widgets/PatientBanner/Hide","");
		},
		
		startup: function(){
			if(this._started){ return; }
			this.inherited(arguments);
		},
		
		buildRendering: function(){
			this.inherited(arguments);

			this.bc = new BorderContainer({
	            style: "height: 100%; width: 100%;"
	        });
	        
	        this.addChild(this.bc);
	        
	        this.bc.startup();
	        
	        this.addTopCenterPanes();
	        
	        this.addModelLifeCycleDisplay();
	        
	        this.addModelsTabs();
		},
		
		postCreate: function(){
			this.inherited(arguments);
		},
		
		addTopCenterPanes: function(){			
	        this.top = new ContentPane({
	        	 region: "top"
	             //content: "some content top"
	        });
	        this.bc.addChild(this.top);
	        
	        this.center = new ContentPane({
	        	 region: "center",
	             //content: "some content center"
	        });
	        this.bc.addChild(this.center);
		},
		
		addModelLifeCycleDisplay: function(){
			var topTable = domConstruct.create("table", {"class":"", style: "width: 80%;"}, this.top.containerNode);
            
            var tr = domConstruct.create("tr", {"class":""}, topTable);

            var td = domConstruct.create("td", {"class":""}, tr);
            var tdiv = domConstruct.create("div", {"class":""}, td);
			//Created on: 
			this.modelLifeCycleCreatedOnTableContainer = new TableContainer({    
                customClass: "viewProperties",
                labelWidth: "160px",
                //style: "float: left;"
            },tdiv);
        
			var modelDate = "Missing!";
			var modelAuthor = "unavailable";
			var modelStatus = "unavailable";
            try{
            	modelDate =	this.model.lifeCycle.date;
            }catch(e){
            	
            }
            try{
            	modelAuthor =	this.model.lifeCycle.author;
            }catch(e){
            	
            }
            try{
            	modelStatus =	this.model.lifeCycle.status + " (" + this.model.lifeCycle.statusAuthor + ")";
            }catch(e1){
            	
            }
            try{
            	if(this.model.workflowId!=null){//Make the discard button to be visible
            		this.addDiscardButton();
            	}
            }catch(e2){
            	
            }
            
            var createdOnTextBox = new DijitTextBox({
                label: "Created on",
                value: modelDate,
                //readOnly: true,
                disabled: true,
                style: "width: 6em",
            });
            createdOnTextBox.placeAt(this.modelLifeCycleCreatedOnTableContainer.containerNode);
            this.modelLifeCycleCreatedOnTableContainer.startup();
            
            td = domConstruct.create("td", {"class":""}, tr);
            tdiv = domConstruct.create("div", {"class":""}, td);
            // Created by:
            this.modelLifeCycleCreatedByTableContainer = new TableContainer({    
                customClass: "viewProperties",
                labelWidth: "160px",
                //style: "float: left; margin-left: 100px;"
            },tdiv);
        
            var createdByTextBox = new DijitTextBox({
                label: "Created by",
                value: modelAuthor,
                //readOnly: true,
                disabled: true,
                style: "width: 20em",
            });
            createdByTextBox.placeAt(this.modelLifeCycleCreatedByTableContainer.containerNode);
            this.modelLifeCycleCreatedByTableContainer.startup();            
            
            td = domConstruct.create("td", {"class":""}, tr);
            tdiv = domConstruct.create("div", {"class":""}, td);
            // Status:
            this.modelLifeCycleStatusTableContainer = new TableContainer({    
                customClass: "viewProperties",
                labelWidth: "160px",
                //style: "float: left; margin-left: 100px;"
            },tdiv);
        
            var statusTextBox = new DijitTextBox({
                label: "Status",
                value: modelStatus,
                //readOnly: true,
                disabled: true,
                style: "width: 20em",
            });
            statusTextBox.placeAt(this.modelLifeCycleStatusTableContainer.containerNode);
            this.modelLifeCycleStatusTableContainer.startup();
            
            tr = domConstruct.create("tr", {"class":""}, topTable);
            td = domConstruct.create("td", {"class":""}, tr);
            tdiv = domConstruct.create("div", {"class":""}, td);
            //Strain
            this.modelmpglsTableContainer = new TableContainer({    
                customClass: "viewProperties",
                labelWidth: "160px",
                //style: "float: left; margin-left: 100px;"
            },tdiv);
            
            this.modelPglsTextBox = new DijitTextBox({
                label: "Model Global Peak LS",
                //readOnly: true,
                disabled: true,
                style: "width: 5em",
            });
            this.modelPglsTextBox.placeAt(this.modelmpglsTableContainer.containerNode);
            this.modelmpglsTableContainer.startup(); 
            
            td = domConstruct.create("td", {"class":""}, tr);
            tdiv = domConstruct.create("div", {"class":""}, td);
            this.specklempglsTableContainer = new TableContainer({    
                customClass: "viewProperties",
                labelWidth: "160px",
                //style: "float: left; margin-left: 100px;"
            },tdiv);
            if(this.usmodel){
	            this.specklePglsTextBox = new DijitTextBox({
	                label: "Speckle Global Peak LS",
	                //readOnly: true,
	                disabled: true,
	                style: "width: 5em",
	            });
	            this.specklePglsTextBox.placeAt(this.specklempglsTableContainer.containerNode);
	            this.specklempglsTableContainer.startup(); 
            }
            td = domConstruct.create("td", {"class":""}, tr);
            tdiv = domConstruct.create("div", {"class":""}, td);
            this.modelejfTableContainer = new TableContainer({    
                customClass: "viewProperties",
                labelWidth: "160px",
                //style: "float: left; margin-left: 100px;"
            },tdiv);
            this.ejfTextBox = new DijitTextBox({
                label: "Ejection Fraction",
                //readOnly: true,
                disabled: true,
                style: "width: 5em",
            });
            this.ejfTextBox.placeAt(this.modelejfTableContainer.containerNode);
            this.modelejfTableContainer.startup();
            
            var studyLinkButton = new Button({
                label: "Go to Study",
                showLabel: true,
                //iconClass: "playIcon",
                style: "float: right;",
                onClick: lang.hitch( this, function(){ 
                	topic.publish("/link", this.model.studyUid);
                })
            });
            studyLinkButton.placeAt(this.top.containerNode);

		},
		
		addModelsTabs: function(){
		    this.tabs = new TabContainer({
                style: "height: 100%; width: 100%;",
                tabStrip: true
            });
            this.tabs.placeAt(this.center.containerNode);
            this.tabs.startup();  
 
            // Fix up tabstrip rendering in Chrome
            this.tabs.tablist.tablistWrapper.firstElementChild.style.cssText = "width: 100%;";
		},
		
		onShow: function(){
			if(!this.analysisTab){
	            this.addAnalysisTab();
	            this.addWorksheetTab();
			}else{
				//this.onAnalysisTabShow();
			}
		},
		
		addAnalysisTab: function(){
		    this.analysisTab = new ContentPane({
                 title: "Analysis",
            });
            this.tabs.addChild(this.analysisTab);
            
            this.createAnalysisTabContents();
            
            //!! This is the switch for the zinc plugin re-attach behaviour
            //!! Currently using non re-attaching mode for all non-Mozilla browsers eg Chrome
/*            if(!dojo.isMozilla){
            	// Manage explicitly destroying and recreating zinc plugins when the Analysis tab is shown
                this.analysisTab.set("onShow", lang.hitch(this, this.onAnalysisTabShow));
            }else{
            	// Simply add the plugins and let the re-attaching thing do its thing
            	this.addZincPlugins();
            }*/
            this.addZincPlugins();
		},
		
		onAnalysisTabShow:function(){
          // On subsequent onShows() we will create all the zinc plugins again, rather than re-attach
        	if(this.debug){
        		console.log("Creating/Recreating Zinc plugins");
        		console.log("Destroying any existing plugins first");
        	}
            this.removeZincPlugins();
            
            this.addZincPlugins();
        },
        
		createAnalysisTabContents: function(){
			
            this.analysisTabBc = new BorderContainer({
            	liveSplitters: true,
                style: "height: 100%; width: 100%; overflow:hidden"
            });
            this.analysisTab.addChild(this.analysisTabBc);
            
            this.analysisTabCenter = new ContentPane({
                 region: "center",
                 //content: "some content center"
                 style: "padding: 0px;  overflow:hidden; "
            });
            this.analysisTabBc.addChild(this.analysisTabCenter);

            this.analysisTabRight = new ContentPane({
                 region: "right",
                 splitter: true,
                 style: "width: 40%",
                 //content: "some content right"
            });
            this.analysisTabBc.addChild(this.analysisTabRight);
            
            
            this.analysisTabCenterBc = new BorderContainer({
            	design: "sidebar",
            });
            this.analysisTabCenter.addChild(this.analysisTabCenterBc);
          
            this.analysisTabCenterBcCenter = new ContentPane({
                 region: "center",
                 //content: "some content center"
                 style: "padding: 0px; overflow:hidden"
            });
            this.analysisTabCenterBc.addChild(this.analysisTabCenterBcCenter);
            
            
            this.analysisTabCenterBcRight = new ContentPane({
                 region: "right",
                 //content: "some content center"
                 style: "padding: 0px; width: 200px"
            });
            this.analysisTabCenterBc.addChild(this.analysisTabCenterBcRight);

            this.analysisTabCenterBcBottom = new ContentPane({
                 region: "bottom",
                 //content: "some content center"
                 style: "padding: 0px;overflow: hidden;"
            });
            this.analysisTabCenterBc.addChild(this.analysisTabCenterBcBottom);          
            
            
/*            var table = domConstruct.create("table", {"class":"", style: "background-color:black; width: 100%; height: 100%;"}, this.analysisTabCenterBcCenter.containerNode);
            
            var tr = domConstruct.create("tr", {"class":""}, table);

            if(this.model.Regions.TCH){
	            td = domConstruct.create("td", {"class":"", style: dojo.isMozilla ? "" : ""}, tr);
	            this.tchDiv = domConstruct.create("div", {"class":"", style: "width: 100%; height: 100%;"}, td);
            }
            if(this.model.Regions.FCH){
	            td = domConstruct.create("td", {"class":"", style: dojo.isMozilla ? "" : ""}, tr);
	            this.fchDiv = domConstruct.create("div", {"class":"", style: "width: 100%; height: 100%;"}, td);
            }
            
            td = domConstruct.create("td", {"class":"", rowspan: "2", style: dojo.isMozilla ? "height: 100%;" : "height: 100%;"}, tr);
            this.modelDiv = domConstruct.create("div", {"class":"", style: "width: 100%; height: 100%;"}, td);

            if(this.model.Regions.SAX || this.model.Regions.APLAX && !(this.model.Regions.TCH || this.model.Regions.FCH)){
            	tr = domConstruct.create("tr", {"class":""}, table);
            }
            
            if(this.model.Regions.SAX){
	            td = domConstruct.create("td", {"class":"", style: dojo.isMozilla ? "overflow:hidden" : ""}, tr);
	            this.saxDiv = domConstruct.create("div", {"class":"", style: "width: 100%; height: 100%;"}, td);
            }
            if(this.model.Regions.APLAX){
	            td = domConstruct.create("td", {"class":"", style: dojo.isMozilla ? "overflow:hidden" : ""}, tr);
	            this.aplaxDiv = domConstruct.create("div", {"class":"", style: "width: 100%; height: 100%;"}, td);
            }*/
            
            this.zincwindowsbc = new BorderContainer({
            	liveSplitters: true,
                style: "height: 100%; width: 100%;",
                design: "sidebar"
            });

            if(this.usmodel){//Projections are not required
	            var mainModelPane = new ContentPane({
	                region: "right",
	                splitter: true,
	                style: "width: 40%; border: 0px; padding: 0px; top: 0px; left: 0px;",
	            });
	            this.zincwindowsbc.addChild(mainModelPane);
	            this.modelDiv = domConstruct.create("div", {"class":"", style: "width: 100%; height: 100%;"}, mainModelPane.containerNode);

	            var projectionPane = new ContentPane({
	                region: "center",
	                splitter: true,
	                style: "width: 60%; border: 0px; padding: 0px; top: 0px; left: 0px;",
	            });
	            this.zincwindowsbc.addChild(projectionPane);
	            var viewRegions = new Array("APLAX","TCH","FCH","SAXAPEX","SAXMID","SAXBASE");
	            var divs = new Array(null,null,null,null,null,null);
	            var table = domConstruct.create("table", {"class":"", style: "background-color:black; width: 100%; height: 100%;"}, projectionPane.containerNode);
	            var tr = domConstruct.create("tr", {"class":""}, table);
	            var numRegions = 0;
	            for(var k=0;k<viewRegions.length;k++){
	            	if(this.model.Regions[viewRegions[k]]!=undefined){
	            		numRegions++;
	            		td = domConstruct.create("td", {"class":"", style: ""}, tr);
	    	            divs[k] = domConstruct.create("div", {"class":"", style: "width: 100%; height: 100%;"}, td);
	            		if(numRegions%2==0){
	            			tr = domConstruct.create("tr", {"class":""}, table);
	            		}
	            	}
	            }
	            this.aplaxDiv = divs[0];
	            this.tchDiv = divs[1];
	            this.fchDiv = divs[2];
	            this.saxApexDiv = divs[3];
	            this.saxMidDiv = divs[4];
	            this.saxBaseDiv = divs[5];
            }else{
            	var mainModelPane = new ContentPane({
	                region: "left",
	                style: "width: 80%;  border: 0px; padding: 0px; top: 0px; left: 0px;",
	            });
	            this.zincwindowsbc.addChild(mainModelPane);
	            this.modelDiv = domConstruct.create("div", {"class":"", style: "width: 100%; height: 100%;"}, mainModelPane.containerNode);
	            var viewSelectionPane = new ContentPane({
	                region: "right",
	                style: "width: 20%; border: 0px; padding: 0px; top: 0px; left: 0px;",
	            });
	            this.zincwindowsbc.addChild(viewSelectionPane);
	            var table = domConstruct.create("table", {"class":"", style: "width: 100%; height: 100%;"}, viewSelectionPane.containerNode);
	            var tr = domConstruct.create("tr", {"class":""}, table);
	            domConstruct.create("td", {"class":"", style: "text-align: center",innerHTML:"Axis Planes"}, tr);
	            tr = domConstruct.create("tr", {"class":""}, table);
	            domConstruct.create("td", {"class":"", style: "float:left",innerHTML:"Short"}, tr);
	            //tr = domConstruct.create("tr", {"class":""}, table);
	            var tds = domConstruct.create("td", {"class":"", style: "float:right"}, tr);
	            tr = domConstruct.create("tr", {"class":""}, table);
	            domConstruct.create("td", {"class":"", style: "float:left",innerHTML:"Long"}, tr);
	            //tr = domConstruct.create("tr", {"class":""}, table);
	            var tdl = domConstruct.create("td", {"class":"", style: "float:right"}, tr);
	            var sv = new Array();
	            var lv = new Array();
	            for(var k in this.model.Regions){
	            	var item = this.model.Regions[k];
	            	var obj = {value:item.seriesid,label:item.planeid};
	            	if(item.type!=undefined){ //Undefined for mesh
		            	if(item.type=="SHORT"){
		            		sv.push(obj);
		            	}else{
		            		lv.push(obj);
		            	}
	            	}
	            }
	            
	            
	            var saxDataStore = new ItemFileReadStore({
	    			data : {
	    				identifier: "value",
	    				label: "label",
	    				items: sv
	    			}
	    		});
	            
	            var laxDataStore = new ItemFileReadStore({
	    			data : {
	    				identifier: "value",
	    				label: "label",
	    				items: lv
	    			}
	    		});

	            
	            
	            var MyCheckedMultiSelect = declare(CheckedMultiSelect, {
	                
	                startup: function() {
	                    this.inherited(arguments);  
	                    setTimeout(lang.hitch(this, function() {
	                    	//Ensure that the label at the begining says Showing all as all views are shown
	                        this.dropDownButton.set("label", "Showing All");            
	                    }));
	                },
	                
	                _updateSelection: function() {
	                    this.inherited(arguments);                
	                    if(this.dropDown && this.dropDownButton){
	                        var label = "";
	                        var sel = 0;
	                        array.forEach(this.options, function(option){
	                            if(option.selected){
	                                //label += (label.length ? ", " : "") + option.label;
	                            	sel = sel + 1;
	                            }
	                        });
	                        if(sel>0)
	                        	label = "Showing "+sel;
	                        this.dropDownButton.set("label", label.length ? label : this.label);
	                    }
	                }
	                    
	            });
	            
	            
	            var shortAxisSelection = new MyCheckedMultiSelect({
	                dropDown: true,
	                label: 'Showing 0',
	                multiple: true,
	                name: 'shortAxis',
	                store: saxDataStore,
	                onChange: lang.hitch(this,this.sceneSelection,"short")
	            });
	            shortAxisSelection.placeAt(tds);
	            shortAxisSelection.startup();
	                        
	            var longAxisSelection = new MyCheckedMultiSelect({
	                dropDown: true,
	                label: 'Showing 0',
	                multiple: true,
	                name: 'longAxis',
	                store: laxDataStore,
	                onChange: lang.hitch(this,this.sceneSelection,"Long")
	            });	
	            longAxisSelection.placeAt(tdl);
	            longAxisSelection.startup();

	            
            }
     
            this.zincwindowsbc.startup();
            
            this.analysisTabCenterBcCenter.addChild(this.zincwindowsbc);
            
            this.modelControlDiv = domConstruct.create("div", {"class":""}, this.analysisTabCenterBcRight.containerNode);

            this.playerDiv = domConstruct.create("div", {"class":""}, this.analysisTabCenterBcBottom.containerNode);
            
            //this.addZincPlugins();            
            
            this.addModelControls();
            
            this.addPlayer();
            
            this.addBullsEye();
            
            this.addPlotsTabs();
            
            this.addClearButton();
            
            this.addSaveButton();
            
		},
		
	
		addModelControls:function(){

			this.modeControlsFieldset = domConstruct.create("div", {"class":"modelControlsFieldset"}, this.modelControlDiv);
            
            this.modeControlsTableContainer = new TableContainer({    
                customClass: "viewProperties",
                labelWidth: "160px"
            });
        
            if(this.usmodel){
	            this.APLAXcheckBoxDiv = domConstruct.create("div", null, this.modeControlsTableContainer.containerNode);
	            this.APLAXcheckBox = new DijitCheckBox({
	                label: "APLAX",
	                checked: true,
	                onChange: lang.hitch(this, this.onAPLAXCheckbox)
	            });
	            this.APLAXcheckBox.placeAt(this.APLAXcheckBoxDiv);
	
	            this.FCHcheckBoxDiv = domConstruct.create("div", null, this.modeControlsTableContainer.containerNode);
	            this.FCHcheckBox = new DijitCheckBox({
	                label: "4CH",
	                checked: true,
	                onChange: lang.hitch(this, this.on4CHCheckbox)
	            });
	            this.FCHcheckBox.placeAt(this.FCHcheckBoxDiv);
	
	            this.TCHcheckBoxDiv = domConstruct.create("div", null, this.modeControlsTableContainer.containerNode);
	            this.TCHcheckBox = new DijitCheckBox({
	                label: "2CH",
	                checked: true,
	                onChange: lang.hitch(this, this.on2CHCheckbox)
	            });
	            this.TCHcheckBox.placeAt(this.TCHcheckBoxDiv);
	
	            this.SAXcheckBoxDiv = domConstruct.create("div", null, this.modeControlsTableContainer.containerNode);
	            this.SAXcheckBox = new DijitCheckBox({
	                label: "SAX",
	                checked: true,
	                onChange: lang.hitch(this, this.onSAXCheckbox)
	            });
	            this.SAXcheckBox.placeAt(this.SAXcheckBoxDiv);
            }
            checkBox = new DijitCheckBox({
                label: "Surface",
                checked: true,
                onChange: lang.hitch(this, this.onSurfaceCheckbox)
            });
            checkBox.placeAt(this.modeControlsTableContainer.containerNode);

            checkBox = new DijitCheckBox({
                label: "Lines",
                checked: true,
                onChange: lang.hitch(this, this.onLinesCheckbox)
                
            });
            checkBox.placeAt(this.modeControlsTableContainer.containerNode);
            
            checkBox = new DijitCheckBox({
                label: "Fibres",
                checked: false,
                onChange: lang.hitch(this, this.onFibresCheckbox)
            });
            checkBox.placeAt(this.modeControlsTableContainer.containerNode);


/*            checkBox = new DijitCheckBox({
                label: "LandMarks",
                checked: false,
                onChange: lang.hitch(this, this.onLandMarksCheckbox)
            });
            checkBox.placeAt(this.modeControlsTableContainer.containerNode);*/

            
            this.modeControlsTableContainer.placeAt(this.modeControlsFieldset);
            this.modeControlsTableContainer.startup();
		},
		
		addPlayer: function(){
			var playStopDiv = domConstruct.create("div", {style: "margin-bottom: 5px; float: left"}, this.playerDiv);
            this.playButton = new Button({
                label: "Play",
                showLabel: false,
                iconClass: "playIcon",
                onClick: lang.hitch( this, function(){ 
                    topic.publish("/" + this.uuid + "/PlayStop");
                })
            });
            this.playButton.placeAt(playStopDiv);
            
			var playSliderDiv = domConstruct.create("div", {"class":"", style : "width: 100%;padding-top: 8px;"}, this.playerDiv);

			this.time = 0; //!!!this.model.startTime; 
            var playSlider = HorizontalSlider({
                value: 0,
                minimum: this.time, 
                maximum: this.maxcapframeid, //!!!this.model.endTime, 
                intermediateChanges: true,
                onChange: lang.hitch( this, function(value){ 
                	this.time = value;
                    topic.publish("/" + this.uuid + "/Time", value);
                })
            });
            playSlider.placeAt(playSliderDiv);
            
            topic.subscribe("/" + this.uuid + "/Controller/Time", function(value){
                    playSlider.set("value", value, false);
            });	
            
            domConstruct.create("br",null,this.playerDiv);            
            
            var epochDiv = domConstruct.create("div", {
            	style: "margin: 0 auto;padding-left: 10px;",
            	}, this.playerDiv);
            
            domConstruct.create("label",{float:"left",innerHTML: "Set epoch as Mitral valve"},epochDiv);
            
            this.mvoButton = new Button({
                label: "Opening",
                showLabel: true,
                onClick: lang.hitch( this, function(){ 
                	this.mvo = this.time;
                    topic.publish("/" + this.uuid + "/setmvo",this.mvo);
                })
            });
            this.mvoButton.placeAt(epochDiv);
            this.mvcButton = new Button({
                label: "Closing",
                showLabel: true,
                onClick: lang.hitch( this, function(){ 
                	this.mvc = this.time;
                    topic.publish("/" + this.uuid + "/setmvc",this.mvc);
                })
            });
            this.mvcButton.placeAt(epochDiv);
            
            br = domConstruct.create("br",null,this.playerDiv);
            

            var animationDiv = domConstruct.create("div", {style:"padding-left: 10px;"}, this.playerDiv);
            
            domConstruct.create("label",{
            	style: "margin-bottom: 5px; float: left",
            	innerHTML: "Animation Speed"	
            	},animationDiv);
            
			var animationSpeedDiv = domConstruct.create("div", {"class":"", style : "width: 70%;padding-top: 4px;float:right;"}, animationDiv);

			this.rate = 0.5; //!!!this.model.increment rate
            var animationSpeedSlider = HorizontalSlider({
                value: 5,
                minimum: 0, 
                maximum: 30, //!!!this.model.endTime, 
                intermediateChanges: true,
                onChange: lang.hitch( this, function(value){ 
                	this.rate = value*0.1;
                })
            });
            animationSpeedSlider.placeAt(animationSpeedDiv);
            
            //mitral valve controls
            //hr = domConstruct.create("hr",null,this.playerDiv);
            
		},
		
		playStop: function(){
			if(!this.playerId){
            	if(this.debug)
            		console.log("Play");
                this.playButton.set("iconClass", "pauseIcon");
                this.playButton.set("label", "Pause");
	            this.playerId = setInterval(lang.hitch(this, function(){
	                this.time = this.time + this.rate;
	                if(this.time > this.maxcapframeid){ //!! this.model.endTime
	                    this.time = 0; //!!this.model.startTime; 
	                }
	                topic.publish("/" + this.uuid + "/Controller/Time", this.time);
	            }), 50); // 50ms for attempted 20fps     
            }else{
            	if(this.debug)
            		console.log("Stop");
                clearInterval(this.playerId);
                this.playerId = null;
                this.playButton.set("iconClass", "playIcon");
                this.playButton.set("label", "Play");
            }     
        },
        
        addBullsEye: function(){
        	
        	domConstruct.create("div", {innerHTML:'<b style="margin-left: 70px;">Anterior</b>'}, this.modelControlDiv);
        	
        	domConstruct.create("img", {
        		src:"ABI/app/ICMA/images/bullseye-200.png", 
        		usemap: "#bullseyemap" + this.uuid
        	}, this.modelControlDiv);
        	
            var map = domConstruct.create("map", {
                name: "bullseyemap" + this.uuid
            }, this.modelControlDiv);
        	
            domConstruct.create("area", {
                shape: "poly",
                segment: "2",
                //coords:"51,18,35,30,22,47,13,63,6,83,4,103,34,103,36,87,42,74,49,62,55,54,62,49,66,45",
                coords:"150,20,126,9,96,7,72,11,56,19,71,48,82,43,97,39,113,39,130,44,136,48",
                onclick: lang.hitch(this, function(){
                	this.clickedBullsEyeSegment(2);
                })
            }, map);
        	
            domConstruct.create("area", {
                shape: "poly",
                alt: "",
                segment: "1",
                //coords: "147,17,123,6,93,4,69,8,53,16,68,45,79,40,94,36,110,36,127,41,133,45",
                coords: "201,104,200,87,193,63,182,44,173,32,154,18,140,45,152,55,160,67,167,79,170,96,171,104",
                onclick: lang.hitch(this, function(){
                    this.clickedBullsEyeSegment(1);
                })
            }, map);

            domConstruct.create("area", {
                shape: "poly",
                alt: "",
                segment: "6",
                //coords: "196,104,195,87,188,63,177,44,168,32,149,18,135,45,147,55,155,67,162,79,165,96,166,104",
                coords: "138,163,153,191,170,177,185,159,195,137,199,116,200,104,170,105,169,121,163,137,152,152",
                onclick: lang.hitch(this, function(){
                    this.clickedBullsEyeSegment(6);
                })
            }, map);
            
            domConstruct.create("area", {
                shape: "poly",
                alt: "",
                segment: "5",
                //coords: "134,165,149,193,166,179,181,161,191,139,195,118,196,106,166,107,165,123,159,139,148,154",
                coords: "53,194,71,203,94,207,116,206,132,201,147,194,133,167,114,174,96,175,81,172,68,166",
                onclick: lang.hitch(this, function(){
                    this.clickedBullsEyeSegment(5);
                })
            }, map);
            
            domConstruct.create("area", {
                shape: "poly",
                alt: "",
                segment: "4",
                //coords: "53,194,71,203,94,207,116,206,132,201,147,194,133,167,114,174,96,175,81,172,68,166",
                coords: "46,192,61,165,45,151,37,138,31,121,29,107,-1,107,2,133,8,149,17,165,30,180",
                onclick: lang.hitch(this, function(){
                    this.clickedBullsEyeSegment(4);
                })
            }, map);
            
            domConstruct.create("area", {
                shape: "poly",
                alt: "",
                segment: "3",
                //coords: "51,192,66,165,50,151,42,138,36,121,34,107,4,107,7,133,13,149,22,165,35,180",
                coords: "48,21,32,33,19,50,10,66,3,86,1,106,31,106,33,90,39,77,46,65,52,57,59,52,63,48",
                onclick: lang.hitch(this, function(){
                    this.clickedBullsEyeSegment(3);
                })
            }, map);
            
            domConstruct.create("area", {
                shape: "poly",
                alt: "",
                segment: "8",
                //coords: "38,104,67,104,68,91,76,81,82,75,68,49,52,62,41,83",
                coords: "72,48,87,74,99,71,108,71,118,74,133,47,114,41,89,41",
                onclick: lang.hitch(this, function(){
                    this.clickedBullsEyeSegment(8);
                })
            }, map);
            
            domConstruct.create("area", {
                shape: "poly",
                alt: "",
                segment: "7",
                //coords: "70,47,85,73,97,70,106,70,116,73,131,46,112,40,87,40",
                coords: "135,50,121,76,131,86,136,97,137,105,165,105,164,86,154,67,145,57",
                onclick: lang.hitch(this, function(){
                    this.clickedBullsEyeSegment(7);
                })
            }, map);
            
            domConstruct.create("area", {
                shape: "poly",
                alt: "",
                segment: "12",
                //coords: "132,49,118,75,128,85,133,96,134,104,162,104,161,85,151,66,142,56",
                coords: "133,105,162,105,159,125,152,141,138,157,131,160,117,135,127,126,135,103",
                onclick: lang.hitch(this, function(){
                    this.clickedBullsEyeSegment(12);
                })
            }, map);
            
            domConstruct.create("area", {
                shape: "poly",
                alt: "",
                segment: "11",
                //coords: "134,106,163,106,160,126,153,142,139,158,132,161,118,136,128,127,133,116",
                coords: "131,164,117,137,105,140,95,140,84,137,70,164,88,170,109,172",
                onclick: lang.hitch(this, function(){
                    this.clickedBullsEyeSegment(11);
                })
            }, map);
            
            domConstruct.create("area", {
                shape: "poly",
                alt: "",
                segment: "10",
                //coords: "131,164,117,137,105,140,95,140,84,137,70,164,88,170,109,172",
                coords: "66,165,81,138,75,131,69,122,65,109,37,109,39,127,45,141,53,153",
                onclick: lang.hitch(this, function(){
                    this.clickedBullsEyeSegment(10);
                })
            }, map);
            
            domConstruct.create("area", {
                shape: "poly",
                alt: "",
                segment: "9",
                //coords: "67,163,82,136,76,129,70,120,66,107,38,107,40,125,46,139,54,151",
                coords: "37,105,66,105,67,92,75,82,81,76,67,50,51,63,40,84",
                onclick: lang.hitch(this, function(){
                    this.clickedBullsEyeSegment(9);
                })
            }, map);
            
            domConstruct.create("area", {
                shape: "poly",
                alt: "",
                segment: "13",
                //coords: "75,88,101,104,114,77,101,72,81,78",
                coords: "104,94,103,76,120,74,132,87,128,106,110,105",
                onclick: lang.hitch(this, function(){
                    this.clickedBullsEyeSegment(13);
                })
            }, map);
            
            domConstruct.create("area", {
                shape: "poly",
                alt: "",
                segment: "16",
                //coords: "103,105,117,78,128,89,132,101,130,113,128,119",
                coords: "112,108,131,106,132,128,119,138,98,137,100,120",
                onclick: lang.hitch(this, function(){
                    this.clickedBullsEyeSegment(16);
                })
            }, map);
            
            domConstruct.create("area", {
                shape: "poly",
                alt: "",
                segment: "15",
                //coords: "101,107,126,122,119,131,106,138,94,137,86,134",
                coords: "100,120,96,139,83,133,75,122,68,106,85,107",
                onclick: lang.hitch(this, function(){
                    this.clickedBullsEyeSegment(15);
                })
            }, map);
            
            domConstruct.create("area", {
                shape: "poly",
                alt: "",
                segment: "14",
                //coords: "98,105,84,133,74,122,70,113,70,101,72,91",
                coords: "65,106,89,105,96,95,96,74,75,84",
                onclick: lang.hitch(this, function(){
                    this.clickedBullsEyeSegment(14);
                })
            }, map);
        },
        
        clickedBullsEyeSegment: function(segment){
        	if(this.debug)
        		console.log("Clicked segment: " + segment);
        	topic.publish("/" + this.uuid + "/ToggleSegment", segment);
        },
        
        addClearButton: function(){
        	var clearButtonDiv = domConstruct.create("div",{style : "margin-left: 65px;"},this.modelControlDiv);
            this.clearButton = new Button({
                label: "Clear",
                onClick: lang.hitch( this, function(){ 
                	if(this.clearButton.get( "label") == "Clear"){
                    	if(this.debug)
                    		console.log("Clear clicked");
	                	this.clearButton.set( "label", "Select All");
	                    topic.publish("/" + this.uuid + "/Clear");
                    }else{
                    	if(this.debug)
                    		console.log("Select All clicked");
                    	this.clearButton.set( "label", "Clear");
                    	topic.publish("/" + this.uuid + "/SelectAll");
                    }
                })
            });
            this.clearButton.placeAt(clearButtonDiv);
        },

        addSaveButton: function(){
            this.saveButtonDiv = domConstruct.create("div", {style: "float:right;"}, this.top.containerNode);
            this.saveButton = new Button({
                label: "Save",
                onClick: lang.hitch( this, this.onSave)
            });
            this.saveButton.placeAt(this.saveButtonDiv);
        },
        
        addDiscardButton: function(){
            this.discardButtonDiv = domConstruct.create("div", {style: "float:right;"}, this.top.containerNode);
            this.discardButton = new Button({
                label: "Discard",
                onClick: lang.hitch( this, this.onDiscard)
            });
            this.discardButton.placeAt(this.discardButtonDiv);
        },
        
        addPlotsTabs: function(){
        	this.plotsTabsMaster = new TabContainer({
                style: "height: 100%; width: 100%;",
            });
            this.plotsTabsMaster.placeAt(this.analysisTabRight.containerNode);
            this.plotsTabsMaster.startup();
            //Order the display order of the measures
            var displayOrder = new Object();
            displayOrder["SPECKLE(2D)"] = false;
            displayOrder["SPECKLE(3D)"] = false;
            displayOrder["LAGRANGIAN"] = false;
            displayOrder["FIBRE"] = false;
            displayOrder["TORSION"] = false;
            
            var displayGroups = new Object();
            var gctr = 0;
            //console.debug(this.model.SelectedStrains);
            //Add all other measures
            for(item in this.model.Measures){
            	displayOrder[item] = true;
            	var measure = this.model.Measures[item];
            	if(measure.group){
            		if(!(measure.group in displayGroups)){
            			var plotsTabs = new TabContainer({
                            style: "height: 100%; width: 100%;",
                            nested: true,
                            title: measure.group
                        });
                        this.plotsTabsMaster.addChild(plotsTabs);
                        plotsTabs.startup();
                        displayGroups[measure.group] = plotsTabs;
                        gctr++;
            		}
            	}
            }
            //No groups has been created create a default group
            if(gctr==0){
            	var plotsTabs = new TabContainer({
                    style: "height: 100%; width: 100%;",
                    nested: true,
                    title: "All Measures"
                });
                this.plotsTabsMaster.addChild(plotsTabs);
                plotsTabs.startup();
                displayGroups["default"] = plotsTabs;
                gctr++;
            }
            //Remove unavailable measures
            for(item in displayOrder){
            	if(!displayOrder[item]){
            		delete displayOrder[item];
            	}
            }
            //for(item in this.model.Measures){
            for(item in displayOrder){
            	if(this.debug)
            		console.debug(item);
                var measure = this.model.Measures[item];
                if(item=="modelPeakGlobalLongitudinalStrain"){
                	this.modelPglsTextBox.set('value',this.model.Measures[item]);
                	continue;
                }
                if(item=="specklePeakGlobalLongitudinalStrain"){
                	this.specklePglsTextBox.set('value',100*parseFloat(""+this.model.Measures[item]));
                	continue;
                }
                if(item=="ejectionFraction"){
                	this.ejfTextBox.set('value',this.model.Measures[item]);
                	continue;
                }
                var pTitle = measure.name;
                var scaleFactor = 1.0;
                if(!pTitle){ //Handle other undisplayed items
                	continue;
                }
                if(measure.name=="LAGRANGIAN"){
                	pTitle = "LINEAR(3D)";
                }
                if(pTitle=="SPECKLE(2D)"||pTitle=="SPECKLE(3D)")
                	scaleFactor = 100.0;
               //Get the corresponding group tab
                var parentTab = displayGroups["default"];
                if(measure.group){
                	parentTab = displayGroups[measure.group];
                }
                
                var plotTab = new ContentPane({
                    title: pTitle
                });
                //this.plotsTabs.addChild(plotTab);
                parentTab.addChild(plotTab);
                
                this.analysisTabRightBc = new BorderContainer({
                    style: "height: 100%; width: 100%;",
                    design: "sidebar"
                });
                plotTab.addChild(this.analysisTabRightBc);
          
	            this.analysisTabRightBcCenter = new ContentPane({
                    region: "center",
	                 //content: "some content center"
	                style: "border: 0px; padding: 0px; top: 0px; left: 0px; overflow: hidden;"
	            });
                this.analysisTabRightBc.addChild(this.analysisTabRightBcCenter);

                aspect.after(this.analysisTabRightBcCenter, "resize", function(){
                    if(this.chart){
                    		if(this.debug){
	                            console.log("Resizing Chart after resize");
	                            console.log("Resizing Chart " + this.h);
                    		}
                            domStyle.set(this.domNode, "top", "0px");
                            domStyle.set(this.domNode, "left", "0px");
                            this.chart.resize();
                    }
                });

                this.analysisTabRightBcRight = new ContentPane({
                     region: "right",
                     //content: "some content center"
                     style: "padding: 0px; width: 4em;"
                });
                this.analysisTabRightBc.addChild(this.analysisTabRightBcRight);
            
            
	            this.analysisTabRightBcBottom = new ContentPane({
	                 region: "bottom",
	                 //content: "some content center"
	                 style: "padding: 0px;"
	            });
	            this.analysisTabRightBc.addChild(this.analysisTabRightBcBottom);
                
                // Convert data strings from the server to arrays for plotting
                // data arrays for plotting are store in:
                // data[wallDepth][segment]
	            //Determine the range to enable appropriate y-axis setting
/*	            var max = -1.0e+300;
	            var min = 1.0e+300;*/
	            var average = new Array();
                var data = new Array();
                for(var wallDepth = 0; wallDepth < measure.numwallsegments; wallDepth++){
                	var wall = measure.timeseries["wall" + wallDepth + ".0"];
                	if(this.debug)
                		if(wallDepth==0){
                			console.debug(wall);
                		}
                	if(wall){ //Some measures do not have values across the wall
	                	data[wallDepth] = new Array();
	                	var avgIdx = 18;
	                	if(wall[19]!=undefined){
	                		avgIdx = 19;
	                	}
	                	for(var k in wall){
	                		if(!isNaN(k)){
								try{
			                		var dataStrings = wall[k].split(",");
			                		data[wallDepth][k] = new Array();
			                		for(var i = 0; i < dataStrings.length; i++){
			                			var value = parseFloat(dataStrings[i]);
			                            data[wallDepth][k][i] = scaleFactor*value;
/*			                            if(max<value){
			                            	max = value;
			                            }
			                            if(min>value){
			                            	min = value;
			                            }*/
			                		}
								}catch(e){
									
								}
	                		}
	                    }
	                	if(this.debug)
	                		console.debug('Average Index '+measure.name+"\t"+avgIdx);
                		average[wallDepth] = data[wallDepth][avgIdx];
                	}
                }
                
/*               	max = max*1.1;
               	min = min*0.9;

                console.debug(measure.name+"\t"+max+"\t"+min);*/
                //this.analysisTabRightBcCenter.yRange = [min,max];
                this.analysisTabRightBcCenter.data = data;
                this.analysisTabRightBcCenter.average = average;
                this.analysisTabRightBcCenter.measure = measure;
                this.analysisTabRightBcCenter.plotColours = this.plotColours;
                
               // var plotDiv = domConstruct.create("div", {}, this.analysisTabRightBcCenter.containerNode);
               
               //this.analysisTabRightBcCenter.segmentVisable = [0,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true];
                this.analysisTabRightBcCenter.segmentVisable = lang.clone(this.model.SelectedStrains);
               //Set this based on the views available
           	   if(this.debug){
           		   console.debug("Model End time \t"+this.model.endTime);
           	   	   console.debug("Model Start time \t"+this.model.startTime);
           	   }
                var chartDiv = domConstruct.create("div", {"class":"", style: "width: 100%; height: 100%;"}, this.analysisTabRightBcCenter.containerNode);               
            
                this.analysisTabRightBcCenter.chart = new Chart(chartDiv, {});
                this.analysisTabRightBcCenter.chart.addPlot("default", {type: Lines, tension: "S"});

                this.analysisTabRightBcCenter.chart.addAxis("x", {
                    title: "Time msecs", 
                    titleGap: 0,
                    titleOrientation: "away",
                    majorTickStep: 1, // setting major tick step to 1 and minorTicks false is to make it work correctly under FF. If not only major ticks rendered of which our end time is not one
                    minorTicks: false,
                    labelFunc: lang.hitch(this, function(formattedValue, value){
                    	var label = " ";
                    	if(value % 2 != 0){
	                    	var TotalTime = this.model.endTime - this.model.startTime;
	                    	var interval = TotalTime/this.maxcapframeid;
	                    	label = this.model.startTime + (value - 1)*interval;
	                    	label = label.toFixed(2).toString();
                    	}
                    	return label;
                    })
                });
                this.analysisTabRightBcCenter.chart.addAxis("y", {
                													vertical: true,
                								                    minorTicks: false,
                								                    //range : {lower : this.analysisTabRightBcCenter.yRange[0], upper : this.analysisTabRightBcCenter.yRange[1]},
                													labelFunc: lang.hitch(this, function(formattedValue, value){
                								                    	var label = value.toPrecision(3);
                								                    	if(Math.abs(value) > 1000.0){
                								                    		var precv = parseFloat(value.toPrecision(3));
                								                    		label = precv.toExponential(); 
                								                    	}
                								                    	return label;
                								                    })
                								             });
                var ulimit = measure.numsegments;;
                if(ulimit>16)
                	ulimit = 16;
                if(ulimit>1){
	                for(var segment = 1; segment <= ulimit; segment++){
	                	try{
				            if(this.analysisTabRightBcCenter.segmentVisable[segment]){
				               this.analysisTabRightBcCenter.chart.addSeries("" + segment, this.analysisTabRightBcCenter.data[0][segment], {stroke: {color: this.plotColours[segment]}});
				            }
		                }catch(e){
		                		
		                }
	                }
	                //Show the Average
	                if(average[0]!=null){
	                	  this.analysisTabRightBcCenter.chart.addSeries("A", this.analysisTabRightBcCenter.average[0], {stroke: {color: "#000000", style:"Dash"}});
	                }
                }else{ //For single series measures, use first charecter of measure as series name
                	try{
                	    	
		               this.analysisTabRightBcCenter.chart.addSeries(pTitle.charAt(0), this.analysisTabRightBcCenter.data[0][1], {stroke: {color: this.plotColours[5]}});
	                }catch(e){
	                		
	                }
                }
                this.analysisTabRightBcCenter.chart.render();
                var legend = new Legend({
	                	chart: this.analysisTabRightBcCenter.chart, 
	                	horizontal: false,
	                	style: "float: right;"
	                });
                domConstruct.place(legend.domNode, this.analysisTabRightBcRight.containerNode);
                this.analysisTabRightBcCenter.legend = legend;
                
                
                // Epi Endo Slider
                var table = domConstruct.create("table", {"class":"", style: "width: 100%; height: 100%;"}, this.analysisTabRightBcBottom.containerNode);
            
                var tr = domConstruct.create("tr", {"class":""}, table);
            
                var td = domConstruct.create("td", {"class":""}, tr);
                domConstruct.create("div", {"class":"", innerHTML: "Endo", style: ""}, td);

                td = domConstruct.create("td", {"class":"", style: "width: 100%"}, tr);
                var sliderDiv = domConstruct.create("div", {"class":"", style: "width: 100%; height: 100%;"}, td);

                td = domConstruct.create("td", {"class":""}, tr);
                domConstruct.create("div", {"class":"", innerHTML: "Epi", style: ""}, td);
                
                        
                this.depth = 0;
                var epiEndoSlider = HorizontalSlider({
	                value: 0,
	                minimum: 0, 
	                maximum: measure.numwallsegments - 1, 
	                intermediateChanges: true,
	                discreteValues: measure.numwallsegments,
	                onChange: lang.hitch( this.analysisTabRightBcCenter, function(value){ 
	                    this.depth = value;
	                    topic.publish("/" + this.uuid + "/Depth", value);
	                    //console.log("Depth: " + value);
	                    if(this.measure.numwallsegments>1){//Only for data with data across segments
		                    var ulimit = this.measure.numsegments;
		                    if(ulimit>16)
		                    	ulimit = 16;
		                    if(ulimit>1){
			                    for(var segment = 1; segment <= ulimit; segment++){
			                       if(this.segmentVisable[segment]){
			                           this.chart.updateSeries(parseInt(segment), this.data[value][segment]);
			                       }
			                    }
			                    //Update average
			                    if(average[value]!=null){
			                    	this.chart.updateSeries(17, this.average[value]);
			                    }
		                    }else{
		                    	this.chart.updateSeries(this.measure.name.charAt(0), this.data[value][1]);
		                    }
		                    this.chart.render();
		                    this.legend.refresh();
	                    }
	                })
	            });
                epiEndoSlider.placeAt(sliderDiv);

	                topic.subscribe("/" + this.uuid + "/ToggleSegment", lang.hitch(this.analysisTabRightBcCenter, function(segment){
	                	if(this.measure.numsegments > 1){
		                	if(this.segmentVisable[segment]){
		                		this.chart.removeSeries(parseInt(segment));
		                        this.chart.render();
		                        this.legend.refresh();
		                        this.segmentVisable[segment] = false;
		                	}else{
		                		if(!this.depth){
		                			this.depth = 0;
		                		}
		                		if(this.data[this.depth][segment]){
			                		this.chart.addSeries("" + segment, this.data[this.depth][segment], {stroke: {color: this.plotColours[segment]}});
			                        this.chart.render();
			                        this.legend.refresh();
			                		this.segmentVisable[segment] = true;
		                		}
		                	}
	                	}
	                }));
	
	                topic.subscribe("/" + this.uuid + "/Clear", lang.hitch(this.analysisTabRightBcCenter, function(){
	                	if(this.measure.numsegments > 1){
	                		var uLimit = this.measure.numsegments;
	                		if(uLimit>16)
	                			uLimit = 16;
		                	for(var segment = 1; segment <= uLimit; segment++){
		                        if(this.segmentVisable[segment]){
			                        this.chart.removeSeries(parseInt(segment));
			                        this.segmentVisable[segment] = false;
		                        }
		                    }
		                	
		                    this.chart.render();
		                    this.legend.refresh();
	                	}
	                }));
	
	                topic.subscribe("/" + this.uuid + "/SelectAll", lang.hitch(this.analysisTabRightBcCenter, function(){
	                	if(this.measure.numsegments > 1){
		                    if(this.depth==undefined){
		                            this.depth = 0;
		                    }
	                		var uLimit = this.measure.numsegments;
	                		if(uLimit>16)
	                			uLimit = 16;
		                    for(var segment = 1; segment <= uLimit; segment++){
		                       if(this.data[this.depth][segment]){ //Some segments are absent in speckle measures
			                       this.chart.addSeries("" + segment, this.data[this.depth][segment], {stroke: {color: this.plotColours[segment]}});
			                       this.segmentVisable[segment] = true;
		                       }
		                    }
		                    this.chart.render();
		                    this.legend.refresh();
	                	}
	                }));
                

                this.analysisTabRightBcCenter.chart.addPlot("indicator", { 
                	type: "Indicator",
//                	labels: "none",
                	values: 1, 
                	lineStroke: { color: "red" }, 
                	precision: 1, 
                	animate: { duration: 0 },
                	labelFunc: lang.hitch(this, function(firstDataPoint, secondDataPoint, fixed, precision){
                        var label = "bob";

                        var TotalTime = this.model.endTime - this.model.startTime;
                        var interval = TotalTime/this.maxcapframeid;

                        label = this.model.startTime + (secondDataPoint[0] - 1)*interval;
                        label = label.toFixed(2).toString();

                        return label;
                	})
                });
                
                this.analysisTabRightBcCenter.chart.addPlot("mvo", { 
                	type: "Indicator",
                	values: 1, 
                	lineStroke: { color: "gray", style: "ShortDash" }, 
                	precision: 1, 
                	animate: { duration: 0 },
                	labelFunc: lang.hitch(this, function(firstDataPoint, secondDataPoint, fixed, precision){
                        var label = "O";
                        if(secondDataPoint[0]==1){
                        	return "";
                        }
                        return label;
                	})
                });
                
                this.analysisTabRightBcCenter.chart.addPlot("mvc", { 
                	type: "Indicator",
                	values: 1, 
                	lineStroke: { color: "black", style: "ShortDash" }, 
                	precision: 1, 
                	animate: { duration: 0 },
                	labelFunc: lang.hitch(this, function(firstDataPoint, secondDataPoint, fixed, precision){
                        var label = "C";
                        if(secondDataPoint[0]==1){
                        	return "";
                        }
                        return label;
                	})
                });
                
                // Update the indicator position with the time player
                topic.subscribe("/" + this.uuid + "/Time", lang.hitch(this.analysisTabRightBcCenter, function(time){
                    var plot = this.chart.getPlot("indicator");
                    if(plot){
                    	try{
	                        plot.opt.values = [time + 1];
	                        plot.dirty = true;
	                        this.chart.render(); 
                    	}catch(e){
                    		//console.log("error indicator update");
                    	}
                    }                   
                }));
                
                
                // Update the mvo position 
                topic.subscribe("/" + this.uuid + "/setmvo", lang.hitch(this.analysisTabRightBcCenter, function(time){
                    var plot = this.chart.getPlot("mvo");
                    if(plot){
                    	try{
	                        plot.opt.values = [time+1];
	                        plot.dirty = true;
	                        this.chart.render(); 
                    	}catch(e){
                    		//console.log("error indicator update");
                    	}
                    }                   
                }));
                
                // Update the mvc indicator position 
                topic.subscribe("/" + this.uuid + "/setmvc", lang.hitch(this.analysisTabRightBcCenter, function(time){
                    var plot = this.chart.getPlot("mvc");
                    if(plot){
                    	try{
	                        plot.opt.values = [time+1];
	                        plot.dirty = true;
	                        this.chart.render(); 
                    	}catch(e){
                    		//console.log("error indicator update");
                    	}
                    }                   
                }));
            
            }
            

        },
		
		setTime: function(value) {
			this.modelTime = value; //Used by onLandMarkCheck
        	if(this.debug)
        		console.log("time: " + value);
	        try{
        		this.zincwidgetModel.setTime(value/this.timescalefactor);
	        }catch(e){
	        	this.playStop();
	        }
        },
		
		addWorksheetTab: function(){
			this.worksheetTab = new ContentPane({
                title: "Worksheet",
                onShow: lang.hitch(this, function(){
                	if(!dojo.isMozilla){                 	
                	  //this.removeZincPlugins();
                	}
                })
            });
			
			this.worksheetTabBc = new BorderContainer({
	            	design: "sidebar",
	                style: "height: 100%; width: 100%; overflow: hidden;"
	        });
			this.worksheetTab.addChild(this.worksheetTabBc);
	        this.worksheetTabBcTop = new ContentPane({
	                 region: "top",
	                 //content: "some content center",
	                 style: "padding: 0px; width: 100%;"
	        });
	        this.worksheetTabBc.addChild(this.worksheetTabBcTop);
	        
			var Table = domConstruct.create("table", {"class":""}, this.worksheetTabBcTop.domNode);
			var Tr = domConstruct.create("tr", {"class":""}, Table);
			//Add a checkbox to toggle between set from video or set from box
			var Td = domConstruct.create("td", {"class":""}, Tr);
	        domConstruct.create("div", {"class":"",innerHTML:"Study Type"}, Td);
	        Td = domConstruct.create("td", {"class":""}, Tr);
	        this.studyTypeSelect = new Select({
	            name: "studyType",
	            options: [
	                { label: "Rest", value: "REST", selected: true},
	                { label: "Cardiac Function", value: "CARDIACFUNCTION"},
	                { label: "Exercise", value: "EXERCISE" }
	            ],
	            style: "width: 10em"
	        }).placeAt(Td);
	        
	        
	        
	        Td = domConstruct.create("td", {"class":""}, Tr);
	        domConstruct.create("div", {"class":"",innerHTML:"Special Classification"}, Td);
	        Td = domConstruct.create("td", {"class":""}, Tr);
	        this.classificationTextBox = new DijitTextBox({
                label: "Classification",
                value: "",
                style: "width: 15em",
            });
	        this.classificationTextBox.placeAt(Td);
	        
	        domConstruct.create("div", {"style":"float:right;",innerHTML:"Gravitational Conditioning"}, Td);
	        Td = domConstruct.create("td", {"class":""}, Tr);
	        this.gravityTypeSelect = new Select({
	            name: "gravityType",
	            options: [
	                { label: "Earth", value: "EARTH", selected: true},
	                { label: "In flight", value: "INFLIGHT"},
	                { label: "Post Flight", value: "POSTFLIGHT" }
	            ],
	            style: "width: 10em"
	        }).placeAt(Td);
			
			this.worksheetTabBcCenter = new ContentPane({
                region: "center",
                content: "<b>Model Notes:</b><BR>",
                style: "padding: 0px;"
	        });
	        this.worksheetTabBc.addChild(this.worksheetTabBcCenter);
	        this.textarea = new Textarea({
	            name: "notes",
	            rows: "23",
	            cols: "80",
	            style: "width:auto;"
	        });
	        this.worksheetTabBcCenter.addChild(this.textarea);
 
	        //Populate it with what is returned from server
	        try{
	        	var object = dojo.fromJson(this.model.annotation);
	        	this.studyTypeSelect.set('value',object.studyType);
	        	this.gravityTypeSelect.set('value',object.gravityType);
	        	if(object.classification)
	        		this.classificationTextBox.set('value',object.classification);
	        	if(object.notes)
	        		this.textarea.set('value',object.notes);
	        	if(object.mitralvalveopening){
	        		this.mvo = object.mitralvalveopening;
	        		topic.publish("/" + this.uuid + "/setmvo",this.mvo);
	        	}
	        	if(object.mitralvalveclosing){
	        		this.mvc = object.mitralvalveclosing;
	        		topic.publish("/" + this.uuid + "/setmvc",this.mvc);
	        	}

	        }catch(ex){
	        	console.debug(this.model.annotation);
	        	console.log(ex);
	        }
	        
	        this.worksheetTabBc.startup();
	        
            this.tabs.addChild(this.worksheetTab);
        },
        
		
		onMeshReady: function(zincWidget) {
        	if(this.debug)
        		console.log("Mesh Ready");
			if(!this.regionLoaded['heart']){
				zincWidget.addRegion("heart");//Since the heart region is loaded via the mesh in root region, the region handle is not created, so create
		        //Legacy meshes are in PS coordinates
		        //Meshes after 22 Aug 2013 are in RC coordinates
		        //zincWidget.prolateToRC("heart", "coordinates", "rc_coordinates");
		        //zincWidget.projectRegionCoordinates("heart", "rc_coordinates", this.model.Transforms.MESH);
		        zincWidget.projectRegionCoordinates("heart", "coordinates", this.model.Transforms.MESH);
		        this.regionLoaded['heart'] = true;
		        
		        zincWidget.addVisibleRegion("heart:surface:lines");
	            zincWidget.addVisibleRegion("TCH:surface");
	            zincWidget.addVisibleRegion("FCH:surface");
	            zincWidget.addVisibleRegion("APLAX:surface");
	            //zincWidget.addVisibleRegion("SAX:surface");
	            zincWidget.addVisibleRegion("SAXAPEX:surface");
	            zincWidget.addVisibleRegion("SAXMID:surface");
	            zincWidget.addVisibleRegion("SAXBASE:surface");
			}
            this.show(zincWidget);
	    },
	    
	    show: function(zincWidget) {
	        var showregions = true;
	        for (item in this.regionLoaded) {
	            showregions = showregions && this.regionLoaded[item];
	        }

	        if (showregions) {

	        	zincWidget.createHeartMaterials();

	            for (item in this.regionLoaded) {
	                if (item != "heart") {
	                    var itemName = "" + item; // item doesnt seem to be thread safe
	                    zincWidget.setupSurfaces(itemName, true);
	                    zincWidget.setupMeshProjection("heart", itemName);
	                }
	            }
	            zincWidget.setupLines("heart", "", "", true, "face xi3_0");
	            //zincwidget.setupSurfaces("heart", true,"face xi3_0 invisible material trans_purple");
	            zincWidget
	                    .setupSurfaces("heart", true,
	                            "face xi3_0 material default data LAGRANGIAN spectrum longaxisstrain");
	            zincWidget.setupFibres("heart",true);
	            zincWidget.filterScene();
	            zincWidget.viewAll();
	            this.zincwindowsbc.resize();
	        } 
	    },
    
        image_loaded: function(regionName, zincWidget) {
	        zincWidget.projectRegionCoordinates(regionName, "coordinates", this.transformation[regionName]);
	        this.regionLoaded[regionName] = true;
	        this.show(zincWidget);
        },
    
        TCHimage_loaded: function(zincWidget) {
	        this.image_loaded('TCH', zincWidget);
	    },
	    
	    FCHimage_loaded: function(zincWidget) {
            this.image_loaded('FCH', zincWidget);
        },

        SAXAPEXimage_loaded: function(zincWidget) {
            this.image_loaded('SAXAPEX', zincWidget);
        },
        
        SAXMIDimage_loaded: function(zincWidget) {
            this.image_loaded('SAXMID', zincWidget);
        },
        
        SAXBASEimage_loaded: function(zincWidget) {
            this.image_loaded('SAXBASE', zincWidget);
        },
        

        APLAXimage_loaded: function(zincWidget) {
            this.image_loaded('APLAX', zincWidget);
        },
	    
        loadModel: function(zincWidgetx){
        	var zincWidget = zincWidgetx;
            var exregions = new Array();
            for(var i = this.model.Regions.MESH.regionStartIndex; i < this.model.Regions.MESH.regionEndIndex; i++){
                exregions.push(this.server + this.model.Regions.MESH.meshRegionprefix + i + ".exregion");
            }
            this.regionLoaded['heart'] = false;
            zincWidget.loadMesh("", exregions, lang.hitch(this, this.onMeshReady));
            if(this.model.Regions.TCH){
	            var imgfiles = new Array();
	            for(var i = this.model.Regions.TCH.imageStartIndex; i < this.model.Regions.TCH.imageEndIndex; i++){
	            	//var j = i;
	            	//if(i < 10){
	            	//	j = "0" + i;
	            	//}
	                imgfiles.push(this.server + this.model.Regions.TCH.imageprefix + i + ".jpg");
	            }
	            this.transformation['TCH'] = this.model.Transforms.TCH;
	            this.regionLoaded['TCH'] = false;
	            zincWidget.loadImages('TCH', imgfiles, this.image_elem, "JPG",  lang.hitch(this, this.TCHimage_loaded));
	            //domAttr.remove(this.TCHcheckBox.attr,"disabled");
            }else{
            	//console.debug(this.TCHcheckBox);
            	this.TCHcheckBox.disabled = 'disabled';
            }

            if(this.model.Regions.FCH){
	            var imgfiles = new Array();
	            for(var i = this.model.Regions.FCH.imageStartIndex; i < this.model.Regions.FCH.imageEndIndex; i++){
	                //var j = i;
	                //if(i < 10){
	                //    j = "0" + i;
	                //}
	                imgfiles.push(this.server + this.model.Regions.FCH.imageprefix + i + ".jpg");
	            }            
	            this.transformation['FCH'] = this.model.Transforms.FCH;
	            this.regionLoaded['FCH'] = false;
	            zincWidget.loadImages('FCH', imgfiles, this.image_elem, "JPG", lang.hitch(this, this.FCHimage_loaded));
            	//domAttr.remove(this.FCHcheckBox.attr,"disabled");
            }else{
            	//console.debug(this.FCHcheckBox);
            	this.FCHcheckBox.disabled = 'disabled';
            }

            if(this.model.Regions.APLAX){
	            var imgfiles = new Array();
	            for(var i = this.model.Regions.APLAX.imageStartIndex; i < this.model.Regions.APLAX.imageEndIndex; i++){
	                //var j = i;
	                //if(i < 10){
	                //    j = "0" + i;
	                //}
	                imgfiles.push(this.server + this.model.Regions.APLAX.imageprefix + i + ".jpg");
	            }            
	            this.transformation['APLAX'] = this.model.Transforms.APLAX;
	            this.regionLoaded['APLAX'] = false;
	            zincWidget.loadImages('APLAX', imgfiles, this.image_elem, "JPG", lang.hitch(this, this.APLAXimage_loaded));
	            zincWidget.loadMesh("", exregions, lang.hitch(this, this.onMeshReady));
                //domAttr.remove(this.APLAXcheckBox.attr,"disabled");
            }else{
            	//console.debug(this.APLAXcheckBox);
            	this.APLAXcheckBox.disabled = 'disabled';
            }

/*            if(this.model.Regions.SAX){
	            var imgfiles = new Array();
	            for(var i = this.model.Regions.SAX.imageStartIndex; i < this.model.Regions.SAX.imageEndIndex; i++){
	                //var j = i;
	                //if(i < 10){
	                //    j = "0" + i;
	                //}
	                imgfiles.push(this.server + this.model.Regions.SAX.imageprefix + i + ".jpg");
	            }   
	            this.transformation['SAX'] = this.model.Transforms.SAX;
	            this.regionLoaded['SAX'] = false;
	            zincWidget.loadImages('SAX', imgfiles, this.image_elem, "JPG", lang.hitch(this, this.SAXimage_loaded));
	            zincWidget.loadMesh("", exregions, lang.hitch(this, this.onMeshReady));

//	            this.regionLoaded['SAX'] = false;
//	            zincWidget.loadImages('SAX', imgfiles, this.image_elem, "JPG", lang.hitch(this, this.SAXimage_loaded));
//            	//domAttr.remove(this.SAXcheckBox.attr,"disabled");
            }else{
            	//console.debug(this.SAXcheckBox);
            	this.SAXcheckBox.disabled = 'disabled';
            }*/
            var saxViewAvailable = false;
            var shortAxisViews = new Array("SAXAPEX","SAXMID","SAXBASE");
            var shortAxisCallbacks = new Array(this.SAXAPEXimage_loaded,this.SAXMIDimage_loaded,this.SAXBASEimage_loaded);
            for(var vc=0;vc<shortAxisViews.length;vc++){
	            if(this.model.Regions[shortAxisViews[vc]]){
	            	var sRegion = this.model.Regions[shortAxisViews[vc]];
		            var imgfiles = new Array();
		            for(var i = sRegion.imageStartIndex; i < sRegion.imageEndIndex; i++){
		                imgfiles.push(this.server + sRegion.imageprefix + i + ".jpg");
		            }   
		            this.transformation[shortAxisViews[vc]] = this.model.Transforms[shortAxisViews[vc]];
		            this.regionLoaded[shortAxisViews[vc]] = false;
		            zincWidget.loadImages(shortAxisViews[vc], imgfiles, this.image_elem, "JPG", lang.hitch(this, shortAxisCallbacks[vc]));
		            zincWidget.loadMesh("", exregions, lang.hitch(this, this.onMeshReady));
		            saxViewAvailable = true;
	            }
            }
            if(!saxViewAvailable){
            	//console.debug(this.SAXcheckBox);
            	this.SAXcheckBox.disabled = 'disabled';
            }
            
        },
        
        onAPLAXCheckbox: function(checked){
        	//console.log("Model.onAPLAXCheckbox: Not implemented. This is a modelling function call to be completed by Jagir. checked = " + checked);
        	if(checked==false){
        		this.zincwidgetModel.removeVisibleRegion("APLAX:surface");
        	}else{
        		this.zincwidgetModel.addVisibleRegion("APLAX:surface");
        	}
        	this.zincwidgetModel.filterMyScene();
        },

        on4CHCheckbox: function(checked){
            //console.log("Model.on4CHCheckbox: Not implemented. This is a modelling function call to be completed by Jagir. checked = " + checked);
        	if(checked==false){
        		this.zincwidgetModel.removeVisibleRegion("FCH:surface");
        	}else{
        		this.zincwidgetModel.addVisibleRegion("FCH:surface");
        	}
        	this.zincwidgetModel.filterMyScene();
        },

        on2CHCheckbox: function(checked){
            //console.log("Model.on2CHCheckbox: Not implemented. This is a modelling function call to be completed by Jagir. checked = " + checked);
        	if(checked==false){
        		this.zincwidgetModel.removeVisibleRegion("TCH:surface");
        	}else{
        		this.zincwidgetModel.addVisibleRegion("TCH:surface");
        	}
        	this.zincwidgetModel.filterMyScene();
        },
	
        onSAXCheckbox: function(checked){
            //console.log("Model.onSAXCheckbox: Not implemented. This is a modelling function call to be completed by Jagir. checked = " + checked);
        	if(checked==false){
        		for(var vc=0;vc< this.availableShortAxisViews.length;vc++){
        			this.zincwidgetModel.removeVisibleRegion( this.availableShortAxisViews[vc]+':surface');
        		}
        	}else{
        		for(var vc=0;vc< this.availableShortAxisViews.length;vc++){
        			this.zincwidgetModel.addVisibleRegion(this.availableShortAxisViews[vc]+':surface');
        		}
        	}
        	this.zincwidgetModel.filterMyScene();
        },

        onSurfaceCheckbox: function(checked){
        	if(this.usmodel){
	            //console.log("Model.onSurfaceCheckbox: Not implemented. This is a modelling function call to be completed by Jagir. checked = " + checked);
	        	var gelems = ["heart","surface","lines","fibres","nodes"];
	        	var gelsel = [true,this.heartSurface,this.heartLines,this.heartFibres,this.heartNodes];
	        	
	        	var cmdString = "";
	        	for(var c=0;c<gelsel.length;c++){
	        		if(gelsel[c]){
	        			cmdString = cmdString + ":" + gelems[c]; 
	        		}
	        	}
	        	cmdString = cmdString.substring(1);
	        	this.zincwidgetModel.removeVisibleRegion(cmdString);
	        	this.heartSurface = checked;
	        	gelsel[1] = this.heartSurface;
	        	cmdString = "";
	        	for(var c=0;c<gelsel.length;c++){
	        		if(gelsel[c]){
	        			cmdString = cmdString + ":" + gelems[c]; 
	        		}
	        	}
	        	cmdString = cmdString.substring(1);
	        	this.zincwidgetModel.addVisibleRegion(cmdString);
	
	        	this.zincwidgetModel.filterMyScene();
        	}else{
        		this.zincwidgetModel.renderRegionSurface("heart",checked);
        	}
        },

        onLinesCheckbox: function(checked){
        	if(this.usmodel){
	            //console.log("Model.onLinesCheckbox: Not implemented. This is a modelling function call to be completed by Jagir. checked = " + checked);
	        	var gelems = ["heart","surface","lines","fibres","nodes"];
	        	var gelsel = [true,this.heartSurface,this.heartLines,this.heartFibres,this.heartNodes];
	        	
	        	var cmdString = "";
	        	for(var c=0;c<gelsel.length;c++){
	        		if(gelsel[c]){
	        			cmdString = cmdString + ":" + gelems[c]; 
	        		}
	        	}
	        	cmdString = cmdString.substring(1);
	        	this.zincwidgetModel.removeVisibleRegion(cmdString);
	        	this.heartLines = checked;
	        	gelsel[2] = this.heartLines;
	        	cmdString = "";
	        	for(var c=0;c<gelsel.length;c++){
	        		if(gelsel[c]){
	        			cmdString = cmdString + ":" + gelems[c]; 
	        		}
	        	}
	        	cmdString = cmdString.substring(1);
	        	this.zincwidgetModel.addVisibleRegion(cmdString);
	
	        	this.zincwidgetModel.filterMyScene();
        	}else{
        		this.zincwidgetModel.renderRegionLine("heart",checked);
        	}
        },
        
/*        onLandMarksCheckbox: function(checked){
        	var gelems = ["heart","surface","lines","fibres","nodes"];
        	var gelsel = [true,this.heartSurface,this.heartLines,this.heartFibres,this.heartNodes];
        	
        	var cmdString = "";
        	for(var c=0;c<gelsel.length;c++){
        		if(gelsel[c]){
        			cmdString = cmdString + ":" + gelems[c]; 
        		}
        	}
        	cmdString = cmdString.substring(1);
        	this.zincwidgetModel.removeVisibleRegion(cmdString);
        	this.heartNodes = checked;
        	gelsel[4] = this.heartNodes;
        	cmdString = "";
        	for(var c=0;c<gelsel.length;c++){
        		if(gelsel[c]){
        			cmdString = cmdString + ":" + gelems[c]; 
        		}
        	}
        	cmdString = cmdString.substring(1);
        	this.zincwidgetModel.addVisibleRegion(cmdString);
        	this.zincwidgetModel.filterMyScene();
        	if(checked){
        		//call set time so that the nodes are displayed
        		this.zincwidgetModel.setTime(this.modelTime);
        	}
        },
*/
        onFibresCheckbox: function(checked){
        	if(this.usmodel){
	            //console.log("Model.onLinesCheckbox: Not implemented. This is a modelling function call to be completed by Jagir. checked = " + checked);
	        	var gelems = ["heart","surface","lines","fibres","nodes"];
	        	var gelsel = [true,this.heartSurface,this.heartLines,this.heartFibres,this.heartNodes];
	        	
	        	var cmdString = "";
	        	for(var c=0;c<gelsel.length;c++){
	        		if(gelsel[c]){
	        			cmdString = cmdString + ":" + gelems[c]; 
	        		}
	        	}
	        	cmdString = cmdString.substring(1);
	        	this.zincwidgetModel.removeVisibleRegion(cmdString);
	        	this.heartFibres = checked;
	        	gelsel[3] = this.heartFibres;
	        	cmdString = "";
	        	for(var c=0;c<gelsel.length;c++){
	        		if(gelsel[c]){
	        			cmdString = cmdString + ":" + gelems[c]; 
	        		}
	        	}
	        	cmdString = cmdString.substring(1);
	        	this.zincwidgetModel.addVisibleRegion(cmdString);
	        	
	
	        	this.zincwidgetModel.filterMyScene();
        	}else{
        		this.zincwidgetModel.renderRegionFibres("heart",checked);
        	}
        },

        
        reattachCallbackModel: function(){
        	//console.log("Model.reattachCallbackModel: Not implemented. This is a modelling function call to be completed by Jagir.");
        	// eg use this.zincwidgetModel to re setup desired view
        	this.zincwidgetModel.filterScene();
        	//console.debug(this.zincwidgetModel);
        },

        reattachCallbackSaxApex: function(){
/*            console.log("Model.reattachCallbackSax: Not implemented. This is a modelling function call to be completed by Jagir.");
            //this.zincwidgetSax.filterMyScene();
            console.debug(this.zincwidgetSax);*/
        },
        
        reattachCallbackSaxMid: function(){
        	/*            console.log("Model.reattachCallbackSax: Not implemented. This is a modelling function call to be completed by Jagir.");
        	            //this.zincwidgetSax.filterMyScene();
        	            console.debug(this.zincwidgetSax);*/
        },
        
        reattachCallbackSaxBase: function(){
        	/*            console.log("Model.reattachCallbackSax: Not implemented. This is a modelling function call to be completed by Jagir.");
        	            //this.zincwidgetSax.filterMyScene();
        	            console.debug(this.zincwidgetSax);*/
        },


        reattachCallbackTch: function(){
/*            console.log("Model.reattachCallbackTch: Not implemented. This is a modelling function call to be completed by Jagir.");
            //this.zincwidgetTch.filterMyScene();
            console.debug(this.zincwidgetTch);*/
        },

        reattachCallbackFch: function(){
/*            console.log("Model.reattachCallbackFch: Not implemented. This is a modelling function call to be completed by Jagir.");
            //this.zincwidgetFch.filterMyScene();
            console.debug(this.zincwidgetFch);*/
        },

        reattachCallbackAplax: function(){
/*            console.log("Model.reattachCallbackAplax: Not implemented. This is a modelling function call to be completed by Jagir.");
            //this.zincwidgetAplax.filterMyScene();
            console.debug(this.zincwidgetAplax);*/
        },
        
        onSave: function(){ 
            topic.publish("/" + this.uuid + "/Save");
        	if(this.debug)
        		console.log("Posting model data");
            var jsono = new Object();
            jsono['modelid']=this.model.modelid;
            try{
            	if(this.model.workflowId!=null){
            		jsono['action']='create'+this.model.workflowId;
            	}
            }catch(e){
        		jsono['action']='update';
            }
            try{
            	if(jsono['action']==null){
            		jsono['action']='update';
            	}
            	//Build the model annotation object
            	var annotation = new Object();
            	annotation["studyType"] = this.studyTypeSelect.get('value');
            	annotation["gravityType"] = this.gravityTypeSelect.get('value');
            	annotation["classification"] = this.classificationTextBox.get('value');
            	annotation["notes"] = this.textarea.get('value');
            	if(this.mvo)
            		annotation["mitralvalveopening"]=""+this.mvo;
            	if(this.mvc)
            		annotation["mitralvalveclosing"]=""+this.mvc;

            	jsono['annotation'] = dojo.toJson(annotation);
        		jsono['userid'] = 'this';
        		jsono['status'] = 'Authored';
        	}catch(ex){
        		console.debug(ex);
        	}
        	
            var xhrArgs = {
                url: this.sxmUpdate,
                postData: dojo.toJson(jsono),
                handleAs: "text",
                discardButton: this.discardButton,
                discardButtonDiv: this.discardButtonDiv,
                load: function(data){
                    var myDialog = new Dialog({
                        title: "Cardiac Model creation",
                        style: "width: 400px"
                    });
                    domStyle.set(this.discardButton,"display","none");
                    //Remove the discard button
                    domConstruct.destroy(this.discardButtonDiv);
                    myDialog.set("content", "Model successfully saved");
                    myDialog.show();
                    //console.log("Model data posted");
                },
                error: function(error){
                	if(error.message.indexOf("Cannot read property 'style'")==0){//Not sure what the error means, but the model is saved at the server
                        var myDialog = new Dialog({
                            title: "Cardiac Model creation",
                            style: "width: 400px"
                        });
                        if(discardButton){
                        domStyle.set(discardButton,"display","none");
                        //Remove the discard button
                        domConstruct.destroy(discardButtonDiv);
                        }
                        myDialog.set("content", "Model successfully saved");
                        myDialog.show();
                    	if(this.debug)
                    		console.log("Model data post error: " + error);
                	}else{
	                    var myDialog = new Dialog({
	                        title: "Cardiac Model creation",
	                        style: "width: 400px"
	                    });
	                    myDialog.set("content", "Failed to save model. If problem persists, please contact the administrator");
	                    myDialog.show();
	                	if(this.debug)
	                		console.log("Model data post error: " + error);
                	}
                }
            };
            var deferred = dojo.xhrPost(xhrArgs);
        },
        
        onDiscard: function(){ 
        	if(this.debug){
        		console.log("Posting model data");
        		console.dir(this.model);
        	}
            topic.publish("/SetupModel",this.model); //setup model as handle cannot be accessed from the xhr load local
            var jsonx = new Object();
            jsonx['action']='delete';
            if(this.model.workflowId!=null){
            	jsonx['action']='delete'+this.model.workflowId;
            }else{
            	jsonx['modelid']=this.model.modelid;
            }
       		jsonx['userid'] = 'this';
        	
            var xhrArgs = {
                url: this.sxmUpdate,
                postData: dojo.toJson(jsonx),
                handleAs: "text",
                load: function(data){
                    topic.publish("/DiscardModel"); //handle removal of the model from the tab
                    var myDialog = new Dialog({
                        title: "Cardiac Model management",
                        style: "width: 400px"
                    });
                    myDialog.set("content", "Model successfully removed");
                    myDialog.show();
                    //console.log("Model data posted");
                },
                error: function(error){
                    var myDialog = new Dialog({
                        title: "Cardiac Model management",
                        style: "width: 400px"
                    });
                    myDialog.set("content", "Failed to remove model.");
                    myDialog.show();
                    //console.log("Model data post error: " + error);
                }
            };
            var deferred = dojo.xhrPost(xhrArgs);
        },
        
        addZincPlugins: function(){
        	if(this.usmodel){
        	            // Master Zinc instance, put here so it can be re-attached to in Chrome
	            this.zincwidgetMaster = new ZincWidget({
	                zincId: "master-" + this.uuid,
	            });
	            domConstruct.place(this.zincwidgetMaster.domNode,  win.body());
	            
	            this.zincwidgetMaster.startup()
	                .then(lang.hitch(this, function(result){
	                	if(this.debug)
                		console.log("Zinc widget for model master started up, creating other shared plugins..");

                    // Model Scene Viewer
                    var style = "padding: 0px; height:100%;";
                    if(dojo.isMozilla){
                        style = style + " overflow: hidden;";
                    }
                    this.zincwidgetModel = new ZincWidget({
                        zincId: "model-" + this.uuid,
                        reattachCallback: lang.hitch(this, this.reattachCallbackModel),
                        sharedinstanceid: "master-" + this.uuid,
                        style: style,
                        "class": "overflowHidden"
                    });
                    domConstruct.place(this.zincwidgetModel.domNode, this.modelDiv);
                    this.zincwidgetModel.startup()
                        .then(lang.hitch(this, function(result){
                            this.loadModel(this.zincwidgetModel);
                        }));             

                    // Sax  Scene Viewer
                	
                	/*if(this.model.Regions.SAX){
	                    this.zincwidgetSax = new ZincWidget({
	                        zincId: "sax-" + this.uuid,
	                        reattachCallback: lang.hitch(this, this.reattachCallbackSax),
	                        sharedinstanceid: "master-" + this.uuid,
	                        style: style,
	                        frameTimeVector: this.model.frameVector,
	                        "class": "overflowHidden"
	                    });
	                    domConstruct.place(this.zincwidgetSax.domNode, this.saxDiv);
	                    this.zincwidgetSax.startup()
	                        .then(lang.hitch(this, function(result){
		                            this.zincwidgetSax.setTumbleRate(0);
		                            this.zincwidgetModel.addChild('SAX', this.zincwidgetSax);
		                            this.zincwidgetSax.addVisibleRegion('SAX:surface:projection');
	                        }));    
                	}*/
                    this.availableShortAxisViews = new Array();
                    //Saxapex Scene Viewer
                	if(this.model.Regions.SAXAPEX){
	                    this.zincwidgetSaxApex = new ZincWidget({
	                        zincId: "saxapex-" + this.uuid,
	                        reattachCallback: lang.hitch(this, this.reattachCallbackSaxApex),
	                        sharedinstanceid: "master-" + this.uuid,
	                        style: style,
	                        frameTimeVector: this.model.frameVector,
	                        "class": "overflowHidden"
	                    });
	                    domConstruct.place(this.zincwidgetSaxApex.domNode, this.saxApexDiv);
	                    this.zincwidgetSaxApex.startup()
	                        .then(lang.hitch(this, function(result){
	                        		this.zincwidgetSaxApex.setTumbleRate(0);
		                            this.zincwidgetModel.addChild('SAXAPEX', this.zincwidgetSaxApex);
		                            this.zincwidgetSaxApex.addVisibleRegion('SAXAPEX:surface:projection');
	                        }));  
	                    this.availableShortAxisViews.push("SAXAPEX");
                	}
                    //Saxmid Scene Viewer
                	if(this.model.Regions.SAXMID){
	                    this.zincwidgetSaxMid = new ZincWidget({
	                        zincId: "saxmid-" + this.uuid,
	                        reattachCallback: lang.hitch(this, this.reattachCallbackSaxMid),
	                        sharedinstanceid: "master-" + this.uuid,
	                        style: style,
	                        frameTimeVector: this.model.frameVector,
	                        "class": "overflowHidden"
	                    });
	                    domConstruct.place(this.zincwidgetSaxMid.domNode, this.saxMidDiv);
	                    this.zincwidgetSaxMid.startup()
	                        .then(lang.hitch(this, function(result){
	                        		this.zincwidgetSaxMid.setTumbleRate(0);
		                            this.zincwidgetModel.addChild('SAXMID', this.zincwidgetSaxMid);
		                            this.zincwidgetSaxMid.addVisibleRegion('SAXMID:surface:projection');
	                        }));   
	                    this.availableShortAxisViews.push("SAXMID");
                	}
                    //Saxbase Scene Viewer
                	if(this.model.Regions.SAXBASE){
	                    this.zincwidgetSaxBase = new ZincWidget({
	                        zincId: "saxbase-" + this.uuid,
	                        reattachCallback: lang.hitch(this, this.reattachCallbackSaxBase),
	                        sharedinstanceid: "master-" + this.uuid,
	                        style: style,
	                        frameTimeVector: this.model.frameVector,
	                        "class": "overflowHidden"
	                    });
	                    domConstruct.place(this.zincwidgetSaxBase.domNode, this.saxBaseDiv);
	                    this.zincwidgetSaxBase.startup()
	                        .then(lang.hitch(this, function(result){
	                        		this.zincwidgetSaxBase.setTumbleRate(0);
		                            this.zincwidgetModel.addChild('SAXBASE', this.zincwidgetSaxBase);
		                            this.zincwidgetSaxBase.addVisibleRegion('SAXBASE:surface:projection');
	                        }));  
	                    this.availableShortAxisViews.push("SAXBASE");
                	}

                	
                    // Tch  Scene Viewer
                	if(this.model.Regions.TCH){
	                    this.zincwidgetTch = new ZincWidget({
	                        zincId: "tch-" + this.uuid,
	                        reattachCallback: lang.hitch(this, this.reattachCallbackTch),
	                        sharedinstanceid: "master-" + this.uuid,
	                        style: style,
	                        frameTimeVector: this.model.frameVector,
	                        "class": "overflowHidden"
	                    });
	                    domConstruct.place(this.zincwidgetTch.domNode, this.tchDiv);
	                    this.zincwidgetTch.startup()
	                        .then(lang.hitch(this, function(result){
		                            this.zincwidgetTch.setTumbleRate(0);
		                            this.zincwidgetModel.addChild('TCH', this.zincwidgetTch);
		                            this.zincwidgetTch.addVisibleRegion('TCH:surface:projection');
	                        }));    
                	}

                    // Fch  Scene Viewer
                	if(this.model.Regions.FCH){
	                    this.zincwidgetFch = new ZincWidget({
	                        zincId: "fch-" + this.uuid,
	                        reattachCallback: lang.hitch(this, this.reattachCallbackFch),
	                        sharedinstanceid: "master-" + this.uuid,
	                        style: style,
	                        frameTimeVector: this.model.frameVector,
	                        "class": "overflowHidden"
	                    });
	                    domConstruct.place(this.zincwidgetFch.domNode, this.fchDiv);
	                    this.zincwidgetFch.startup()
	                        .then(lang.hitch(this, function(result){
		                            this.zincwidgetFch.setTumbleRate(0);
		                            this.zincwidgetModel.addChild('FCH', this.zincwidgetFch);
		                            this.zincwidgetFch.addVisibleRegion('FCH:surface:projection');
	                        }));    
                	}
                     // Aplax  Scene Viewer
                	if(this.model.Regions.APLAX){
	                     this.zincwidgetAplax = new ZincWidget({
	                        zincId: "aplax-" + this.uuid,
	                        reattachCallback: lang.hitch(this, this.reattachCallbackAplax),
	                        sharedinstanceid: "master-" + this.uuid,
	                        style: style,
	                        frameTimeVector: this.model.frameVector,
	                        "class": "overflowHidden"
	                    });
	                    domConstruct.place(this.zincwidgetAplax.domNode, this.aplaxDiv);
	                    this.zincwidgetAplax.startup()
	                        .then(lang.hitch(this, function(result){
		                            this.zincwidgetAplax.setTumbleRate(0);
		                            this.zincwidgetModel.addChild('APLAX', this.zincwidgetAplax);
		                            this.zincwidgetAplax.addVisibleRegion('APLAX:surface:projection');
	                        }));
                	}
                })); 
        	}else{
        		
	            this.zincwidgetMaster = new MRIZincWidget({
	                zincId: "master-" + this.uuid,
	            });
	            domConstruct.place(this.zincwidgetMaster.domNode,  win.body());
	            
				this.zincwidgetMaster.startup()
			    .then(lang.hitch(this, function(result){
			    	if(this.debug)
			    		console.log("MRIZinc widget for model master started up, creating other shared plugins..");
			
			        // Model Scene Viewer
			        var style = "padding: 0px; height:100%;";
			        if(dojo.isMozilla){
			            style = style + " overflow: hidden;";
			        }
			        this.zincwidgetModel = new MRIZincWidget({
			            zincId: "model-" + this.uuid,
			            reattachCallback: lang.hitch(this, this.reattachCallbackMRIModel),
			            sharedinstanceid: "master-" + this.uuid,
			            style: style,
			            "class": "overflowHidden"
			        });
			        domConstruct.place(this.zincwidgetModel.domNode, this.modelDiv);
			        this.zincwidgetModel.startup()
			            .then(lang.hitch(this, function(result){
			                this.loadMRIModel(this.zincwidgetModel);
			            }));             
			    }));
        	}
        	this.pluginsNeedCreated = false;
        },
        
       
        loadMRIModel: function(widget){
			var zincWidget = widget;
			this.planeseriesmap = new Object();
			this.visibleRegions = new Object();
			this.shortAxisRegions = new Object();
			this.longAxisRegions = new Object();
			this.planecounter = 0;
			zincWidget.setFrameTimeVector(this.model.frameVector);
			for(entry in this.model.Regions){
				var view = this.model.Regions[entry];

				if(view.type!=undefined){
						var planename = "PLANE"+view.planeid;
						this.planecounter++;
						//Get the frame urls
						var numframes = parseInt(view.frames.NUMFRAMES);
						this.planeseriesmap[view.seriesid] = planename;
						var imagefiles = new Array(numframes);
						for(var fct=0;fct<numframes;fct++){
							//Add . so that are uri is resolved with respect to current server address 
							imagefiles[fct] = "."+view.frames[""+fct];
						}
						var planecoordinates = new Array(4);
						planecoordinates[0] = view.frames.TLC;
						planecoordinates[1] = view.frames.TRC;
						planecoordinates[2] = view.frames.BLC;
						planecoordinates[3] = view.frames.BRC;
						if(view.seriesid==this.model.targetView)
							zincWidget.setTargetView(planecoordinates);
						zincWidget.addRegion(planename);
						this.visibleRegions[planename] = true;
						if(view.type=="SHORT"){
							this.shortAxisRegions[planename] = true;
						}else{
							this.longAxisRegions[planename] = true;
						}
						zincWidget.loadImages(planename, imagefiles, this.image_elem, "JPG", planecoordinates, lang.hitch(this, this.mriimage_loaded));
				}else{ //View is mesh
		            var exregions = new Array();
		            for(var i = view.regionStartIndex; i < view.regionEndIndex; i++){
		                exregions.push(this.server + this.model.Regions.MESH.meshRegionprefix + i + ".exregion");
		            }
		            this.visibleRegions['heart'] = false;
		            zincWidget.addRegion("heart");
		            zincWidget.loadMesh("", exregions, lang.hitch(this, this.onMRIMeshReady));
				}
			}
        },
        
        onMRIMeshReady: function(){
        	if(this.debug)
        		console.log("MRI Mesh Ready");
			if(!this.visibleRegions['heart']){
				this.visibleRegions['heart'] = true;
				this.zincwidgetModel.renderRegionLine('heart',true);
				this.zincwidgetModel.renderRegionSurface('heart',true);
				//this.zincwidgetModel.renderRegionFibres('heart',true);
	        } 
        },
        
        reattachCallbackMRIModel: function(){
        	//console.log("Model.reattachCallbackModel: Not implemented. This is a modelling function call to be completed by Jagir.");
        	//this.zincwidgetModel.showAllRegions();
        	for(k in this.visibleRegions){
        		if(this.visibleRegions[k])
        			this.zincwidgetModel.showRegion(k);
        	}
        	this.zincwidgetModel.viewAll();
        },
        
		sceneSelection: function(type,list){
			var vr = new Object();
			vr["heart"] = true; //Heart graphics is handled seperately
			if(type=="short"){
				for(k in this.longAxisRegions){
					if(this.visibleRegions[k]!=undefined)
						vr[k] = this.visibleRegions[k];
				}
			}else{
				for(k in this.shortAxisRegions){
					if(this.visibleRegions[k]!=undefined)
						vr[k] = this.visibleRegions[k];
				}
			}
			for(var c=0;c<list.length;c++)
				vr[this.planeseriesmap[list[c]]] = true;

			for(k in this.visibleRegions){//Clear all currently visible regions that are not selected
				if(!vr[k])
					this.zincwidgetModel.clear(k);
			}
			
			for(k in vr){
        		if(vr[k])
        			this.zincwidgetModel.showRegion(k);
        	}
			this.visibleRegions = vr;
		},
        
        mriimage_loaded: function(zincWidget) {
        	this.planecounter--;
	        if (this.planecounter<1) {
	        	zincWidget.createHeartMaterials();
	            zincWidget.showAllRegions();
	            zincWidget.viewAll();
	            this.zincwindowsbc.resize();
	        } 
        },

        
        
        
        removeZincPlugins: function(){
        	// If this function is called all zinc plugins for a model are destroyed
        	// They will be recreated when the Analysis tabs onShow is next called
        	
        	// I suspect these are not getting garbage collected very quickly or at all....
        	if(this.usmodel){
        	if(this.zincwidgetSaxApex){
        	   this.zincwidgetSaxApex.destroyRecursive(false);
        	}
        	if(this.zincwidgetSaxMid){
         	   this.zincwidgetSaxMid.destroyRecursive(false);
         	}
        	if(this.zincwidgetSaxBase){
         	   this.zincwidgetSaxBase.destroyRecursive(false);
         	}
        	if(this.zincwidgetTch){
        	   this.zincwidgetTch.destroyRecursive(false);
        	}
            if(this.zincwidgetFch){
        	   this.zincwidgetFch.destroyRecursive(false);
        	}
            if(this.zincwidgetAplax){
        	   this.zincwidgetAplax.destroyRecursive(false);
        	}
            if(this.zincwidgetModel){
        	   this.zincwidgetModel.destroyRecursive(false);
        	}
            if(this.zincwidgetMaster){
        	   this.zincwidgetMaster.destroyRecursive(false);
            }
        	}
        	if(this.debug)
        		console.log("Destroyed plugins");
        	
        	this.pluginsNeedCreated = true;
        }

	});
});
