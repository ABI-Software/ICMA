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
	"dojo/_base/json",
	"dojo/_base/lang",
    "dojo/request",
	"dojo/topic",
	"dojo/dom-style",
	"dojo/window",
	"dojo/on",
	"dojo/dom-construct",
	"dijit/Dialog",
	"dijit/form/Button",
	"dijit/form/TextBox",
	"dojox/layout/TableContainer",
	"ABI/app/ICMA/LVDigitiser",
	"dojox/dialog/AlertDialog",
	"ABI/app/ICMA/SpeckleVerifier",
	"dojox/uuid/Uuid",
	"dojox/uuid/generateRandomUuid",
	"dijit/form/HorizontalSlider",
	"ABI/lib/Widgets/ProgressIndicator",
	"dojox/charting/Chart", 
	"dojox/charting/action2d/_IndicatorElement",
	"dojox/charting/axis2d/Default", 	
	"dojox/charting/plot2d/Lines",
    "dojox/charting/widget/Legend",
    "dojox/charting/widget/SelectableLegend",
	"dijit/layout/BorderContainer",
	"dijit/layout/ContentPane",
	"dijit/layout/_LayoutWidget"
], function(declare, json, lang, request, topic, domStyle, win, on, domConstruct, 
		Dialog, Button, TextBox, TableContainer, LVDigitiser, 
		AlertDialog, SpeckleVerifier, Uuid, generateRandomUuid,
		HorizontalSlider,ProgressBar,
		Chart, IndicatorElement, Default, Lines, Legend, 
		SelectableLegend,BorderContainer,ContentPane, LayoutWidget){
  
	// module:
	//		ABI/app/ICMA/MultiImageFit
	// summary:
	//		ICMA MultiImageFit

	var MultiImageFit = declare("ABI.app.ICMA.MultiImageFit", null,{

        SXFIT: "",
        SXSPECKLETRACK: "",
        POLLINTERVAL: 10000,
        POLLTIMERHANDLE: null,
		POLLHANDLE: null,
		SPECKLETRACKINGDATA: null,
		uuid: "",
		xDIM: 400,
		yDIM: 300,
		xscalefactor: 0.5,
		yscalefactor: 0.5,
		anirate: 1,
		trackMotionInProgress: false,
		fitRequestInProgress: false,
		imagerwavestart: 0.0,
		imagerwaveend: 1.0,
		maxCAPFrames: 17,
		refreshingGraph: false,
		plotColours : ["", "#00a9a9", "#0000fd", "#dd0000", "#db7700", "#8f8e0d", "#00b200", "#04e4e4", "#ff003f", "#ff4343", "#fc8c04", "#ffff00", "#00ff00", "#0000ff", "#fb999a", "#fcfc6e", "#74b774"],
		plotStyles  : ["", "Solid",   "Solid",   "Solid",   "Solid",   "Solid",    "Solid",  "ShortDash","ShortDash", "ShortDash", "ShortDash","ShortDash", "ShortDash", "ShortDashDotDot", "ShortDashDotDot", "ShortDashDotDot", "ShortDashDotDot", "ShortDashDotDot" ],
		
		constructor: function(params){
			dojo.mixin(this, params);
			this.uuid = new Uuid(dojox.uuid.generateRandomUuid()).toString();
			
            this.aplaxKJS = null;
            this.saxKJS = null;
            this.tchKJS = null;
            this.fchKJS = null;
            
            
            this.aplaxSPEC = null;
            this.saxSPEC = null;
            this.tchSPEC = null;
            this.fchSPEC = null;
                
			this.initImageData(); //Create memory handles
			
			this.Dlg = new Dialog({
				title: "Multi-Image Fit",
				uuid: this.uuid,
				onExecute: function(){
					this.Dlg.hide();
				},
				onCancel : function(){
					topic.publish("/" + this.uuid + "/destroy",this);
				}
			});

			
			this.contentTable = domConstruct.create("table", {"class":""}, this.Dlg.containerNode);
			
			var tr = domConstruct.create("tr", {"class":""}, this.contentTable);
			
			var td = domConstruct.create("td", {"class":""}, tr);
			this.contentDiv = domConstruct.create("div", {"class":""}, td);
			
			td = domConstruct.create("td", {"class":""}, tr);
			this.graphDivHandle = domConstruct.create("div", {"class":""}, td);
			
		
			var actionBarDiv = domConstruct.create("div", {
				"class":"dijitDialogPaneActionBar"
			}, this.Dlg.containerNode);

            this.myModelTableContainer = new TableContainer({    
                //customClass: "",
                labelWidth: "160px",
                style: "float: left;"
            });
            
            this.myModelTextBox = new TextBox({
                label: "Model Name",
                value: "",
                style: "width: 7em",
            });
            this.myModelTextBox.placeAt(this.myModelTableContainer.containerNode);

            this.myModelTableContainer.placeAt(actionBarDiv);
            this.myModelTableContainer.startup();

            this.TrackButton = new Button({
				label: "Track Motion",
				"class": "",
				onClick: lang.hitch(this, this.onTrackMotion)
			});
            this.TrackButton.placeAt(actionBarDiv);
            
            
			var FitButton = new Button({
				label: "Fit",
				"class": "",
				onClick: lang.hitch(this, this.onFit)
			});
			FitButton.placeAt(actionBarDiv);
			
			topic.subscribe("/" + this.uuid + "/Time", lang.hitch( this, this.setSpeckleTime));
			topic.subscribe("/" + this.uuid + "/PlayStop", lang.hitch( this, this.playStop));
			topic.subscribe("/" + this.uuid + "/destroy", lang.hitch( this, this.destroy));
			topic.subscribe("RefreshStrainGraph",lang.hitch(this,this.refreshGraph));
		},
		
		destroy : function(){
            if(this.aplaxKJS)
            	this.aplaxKJS.close();
            if(this.saxKJS)
            	this.saxKJS.close();
            if(this.tchKJS)
            	this.tchKJS.close();
            if(this.fchKJS)
            	this.fchKJS.close();
            if(this.graphDiv){
            	domConstruct.empty(this.graphDiv);
            }
            if(this.playerId)
            	clearInterval(this.playerId);
            this.SPECKLETRACKINGDATA = null;
    		this.trackMotionInProgress=false;
    		this.fitRequestInProgress=false;
    		//console.debug("MultiImageFit destroy called");
		},
		
		
		startup: function(){
			if(this.playerId)
            	clearInterval(this.playerId);
		},
		
		addImagesTable: function(){
			
			if(this.imagesTable){
                domConstruct.destroy(this.saxDiv);
                domConstruct.destroy(this.tchDiv);
                domConstruct.destroy(this.fchDiv);
                domConstruct.destroy(this.aplaxDiv);

                this.aplaxKJS = null;
                this.saxKJS = null;
                this.tchKJS = null;
                this.fchKJS = null;
                
                if(this.graphDiv!=null){
                	domConstruct.empty(this.graphDiv);
                	this.strainchart = null;
                }
                
                domConstruct.destroy(this.imagesTable);
                this.SPECKLETRACKINGDATA=null;
            }
            
			var divs = 0;
			if(this.aplaxData.files){
				divs = divs + 1;
			}
			if(this.saxData.files){
				divs = divs + 1;
			}
			if(this.tchData.files){
				divs = divs + 1;
			}
			if(this.fchData.files){
				divs = divs + 1;
			}
			this.numActiveImages = divs;
			if(divs>2){
				this.imagesTable = domConstruct.create("table", {"class":""}, this.contentDiv);
				
				var tr = domConstruct.create("tr", {"class":""}, this.imagesTable);
				
				var td = domConstruct.create("td", {"class":""}, tr);
				this.saxDiv = domConstruct.create("div", {"class":""}, td);
				
				td = domConstruct.create("td", {"class":""}, tr);
				this.tchDiv = domConstruct.create("div", {"class":""}, td);
	
				tr = domConstruct.create("tr", {"class":""}, this.imagesTable);
	
				td = domConstruct.create("td", {"class":""}, tr);
				this.fchDiv = domConstruct.create("div", {"class":""}, td);
				
				td = domConstruct.create("td", {"class":""}, tr);
				this.aplaxDiv = domConstruct.create("div", {"class":""}, td);
				
			
				tr = domConstruct.create("tr", {"class":""}, this.imagesTable);

				td = domConstruct.create("td", {"class":"", colspan:2}, tr);
				this.flipRateDiv = domConstruct.create("div", {"class":"", style : "width:"+this.xDIM+"px; padding-top: 8px;margin: 0 auto"}, td);
				
			}else if(divs==2){
				
				this.imagesTable = domConstruct.create("table", {"class":""}, this.contentDiv);
				
				var tr = domConstruct.create("tr", {"class":""}, this.imagesTable);
				
				var td = domConstruct.create("td", {"class":""}, tr);
				if(this.saxData.files){
					this.saxDiv = domConstruct.create("div", {"class":""}, td);
					td = domConstruct.create("td", {"class":""}, tr);
				}
				if(this.aplaxData.files){
					this.aplaxDiv = domConstruct.create("div", {"class":""}, td);
					td = domConstruct.create("td", {"class":""}, tr);
				}
				if(this.tchData.files){
					this.tchDiv = domConstruct.create("div", {"class":""}, td);
					td = domConstruct.create("td", {"class":""}, tr);
				}
				if(this.fchData.files){
					this.fchDiv = domConstruct.create("div", {"class":""}, td);
				}
				
				tr = domConstruct.create("tr", {"class":""}, this.imagesTable);

				td = domConstruct.create("td", {"class":"", colspan:2}, tr);
				this.flipRateDiv = domConstruct.create("div", {"class":"", style : "width:"+this.xDIM+"px; padding-top: 8px;margin: 0 auto"}, td);
	
			}else if(divs==1){
			
				
				this.imagesTable = domConstruct.create("table", {"class":""}, this.contentDiv);
				
				var tr = domConstruct.create("tr", {"class":""}, this.imagesTable);
				
				var td = domConstruct.create("td", {"class":""}, tr);
				if(this.saxData.files){
					this.saxDiv = domConstruct.create("div", {"class":""}, td);
					td = domConstruct.create("td", {"class":""}, tr);
				}
				if(this.aplaxData.files){
					this.aplaxDiv = domConstruct.create("div", {"class":""}, td);
					td = domConstruct.create("td", {"class":""}, tr);
				}
				if(this.tchData.files){
					this.tchDiv = domConstruct.create("div", {"class":""}, td);
					td = domConstruct.create("td", {"class":""}, tr);
				}
				if(this.fchData.files){
					this.fchDiv = domConstruct.create("div", {"class":""}, td);
				}
				
				tr = domConstruct.create("tr", {"class":""}, this.imagesTable);

				td = domConstruct.create("td", {"class":""}, tr);
				this.flipRateDiv = domConstruct.create("div", {"class":"", style : "width:"+this.xDIM+"px; padding-top: 8px;margin: 0 auto"}, td);

			}

		},
		
		addSpeckleVerificationTable : function(){
			
			if(this.imagesTable){
				if(this.saxDiv!=null)
                domConstruct.destroy(this.saxDiv);
				if(this.tchDiv!=null)
                domConstruct.destroy(this.tchDiv);
				if(this.fchDiv!=null)
                domConstruct.destroy(this.fchDiv);
				if(this.aplaxDiv!=null)
                domConstruct.destroy(this.aplaxDiv);
                if(this.SPECCTRLS!=null)
                	domConstruct.destroy(this.SPECCTRLS);
                if(this.flipRateDiv!=null)
                	domConstruct.destroy(this.flipRateDiv);
                if(this.animationDiv!=null)
                	domConstruct.destroy(this.animationDiv);
                if(this.graphDiv!=null){
                	domConstruct.empty(this.graphDiv);
                	this.strainchart = null;
                }
                this.animationDiv = null; 
                this.aplaxKJS = null;
                this.saxKJS = null;
                this.tchKJS = null;
                this.fchKJS = null;
                this.flipRateDiv = null;
                
                this.aplaxSPEC = null;
                this.saxSPEC = null;
                this.tchSPEC = null;
                this.fchSPEC = null;
                
                domConstruct.destroy(this.imagesTable);
            }
            
			var divs = 1; //The graph exists by default
			if(this.SPECKLETRACKINGDATA.SAX){
				divs = divs + 1;
			}
			if(this.SPECKLETRACKINGDATA.APLAX){
				divs = divs + 1;
			}
			if(this.SPECKLETRACKINGDATA.TCH){
				divs = divs + 1;
			}
			if(this.SPECKLETRACKINGDATA.FCH){
				divs = divs + 1;
			}

			if(divs>2){
				this.imagesTable = domConstruct.create("table", {"class":""}, this.contentDiv);
				
				var tr = domConstruct.create("tr", {"class":""}, this.imagesTable);
				
				var td = domConstruct.create("td", {"class":""}, tr);
				this.saxDiv = domConstruct.create("div", {"class":""}, td);
				
				td = domConstruct.create("td", {"class":""}, tr);
				this.tchDiv = domConstruct.create("div", {"class":""}, td);
	
				tr = domConstruct.create("tr", {"class":""}, this.imagesTable);
	
				td = domConstruct.create("td", {"class":""}, tr);
				this.fchDiv = domConstruct.create("div", {"class":""}, td);
				
				td = domConstruct.create("td", {"class":""}, tr);
				this.aplaxDiv = domConstruct.create("div", {"class":""}, td);
				
				//Assign appropriate one for graph
				{
					if(this.graphDiv!=null)
						domConstruct.empty(this.graphDiv);
					this.graphDiv = null;
					if(!this.SPECKLETRACKINGDATA.SAX){
						this.graphDiv = this.saxDiv;
					}
					if(!this.SPECKLETRACKINGDATA.APLAX){
						this.graphDiv = this.aplaxDiv;
					}
					if(!this.SPECKLETRACKINGDATA.TCH){
						this.graphDiv = this.tchDiv;
					}
					if(!this.SPECKLETRACKINGDATA.FCH){
						this.graphDiv = this.fchDiv;
					}
					if(this.graphDiv==null){
						this.graphDiv = this.graphDivHandle;
					}
				}
				
				
				tr = domConstruct.create("tr", {"class":""}, this.imagesTable);
				
				td = domConstruct.create("td", { colspan: 2, "class":""}, tr);
				//this.SPECCTRLS = domConstruct.create("div", {"class":"", style:"margin: 0 auto;"}, td);
				this.SPECCTRLS = domConstruct.create("div", {"class":"", style : "width:"+this.xDIM+"px; padding-top: 8px;margin: 0 auto"}, td);

				tr = domConstruct.create("tr", {"class":""}, this.imagesTable);
				td = domConstruct.create("td", { colspan: 2, "class":""}, tr);
				//this.animationDiv = domConstruct.create("div", {"class":"", style:"margin: 0 auto;"}, td);this.flipRateDiv = domConstruct.create("div", {"class":"", style : "width:"+this.xDIM+"px; padding-top: 8px;margin: 0 auto"}, td);
				this.animationDiv = domConstruct.create("div", {"class":"", style : "width:"+this.xDIM+"px; padding-top: 8px;margin: 0 auto"}, td);
				
			}else if(divs==2){
				
				this.imagesTable = domConstruct.create("table", {"class":""}, this.contentDiv);
				
				var tr = domConstruct.create("tr", {"class":""}, this.imagesTable);
				
				var td = domConstruct.create("td", {"class":""}, tr);
				if(this.SPECKLETRACKINGDATA.SAX){
					this.saxDiv = domConstruct.create("div", {"class":""}, td);
					td = domConstruct.create("td", {"class":""}, tr);
				}
				if(this.SPECKLETRACKINGDATA.APLAX){
					this.aplaxDiv = domConstruct.create("div", {"class":""}, td);
					td = domConstruct.create("td", {"class":""}, tr);
				}
				if(this.SPECKLETRACKINGDATA.TCH){
					this.tchDiv = domConstruct.create("div", {"class":""}, td);
					td = domConstruct.create("td", {"class":""}, tr);
				}
				if(this.SPECKLETRACKINGDATA.FCH){
					this.fchDiv = domConstruct.create("div", {"class":""}, td);
					td = domConstruct.create("td", {"class":""}, tr);
				}
				//Create one for the graph
				{
					if(this.graphDiv!=null)
						domConstruct.empty(this.graphDiv);
					this.graphDiv = domConstruct.create("div", {"class":""}, td);
				}
				
				tr = domConstruct.create("tr", {"class":""}, this.imagesTable);
				
				td = domConstruct.create("td", { colspan: 2, "class":""}, tr);
				//this.SPECCTRLS = domConstruct.create("div", {"class":"", style:"margin: 0 auto;"}, td);
				this.SPECCTRLS = domConstruct.create("div", {"class":"", style : "width:"+this.xDIM+"px; padding-top: 8px;margin: 0 auto"}, td);

				tr = domConstruct.create("tr", {"class":""}, this.imagesTable);
				td = domConstruct.create("td", { colspan: 2, "class":""}, tr);
				//this.animationDiv = domConstruct.create("div", {"class":"", style:"margin: 0 auto;"}, td);this.flipRateDiv = domConstruct.create("div", {"class":"", style : "width:"+this.xDIM+"px; padding-top: 8px;margin: 0 auto"}, td);
				this.animationDiv = domConstruct.create("div", {"class":"", style : "width:"+this.xDIM+"px; padding-top: 8px;margin: 0 auto"}, td);
	
			}
						
			var playStopDiv = domConstruct.create("div", {style: "margin-bottom: 5px; float: left"}, this.SPECCTRLS);
            this.playButton = new Button({
                label: "Play",
                showLabel: false,
                iconClass: "playIcon",
                onClick: lang.hitch( this, function(){ 
                    topic.publish("/" + this.uuid + "/PlayStop");
                })
            });
            this.playButton.placeAt(playStopDiv);
            
			var playSliderDiv = domConstruct.create("div", {"class":"", style : "width:"+this.xDIM+"px;padding-top: 8px;"}, this.SPECCTRLS);
			
			this.time = 0; 
            var playSlider = HorizontalSlider({
                value: 0,
                minimum: this.time, 
                maximum: this.maxCAPFrames-1,  
                intermediateChanges: true,
                onChange: lang.hitch( this, function(value){ 
                	this.time = value;
                	this.setSpeckleTime();
                    //topic.publish("/" + this.uuid + "/Time", value);
                })
            });
            playSlider.placeAt(playSliderDiv);
            
            topic.subscribe("/" + this.uuid + "/Controller/Time", function(value){
                    playSlider.set("value", value, false);
            });
            
            var aniDiv = domConstruct.create("div", {style: "margin-bottom: 5px; padding-top: 8px;"}, this.animationDiv);
            
            domConstruct.create("label", {
            	style: "float:left",
				innerHTML: "Animation speed"
			}, aniDiv);
			
            var aniSlider = HorizontalSlider({
            	style: "float:right; padding-top: 3px; width: 65%;",
                value: 1,
                minimum: 0.1, 
                maximum: 1.5,  
                intermediateChanges: true,
                onChange: lang.hitch( this, function(value){ 
                	this.anirate = value;
                })
            });
            aniSlider.placeAt(aniDiv);
                        
		},
		
		playStop: function(){
			if(!this.playerId){
				console.log("Play");
                this.playButton.set("iconClass", "pauseIcon");
                this.playButton.set("label", "Pause");
	            this.playerId = setInterval(lang.hitch(this, function(){
	            	this.time = this.time+this.anirate;
	                if(this.time >= this.maxCAPFrames){ //!! this.model.endTime
	                    this.time = 0; //!!this.model.startTime; 
	                }
	                topic.publish("/" + this.uuid + "/Controller/Time", this.time);
	            }), 50);
            }else{
            	console.log("Stop");
                clearInterval(this.playerId);
                this.playerId = null;
                this.playButton.set("iconClass", "playIcon");
                this.playButton.set("label", "Play");
            }     
        },
		
		show: function(){
			if(this.playerId)
            	clearInterval(this.playerId);
			this.myModelTextBox.set("value", "");
			this.addImagesTable();
			this.Dlg.show();
			var winDim = win.getBox();
			if(this.numActiveImages>1){
				if(this.numActiveImages<=2)
					domStyle.set(this.Dlg.domNode,"top",winDim.h*0.25+"px");
				else
					domStyle.set(this.Dlg.domNode,"top","10px");
				domStyle.set(this.Dlg.domNode,"left",winDim.w*0.25+"px");
			}else{
				domStyle.set(this.Dlg.domNode,"top",winDim.h*0.25+"px");
				domStyle.set(this.Dlg.domNode,"left",winDim.w*0.4+"px");
			}
			if(this.TrackButton)
				this.TrackButton.set('disabled',false);
			this.initializeFit();
		},
		
		
		collateData: function (){
			// Make the object to post to the server
			var data = new Object();
			data.modelname = this.myModelTextBox.get("value");
			
			if(data.modelname.length == 0){
			 var ald = new AlertDialog({message:"Specify a model name",iconClass:"Error"});
			 ald.show();
			 return null;
			}
			var pointsChosen = false;
			var saxPointsChosen = false;
			this.epiApex = null;
			if(this.aplaxKJS){
	            if(this.aplaxKJS.orderedAnchorPoints.length > 0){
	                data.aplax = {
	                    imageid: this.aplaxData.uid[0],
	                    rwavestart: this.aplaxData.rwavestart,
	                    rwaveend: this.aplaxData.rwaveend,
	                    bpm: this.aplaxData.bpm,
	                    coordinates: new Array(),
	                };
					var coords = this.aplaxKJS.getOrderedCoordinates(800,600);
					for(var i = 0; i < coords.length; i++){
						data.aplax.coordinates.push({x: coords[i][0], y : coords[i][1]});
					}
					this.epiApex = [coords[1][0], coords[1][1]];
					pointsChosen = true;
	            }
			}
			if(this.tchKJS){
	            if(this.tchKJS.orderedAnchorPoints.length > 0){
	                data.tch = {
	                    imageid: this.tchData.uid[0],
	                    rwavestart: this.tchData.rwavestart,
	                    rwaveend: this.tchData.rwaveend,
	                    bpm: this.tchData.bpm,
	                    coordinates: new Array(),
	                };
					var coords = this.tchKJS.getOrderedCoordinates(800,600);
					for(var i = 0; i < coords.length; i++){
						data.tch.coordinates.push({x: coords[i][0], y : coords[i][1]});
					}
					if(this.epiApex==null)
						this.epiApex = [coords[1][0], coords[1][1]];
					pointsChosen = true;
	            }
			}
			if(this.fchKJS){
				if(this.fchKJS.orderedAnchorPoints.length > 0){
					data.fch = {
						imageid: this.fchData.uid[0],
						rwavestart: this.fchData.rwavestart,
						rwaveend: this.fchData.rwaveend,
						bpm: this.fchData.bpm,
						coordinates: new Array(),
					};
					var coords = this.fchKJS.getOrderedCoordinates(800,600);
					for(var i = 0; i < coords.length; i++){
						data.fch.coordinates.push({x: coords[i][0], y : coords[i][1]});
					}
					if(this.epiApex==null)
						this.epiApex = [coords[1][0], coords[1][1]];
					pointsChosen = true;
				}
			}
			if(this.saxKJS){
	            if(this.saxKJS.orderedAnchorPoints.length > 0){
	                data.sax = {
	                    imageid: this.saxData.uid[0],
	                    rwavestart: this.saxData.rwavestart,
	                    rwaveend: this.saxData.rwaveend,
	                    bpm: this.saxData.bpm,
	                    coordinates: new Array(),
	                };
					var coords = this.saxKJS.getOrderedCoordinates(800,600);
					for(var i = 0; i < coords.length; i++){
						data.sax.coordinates.push({x: coords[i][0], y : coords[i][1]});
					}
					saxPointsChosen = true;
	            }
			}


			if(pointsChosen == false ){
				if(!saxPointsChosen){
					 var ald = new AlertDialog({message:"No landmarks where chosen: Unable to submit request",buttonLabel:"Ok"});
					 ald.show();
				}else{
					 var ald = new AlertDialog({message:"A long axis view is required: Unable to submit request",buttonLabel:"Ok"});
					 ald.show();
				}
				 return null;
			}else{
				data.formatType="SINGLEFRAME";
				return data;
			}

		},
		
		renderSpeckleTrackingResult : function(){
			this.trackMotionInProgress = false;
			request.post(this.SXSPECKLETRACK+"?workflowID="+this.POLLHANDLE+"&result=true").then(lang.hitch(this, function(data){
        		//console.debug(dojo.fromJson(data));
				// do something with handled data
				this.SPECKLETRACKINGDATA = dojo.fromJson(data);
				//Setup maxCAPFrames
				if(this.SPECKLETRACKINGDATA.APLAX){
					if(this.SPECKLETRACKINGDATA.APLAX.ENDO){
						this.maxCAPFrames = this.SPECKLETRACKINGDATA.APLAX.ENDO.MARKERS.length;
					}
				}else if(this.SPECKLETRACKINGDATA.TCH){
					if(this.SPECKLETRACKINGDATA.TCH.ENDO){
						this.maxCAPFrames = this.SPECKLETRACKINGDATA.TCH.ENDO.MARKERS.length;
					}
				}else if(this.SPECKLETRACKINGDATA.FCH){
					if(this.SPECKLETRACKINGDATA.FCH.ENDO){
						this.maxCAPFrames = this.SPECKLETRACKINGDATA.FCH.ENDO.MARKERS.length;
					}
				}
				this.addSpeckleVerificationTable();
				this.initializeSpeckleVerification();
				this.initializeStrainGraph();
			}), lang.hitch(this, function(err){
            	// handle an error condition
				try{
	            	this.progressIndicator.hide();
	            	//this.progressIndicator.destroyRecursive();
	            	clearInterval(this.POLLTIMERHANDLE);
	    			this.POLLTIMERHANDLE = null;
	            	console.log("Error occured"+err);
	            	this.TrackButton.set('disabled',false);
				}catch(e){
					
				}
			}));
		},
		
		
		pollForTrackingStatus: function(){
			//console.debug(this.SXSPECKLETRACK+"?workflowID="+this.POLLHANDLE);
			request.post(this.SXSPECKLETRACK+"?workflowID="+this.POLLHANDLE).then(lang.hitch(this, function(data){
	            	// do something with handled data
	        		//console.debug(data);
					//Ensure that the progressIndicator does not go back
					if(this.progressIndicator.getValue()<data*0.99)
						this.progressIndicator.setValue(data*0.99);
	        		if(data>=1.0){
	        			this.renderSpeckleTrackingResult();
	        			clearInterval(this.POLLTIMERHANDLE);
	        			this.POLLTIMERHANDLE = null;
	        		}
	        }), lang.hitch(this, function(err){
	        	this.trackMotionInProgress = false;
            	// handle an error condition
            	console.log("Error occured"+err);
            	alert("Server error"+err+" occured!");
            	try{
	            	this.progressIndicator.hide();
	            	//this.progressIndicator.destroyRecursive();
            	}catch(e){
            		
            	}
            	clearInterval(this.POLLTIMERHANDLE);
    			this.POLLTIMERHANDLE = null;
            	this.TrackButton.set('disabled',false);
	        }));

		}, 
		
		
		onTrackMotion: function (){
			if(!this.trackMotionInProgress){
				this.trackMotionInProgress = true;
				//Construct the speckle motion display table
				var fitInput = this.collateData();
				if(fitInput!=null){
					//Disable the animation
	                for(var i =0;i<this.activeKJS.length;i++){
	              	   this.activeKJS[i].setFlipRate(0);
	                }
	                //Disable the button
					this.TrackButton.set('disabled',true);
				    request.post(this.SXSPECKLETRACK, {
			        		data : dojo.toJson(fitInput)
			        }).then(lang.hitch(this, function(data){
			        		//Check if the data is not the login challange
			        		//This occurs when the window is open on the user side and
			        		// the session has timed out
			        		if(data.indexOf("LOGIN")===-1){
				        		this.progressIndicator = new ProgressBar({headline:"Speckle Tracking Progress"});
				        		this.progressIndicator.show();
				        		this.POLLHANDLE = data;
				        		this.POLLTIMERHANDLE = setInterval(lang.hitch(this,"pollForTrackingStatus")
				        				,this.POLLINTERVAL);
			        		}else{
			        			alert("Session expired!! Relogin to continue.. all unsaved data has been lost");
			        			location.reload();
			        		}
			        }), lang.hitch(this, function(err){
			        		this.trackMotionInProgress = false;
			            	// handle an error condition
			            	console.log("Error occured"+err);
			            	alert("Server error"+err+" occured!");
			            	try{
				            	this.progressIndicator.hide();
				            	//this.progressIndicator.destroyRecursive();
			            	}catch(e){
			            		
			            	}
			            	clearInterval(this.POLLTIMERHANDLE);
			    			this.POLLTIMERHANDLE = null;
			            	this.TrackButton.set('disabled',false);
							//Enable the animation
			                for(var i =0;i<this.activeKJS.length;i++){
			              	   this.activeKJS[i].setFlipRate(this.flipRate);
			                }
			        }));
				}else{
					this.trackMotionInProgress = false;
				}
			}
		},
		
		onFit: function(){
			if(!this.fitRequestInProgress){
					this.fitRequestInProgress = true;
					if(this.SPECKLETRACKINGDATA==null){
						var data = this.collateData();
						if(data!=null){
							this.fit(data);
						}else{
							this.fitRequestInProgress = false;
						}
					}else{
						//Collect the data from verifiers
						if(this.SPECKLETRACKINGDATA.APLAX){
							if(this.SPECKLETRACKINGDATA.APLAX.ENDO){
								this.SPECKLETRACKINGDATA.APLAX.ENDO.MARKERS=this.aplaxSPEC.getMarkers();
							}
						}
						if(this.SPECKLETRACKINGDATA.TCH){
							if(this.SPECKLETRACKINGDATA.TCH.ENDO){
								this.SPECKLETRACKINGDATA.TCH.ENDO.MARKERS=this.tchSPEC.getMarkers();
							}
						}
						if(this.SPECKLETRACKINGDATA.FCH){
							if(this.SPECKLETRACKINGDATA.FCH.ENDO){
								this.SPECKLETRACKINGDATA.FCH.ENDO.MARKERS=this.fchSPEC.getMarkers();
							}
						}
						if(this.SPECKLETRACKINGDATA.SAX){
							if(this.SPECKLETRACKINGDATA.SAX.ENDO){
								this.SPECKLETRACKINGDATA.SAX.ENDO.MARKERS=this.saxSPEC.getMarkers();
							}
						}
	
						this.SPECKLETRACKINGDATA.formatType="SPECKLETRACKED";
						this.SPECKLETRACKINGDATA.workflowID=this.POLLHANDLE;
						this.fit(this.SPECKLETRACKINGDATA);
				   }
			}
		},
		
		initializeFit: function() {
			this.activeKJS = new Array();
			if(this.aplaxData.files){
				if(this.aplaxData.files.length > 0){
					var cfstep = 6.0/this.aplaxData.bpm;
					if(isNaN(cfstep))
						cfstep = 0.1;
					this.aplaxKJS = new LVDigitiser({
						element: this.aplaxDiv,
						img: this.aplaxData.files[0],
						frame: this.aplaxData.frame,
						xdim: this.xDIM,
						ydim: this.yDIM,
						type:"long",
						fstep: cfstep
					});
					this.activeKJS.push(this.aplaxKJS);
				}
			}
			if(this.saxData.files){
				if(this.saxData.files.length > 0){
					var cfstep = 6.0/this.saxData.bpm;
					if(isNaN(cfstep))
						cfstep = 0.1;
					this.saxKJS = new LVDigitiser({
						element: this.saxDiv,
						img: this.saxData.files[0],
						frame: this.saxData.frame,
						xdim: this.xDIM,
						ydim: this.yDIM,
						type:"SAX",
						fstep: cfstep
					});
					this.activeKJS.push(this.saxKJS);
				}
			}
			if(this.tchData.files){
				if(this.tchData.files.length > 0){
					var cfstep = 6.0/this.tchData.bpm;
					if(isNaN(cfstep))
						cfstep = 0.1;
					this.tchKJS = new LVDigitiser({
						element: this.tchDiv,
						img: this.tchData.files[0],
						frame: this.tchData.frame,
						xdim: this.xDIM,
						ydim: this.yDIM,
						type:"long",
						fstep: cfstep
					});
					this.activeKJS.push(this.tchKJS);
				}
			}
			if(this.fchData.files){
				if(this.fchData.files.length > 0){
					var cfstep = 6.0/this.fchData.bpm;
					if(isNaN(cfstep))
						cfstep = 0.1;

					this.fchKJS = new LVDigitiser({
						element: this.fchDiv,
						img: this.fchData.files[0],
						frame: this.fchData.frame,
						xdim: this.xDIM,
						ydim: this.yDIM,
						type:"long",			
						fstep: cfstep
					});
					this.activeKJS.push(this.fchKJS);
				}
			}
			
			domConstruct.create("label", {
            	style: "float:left",
				innerHTML: "Flip speed"
			}, this.flipRateDiv);
			
			this.flipRate = 0; 
            var flipSlider = HorizontalSlider({
            	style: "padding-top: 4px;",
                value: 10,
                minimum: 0, 
                maximum: 100,  
                intermediateChanges: true,
                onChange: lang.hitch( this, function(value){ 
                	this.flipRate = value;
                    for(var i =0;i<this.activeKJS.length;i++){
                 	   this.activeKJS[i].setFlipRate(value);
                    } 
                })
            });
            flipSlider.placeAt(this.flipRateDiv);
            			
		},
		
		initializeStrainGraph: function() {
			var strainData = new Object();
			if(this.tchSPEC){
				var sd =  this.tchSPEC.computeStrains();
				strainData["2"] = sd[0];
				strainData["8"] = sd[1];
				strainData["13"] = sd[2];
				strainData["15"] = sd[3];
				strainData["11"] = sd[4];
				strainData["5"] = sd[5];
			}
			if(this.fchSPEC){
				var sd =  this.fchSPEC.computeStrains();
				strainData["3"] = sd[0];
				strainData["9"] = sd[1];
				strainData["14"] = sd[2];
				strainData["16"] = sd[3];
				strainData["12"] = sd[4];
				strainData["6"] = sd[5];
			}
			if(this.aplaxSPEC){
				var sd =  this.aplaxSPEC.computeStrains();
				strainData["4"] = sd[5];
				strainData["10"] = sd[4];
				strainData["7"] = sd[1];
				strainData["1"] = sd[0];
/*				strainData["14p15"] = sd[3];
				strainData["13p16"] = sd[2];*/
			}

			//Create a border container layout for chart and legend
			
/*			var outer = new BorderContainer({ 
			    style:"width:"+this.xDIM+"px; height:"+this.yDIM+"px",
			    design: "sidebar"
		    }).placeAt(this.graphDiv); 

		   
			var center = new ContentPane({ 
			        region:"center", 
			        style: "border: 0px; padding: 0px; top: 0px; left: 0px; overflow: hidden; width: 90%; height: 90%"
		    //}).placeAt(outer); 
			});
			outer.addChild(center);
			
			var rightTab = new ContentPane({ 
			    region:"right",
			    style: "padding: 0px; width: 4em;"
	        //}).placeAt(outer); 
			});
			outer.addChild(rightTab);
			
			  // bottom region: 
			var outerTabs = new ContentPane({ 
			    region:"bottom",
			    style: "padding: 0px;"
	        //}).placeAt(outer); 
			});
			outer.addChild(outerTabs);
			
			outer.startup();
			
			
			this.strainchart = new Chart(center.domNode, {});
            this.strainchart.addPlot("default", {type: Lines, tension: "S"});

            this.strainchart.addAxis("x", {
                title: "Time", 
                titleGap: 0,
                titleOrientation: "away",
                majorTickStep: 1, // setting major tick step to 1 and minorTicks false is to make it work correctly under FF. If not only major ticks rendered of which our end time is not one
                minorTicks: false,
                labelFunc: lang.hitch(this, function(formattedValue, value){
                	var label = " ";
                	if(value % 2 != 0){
                    	var TotalTime = 18;
                    	var interval = TotalTime/18;
                    	label = 0 + (value - 1)*interval;
                    	label = label.toFixed(2).toString();
                	}
                	return label;
                })
            });
            this.strainchart.addAxis("y", {vertical: true});
            
            for(var k in strainData){
            	this.strainchart.addSeries(k,strainData[k],{stroke: {color: this.plotColours[k]}});
            }
            
            
            this.strainchart.addPlot("indicator", { 
            	type: "Indicator",
            	values: 1, 
            	lineStroke: { color: "red" }, 
            	precision: 1, 
            	animate: { duration: 0 },
            	labelFunc: lang.hitch(this, function(firstDataPoint, secondDataPoint, fixed, precision){
                    var label = "";
                    var TotalTime = 18;
                    var interval = TotalTime/18;

                    label = 0 + (secondDataPoint[0] - 1)*interval;
                    label = label.toFixed(2).toString();

                    return label;
            	})
            });
            
          
            this.strainchartLegend = new Legend({
            	chart: this.strainchart, 
            	horizontal: false,
            	style: "float:right"
            });
            domConstruct.place(this.strainchartLegend.domNode,rightTab.containerNode);
        
            this.graphRefreshButton = domConstruct.create("button", {innerHTML:"Refresh", style:"margin: 0 auto;"}, outerTabs.containerNode);
            on(this.graphRefreshButton, "click", lang.hitch(this,this.refreshGraph));

            on(center, "resize", this.strainchart, "resize"); 
            on(center, "resize", this.strainchartLegend, "resize"); 
           //Render the graph
            this.strainchart.render();
            this.strainchartLegend.refresh();
            
*/            

			
			var graphTableDiv = domConstruct.create("table", {"class":""}, this.graphDiv);
			
			var tr = domConstruct.create("tr", {style:"width:"+this.xDIM+"px; height:"+this.yDIM+"px",}, graphTableDiv);
			
			var td = domConstruct.create("td", {style:"width:90%; height: 100%"}, tr);
			
			var sContainer = domConstruct.create("div", null, td);
			
			var outer = new BorderContainer({ 
			    style:"width:"+this.xDIM*0.9+"px; height:"+this.yDIM*0.9+"px",
			    design: "sidebar"
		    }).placeAt(sContainer); 

		   
			var center = new ContentPane({ 
			        region:"center", 
			        style: "border: 0px; padding: 0px; top: 0px; left: 0px; overflow: hidden; width: 100%; height: 100%"
		    }).placeAt(outer); 
			
			outer.startup();

			this.strainchart = new Chart(center.domNode, {});
            this.strainchart.addPlot("default", {type: Lines, tension: "S"});
            
            /*,
            labelFunc: lang.hitch(this, function(formattedValue, value){
            	var label = " ";
            	if(value % 2 != 0){
                	var TotalTime = this.imagerwaveend;
                	var interval = TotalTime/this.maxCAPFrames;
                	label = this.imagerwavestart + (value - 1)*interval;
                	label = label.toFixed(2).toString();
            	}
            	return label;
            })*/
            
            this.strainchart.addAxis("x", {
                title: "Time", 
                titleGap: 0,
                titleOrientation: "away",
                majorTickStep: 1, // setting major tick step to 1 and minorTicks false is to make it work correctly under FF. If not only major ticks rendered of which our end time is not one
                minorTicks: true
            });
            this.strainchart.addAxis("y", {vertical: true, fixLower:"major", includeZero: true});
            
            for(var k in strainData){
            	this.strainchart.addSeries(k,strainData[k],{stroke: {color: this.plotColours[k],style: this.plotStyles[k]}});
            }

            
            this.strainchart.addPlot("indicator", { 
            	type: "Indicator",
            	values: 0, 
            	lineStroke: { color: "red" }, 
            	precision: 0.1, 
            	animate: { duration: 0 },
            	labelFunc: lang.hitch(this, function(firstDataPoint, secondDataPoint, fixed, precision){
                    var label = "";
                    label = secondDataPoint[0];
                    label = label.toFixed(2).toString();
                    return label;
            	})
            });
            
            
            //Render the graph
            this.strainchart.render();
            
            td = domConstruct.create("td", {"class":"", style:"width:10%; height: 100%"}, tr);
            var lDiv = domConstruct.create("div", {style:"padding: 0px; width: 4em;"}, td);
            
            this.strainchartLegend = new Legend({
            	chart: this.strainchart,
            	style: "padding: 0px; width: 4em;",
            	horizontal: false
            },lDiv);
            
        
/*            tr = domConstruct.create("tr", {"class":""}, graphTableDiv);
			
			td = domConstruct.create("td", {"class":"", style:"width:100%; height: 100%"}, tr);
            
            this.graphRefreshButton = domConstruct.create("button", {innerHTML:"Refresh Speckle strain (2D)", style:"margin-left: 110px;"}, td);
            
            on(this.graphRefreshButton, "click", lang.hitch(this,this.refreshGraph));*/

            on(graphTableDiv, "resize", this.strainchart, "resize"); 
            on(graphTableDiv, "resize", this.strainchartLegend, "resize"); 

            this.strainchartLegend.refresh();
            //Change window positioning
            var winDim = win.getBox();
			if(this.numActiveImages<2)
				domStyle.set(this.Dlg.domNode,"top",winDim.h*0.25+"px");
			else
				domStyle.set(this.Dlg.domNode,"top","10px");
			if(this.numActiveImages<4)
				domStyle.set(this.Dlg.domNode,"left",winDim.w*0.25+"px");
			else
				domStyle.set(this.Dlg.domNode,"left",winDim.w*0.1+"px");
            
            
		},
		
		refreshGraph: function (){
			if(!this.refreshingGraph && this.strainchart != undefined){
				this.refreshingGraph = true;
				var strainData = new Object();
				var selected = new Object();
				if(arguments[1]){
					if(arguments[1]=="TCH"||arguments[1]=="tch"){
						selected["TCH"] = true;
					}
					if(arguments[1]=="FCH"||arguments[1]=="fch"){
						selected["FCH"] = true;
					}
					if(arguments[1]=="APLAX"||arguments[1]=="APLAX"){
						selected["APLAX"] = true;
					}
				}else{
					selected["TCH"] = true;
					selected["FCH"] = true;
					selected["APLAX"] = true;			
				}
	
				if(this.tchSPEC && selected["TCH"]){
					var sd =  this.tchSPEC.computeStrains();
					strainData["15"] = sd[0];
					strainData["13"] = sd[1];
					strainData["11"] = sd[2];
					strainData["8"] = sd[3];
					strainData["5"] = sd[4];
					strainData["2"] = sd[5];
				}
				if(this.fchSPEC && selected["FCH"]){
					var sd =  this.fchSPEC.computeStrains();
					strainData["16"] = sd[0];
					strainData["14"] = sd[1];
					strainData["12"] = sd[2];
					strainData["9"] = sd[3];
					strainData["6"] = sd[4];
					strainData["3"] = sd[5];
				}
				if(this.aplaxSPEC && selected["APLAX"]){
					var sd =  this.aplaxSPEC.computeStrains();
					strainData["7"] = sd[5];
					strainData["10"] = sd[4];
					strainData["1"] = sd[1];
					strainData["4"] = sd[0];
	/*				strainData["14p15"] = sd[2];
					strainData["13p16"] = sd[3];*/
				}
				
				for(var k in strainData){
	            	this.strainchart.removeSeries(k);
	            	this.strainchart.addSeries(k,strainData[k],{stroke: {color: this.plotColours[k],style: this.plotStyles[k]}});
	            }
				this.strainchart.render();
				this.strainchartLegend.refresh();
				this.refreshingGraph = false;
			}
		},
		
		
		initializeSpeckleVerification: function() {
			this.activeSPEC = new Array();
			if(this.SPECKLETRACKINGDATA.APLAX){
				if(this.SPECKLETRACKINGDATA.APLAX.ENDO){
					this.aplaxSPEC = new SpeckleVerifier({
							element: this.aplaxDiv,
							xdim: this.xDIM,
							ydim: this.yDIM,
							xscalefactor: this.xscalefactor,
							yscalefactor: this.yscalefactor,
					        urls: this.SPECKLETRACKINGDATA.APLAX.URLS,
					        markerArray: this.SPECKLETRACKINGDATA.APLAX.ENDO.MARKERS,
					        type: "long",
					        viewPlane: "APLAX",
					        framevector: this.SPECKLETRACKINGDATA.FRAMEVECTORS.APLAX
					});
					this.activeSPEC.push(this.aplaxSPEC);
				}
			}
			if(this.SPECKLETRACKINGDATA.SAX){
				if(this.SPECKLETRACKINGDATA.SAX.ENDO){
					this.saxSPEC = new SpeckleVerifier({
						element: this.saxDiv,
						xdim: this.xDIM,
						ydim: this.yDIM,
						xscalefactor: this.xscalefactor,
						yscalefactor: this.yscalefactor,
				        urls: this.SPECKLETRACKINGDATA.SAX.URLS,
				        markerArray: this.SPECKLETRACKINGDATA.SAX.ENDO.MARKERS,
				        type: "sax",
				        viewPlane: "SAX",
				        framevector: this.SPECKLETRACKINGDATA.FRAMEVECTORS.SAX
					});
					this.activeSPEC.push(this.saxSPEC);
				}
			}
			if(this.SPECKLETRACKINGDATA.TCH){
				if(this.SPECKLETRACKINGDATA.TCH.ENDO){
					this.tchSPEC = new SpeckleVerifier({
						element: this.tchDiv,
						xdim: this.xDIM,
						ydim: this.yDIM,
						xscalefactor: this.xscalefactor,
						yscalefactor: this.yscalefactor,
				        urls: this.SPECKLETRACKINGDATA.TCH.URLS,
				        markerArray: this.SPECKLETRACKINGDATA.TCH.ENDO.MARKERS,
				        type: "long",
				        viewPlane: "TCH",
				        framevector: this.SPECKLETRACKINGDATA.FRAMEVECTORS.TCH
					});
					this.activeSPEC.push(this.tchSPEC);
				}
			}
			if(this.SPECKLETRACKINGDATA.FCH){
				if(this.SPECKLETRACKINGDATA.FCH.ENDO){
					this.fchSPEC = new SpeckleVerifier({
						element: this.fchDiv,
						xdim: this.xDIM,
						ydim: this.yDIM,
						xscalefactor: this.xscalefactor,
						yscalefactor: this.yscalefactor,
				        urls: this.SPECKLETRACKINGDATA.FCH.URLS,
				        markerArray: this.SPECKLETRACKINGDATA.FCH.ENDO.MARKERS,
				        type: "long",
				        viewPlane: "FCH",
				        framevector: this.SPECKLETRACKINGDATA.FRAMEVECTORS.FCH
					});
					this.activeSPEC.push(this.fchSPEC);
				}
			}
			this.progressIndicator.hide();
			this.progressIndicator = null;
            topic.subscribe("/" + this.uuid + "/Time", lang.hitch(this, function(time){
            		//console.debug("topic for time "+this.time); 
            		this.time = time;
                    this.setSpeckleTime();    
            }));
		},
		
		setSpeckleTime : function(){
			for(var i=0;i<this.activeSPEC.length;i++){
				this.activeSPEC[i].setStep(Math.round(this.time));
            }
            
            var plot = this.strainchart.getPlot("indicator");
            if(plot){
            	try{
            		var ptime = (Math.round(this.time))/(this.maxCAPFrames-1);
                    plot.opt.values = [ptime];
                    plot.dirty = true;
                    this.strainchart.render(); 
            	}catch(e){
            		//console.log("error indicator update");
            	}
            }   
		}, 
		
		initImageData: function(){
            this.fchData = new Object();
            this.tchData = new Object();
            this.aplaxData = new Object();
            this.saxBaseData = new Object();
            this.saxMidData = new Object();
            this.saxApexData = new Object();
		},
		
		setPatientID: function(patientID){
			this.patientID = patientID;
		},
		
		setStudyID: function(studyID){
			this.studyID = studyID;
		},
		
        setAplaxData: function(files, uid, rwavestart, rwaveend, beatsPerMin, frameRate){
            this.aplaxData = {
                files: files,
                uid: uid,
                rwavestart: rwavestart,
                rwaveend: rwaveend,
                bpm: beatsPerMin,
                frame: Math.floor(rwavestart/frameRate)
            };
            var startTime = rwavestart/frameRate;
            var endTime = rwaveend/frameRate;
            if((this.imagerwaveend - this.imagerwavestart)<(endTime-startTime)){
            	this.imagerwavestart = startTime;
            	this.imagerwaveend   = endTime;
            }
        },

        setTchData: function(files, uid, rwavestart, rwaveend, beatsPerMin, frameRate){
            this.tchData = {
                files: files,
                uid: uid,
                rwavestart: rwavestart,
                rwaveend: rwaveend,
                bpm: beatsPerMin,
                frame: Math.floor(rwavestart/frameRate)
            };
            var startTime = rwavestart/frameRate;
            var endTime = rwaveend/frameRate;
            if((this.imagerwaveend - this.imagerwavestart)<(endTime-startTime)){
            	this.imagerwavestart = startTime;
            	this.imagerwaveend   = endTime;
            }
        },
        
        setFchData: function(files, uid, rwavestart, rwaveend, beatsPerMin, frameRate){
            this.fchData = {
            	files: files,
            	uid: uid,
            	rwavestart: rwavestart,
            	rwaveend: rwaveend,
                bpm: beatsPerMin,
                frame: Math.floor(rwavestart/frameRate)
            };
            var startTime = rwavestart/frameRate;
            var endTime = rwaveend/frameRate;
            if((this.imagerwaveend - this.imagerwavestart)<(endTime-startTime)){
            	this.imagerwavestart = startTime;
            	this.imagerwaveend   = endTime;
            }
/*            if(this.imagerwavestart>rwavestart/frameRate)
            	this.imagerwavestart = rwavestart/frameRate;
            if(this.imagerwaveend<rwaveend/frameRate)
            	this.imagerwaveend = rwaveend/frameRate;*/
        },

        setSaxData: function(files, uid, rwavestart, rwaveend, beatsPerMin, frameRate){
            this.saxData = {
                files: files,
                uid: uid,
                rwavestart: rwavestart,
                rwaveend: rwaveend,
                bpm: beatsPerMin,
                frame: Math.floor(rwavestart/frameRate)
            };
        },
        
        setSaxBaseData: function(files, uid, rwavestart, rwaveend, beatsPerMin, frameRate){
            this.saxBaseData = {
                files: files,
                uid: uid,
                rwavestart: rwavestart,
                rwaveend: rwaveend,
                bpm: beatsPerMin,
                frame: Math.floor(rwavestart/frameRate)
            };
        },
        
        setSaxMidData: function(files, uid, rwavestart, rwaveend, beatsPerMin, frameRate){
            this.saxMidData = {
                files: files,
                uid: uid,
                rwavestart: rwavestart,
                rwaveend: rwaveend,
                bpm: beatsPerMin,
                frame: Math.floor(rwavestart/frameRate)
            };
        },
        
        setSaxApexData: function(files, uid, rwavestart, rwaveend, beatsPerMin, frameRate){
            this.saxApexData = {
                files: files,
                uid: uid,
                rwavestart: rwavestart,
                rwaveend: rwaveend,
                bpm: beatsPerMin,
                frame: Math.floor(rwavestart/frameRate)
            };
        },
        
        setSxFit: function (SXFIT){
        	this.SXFIT = SXFIT;
        },
        
        setSxSpeckleTrack: function (SXSpeckleTrack){
        	this.SXSPECKLETRACK = SXSpeckleTrack;
        },
        
		renderFittingResult : function(){
			request.post(this.SXFIT+"?workflowID="+this.POLLHANDLE+"&result=true").then(lang.hitch(this, function(data){
        		//console.debug(dojo.fromJson(data));

        		var jsonD = dojo.fromJson(data);
                
        		this.progressIndicator.hide();
            	clearInterval(this.POLLTIMERHANDLE);
    			this.POLLTIMERHANDLE = null;

                if(jsonD.workflowId){
                	topic.publish("/NewModel",jsonD);
               		var myDialog = new Dialog({
                        title: "Cardiac Model creation",
                        style: "width: 500px"
                    });

                    myDialog.set("content", "Model <b>"+jsonD.name+"</b> has been created. You should be able to see the model in the models Tab. <BR> Unsaved models are automatically purged, make sure you save the model if you approve.");

                    myDialog.show();
                }else{
                	alert("Server error occured!!"+data);
                }
			}), lang.hitch(this, function(err) {
	        		this.progressIndicator.hide();
	            	//this.progressIndicator.destroyRecursive();
	            	clearInterval(this.POLLTIMERHANDLE);
	    			this.POLLTIMERHANDLE = null;
	            	// handle an error condition
	            	console.log("Error occured"+err);
	      			var ald = new AlertDialog({message:"Service failed "+err,iconClass:"Error"});
	    			ald.show();
				}));
		},
		
		
		pollForFittingStatus: function(){
			request.post(this.SXFIT+"?workflowID="+this.POLLHANDLE).then(lang.hitch(this, function(data){
	            	// do something with handled data
	        		//console.debug(data);
	        		var jsonD = dojo.fromJson(data);
	        		var progress = 0.0;
	        		if(jsonD.tracking){
	        			progress = jsonD.tracking*0.5;
	        		}
	        		if(jsonD.fitting){
	        			progress = progress + jsonD.fitting*1.0;
	        		}
	        		if(progress>1.0){
	        			progress = 1.0;
	        		}
	        		if(this.progressIndicator.getValue()<progress*0.95)
	        			this.progressIndicator.setValue(progress*0.95);//Since preparing the json takes time
	        		if(progress==1.0){
	        			this.renderFittingResult();
	        			clearInterval(this.POLLTIMERHANDLE);
	        			this.POLLTIMERHANDLE = null;
	        		}
	        }), lang.hitch(this, function(err){
	            	// handle an error condition
		        	try{
	        			this.progressIndicator.hide();
		            	//this.progressIndicator.destroyRecursive();
		        	}catch(e){
		        		
		        	}
        			clearInterval(this.POLLTIMERHANDLE);
        			this.POLLTIMERHANDLE = null;
	            	console.log("Error occured"+err);
	       			var ald = new AlertDialog({message:"Service failed "+err,iconClass:"Error"});
	    			ald.show(); 
	        	}));
		}, 

        
        fit: function(dataX){
        	var data = dataX;
        	data.epiApex = this.epiApex;
        	data.GELEM_CENTROID = [this.xDIM/2,this.yDIM/2];
          	request.post(this.SXFIT, {
        		data : dojo.toJson(data),
        	}).then(lang.hitch(this, function(data){
        		this.fitRequestInProgress = false;
				this.Dlg.hide();
        		//console.debug(data);
        		try{
	        		var jsonD = dojo.fromJson(data);
	        		if(jsonD.workflowID){
	        			this.POLLHANDLE = jsonD.workflowID;
	        			this.POLLTIMERHANDLE = setInterval(lang.hitch(this,this.pollForFittingStatus),this.POLLINTERVAL);
	        			this.progressIndicator = new ProgressBar({headline:"Model Fitting Progress"});
	               		this.progressIndicator.show();
	        			this.pollForFittingStatus();
	        		}else{
	       			 var ald = new AlertDialog({message:"Service failed "+data,iconClass:"Error"});
	    			 ald.show();
	        		}
        		}catch(e){
	       			 var ald = new AlertDialog({message:"Service failed "+data,iconClass:"Error"});
	    			 ald.show();        			
        		}
            }), function(err){
        		this.fitRequestInProgress = false;
      			var ald = new AlertDialog({message:"Service failed "+err,iconClass:"Error"});
    			ald.show();        						
            	console.log("Error occured"+err);
            });
        }
        
	});
	
	ABI.app.ICMA._multiImageFit = null;

	MultiImageFit.getSingleton = ABI.app.ICMA.multiImageFit = function () {
        if (!ABI.app.ICMA._multiImageFit) {
        	ABI.app.ICMA._multiImageFit = new ABI.app.ICMA.MultiImageFit();
        }
        return ABI.app.ICMA._multiImageFit; // Object
    };

    ABI.app.ICMA.MultiImageFitSingleton = MultiImageFit.getSingleton();

    return MultiImageFit;
});
