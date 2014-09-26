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
	"dojo/dom-geometry",
    "dojo/request",
	"dojo/_base/window",
	"dojo/_base/sniff",
	"dojo/aspect",
	"dojo/date/locale",
	"dojo/dom",
	"dojo/dom-class",
	"dojo/dom-construct",
	"dojo/dom-style",
	"dojo/dom-attr",
	"dojo/topic",
	"dojo/on",
	"dojo/query",
	"dojo/window",
	"dojox/mobile/Carousel",
	"dojox/mobile/CarouselItem",
	//"dojox/mobile/Button",
	"dijit/form/Button",
	"dojox/string/sprintf",
	"dojox/mobile/SwapView",
	"dojox/mobile/TextBox",
	"dijit/form/CheckBox",
	"dojox/mobile/Accordion",
	"dojox/mobile/RoundRectList",
	"dojox/mobile/ListItem",
	"dojox/mobile/ContentPane",
	//"dojox/mobile/Video",
	"ABI/lib/Widgets/VideoPlayer",
	"dojox/layout/TableContainer",
	"dijit/Dialog",
	"dijit/TitlePane",
	"dijit/form/TextBox",
	"dijit/layout/BorderContainer",
	"dijit/layout/ContentPane",
	"dijit/layout/TabContainer",
	"dijit/_Contained",
	"dijit/_Container",
	"dijit/_WidgetBase",
	"ABI/lib/Widgets/Alert",
	"ABI/app/ICMA/LVDigitiser",
	"ABI/app/ICMA/LVBasePlaneDigitiser",
	"dojox/dialog/AlertDialog",
	"ABI/app/ICMA/SpeckleVerifier",
	"dojox/uuid/Uuid",
	"dojox/uuid/generateRandomUuid",
	"dijit/form/HorizontalSlider",
	"ABI/lib/Widgets/ProgressIndicator",
	"dojox/charting/widget/Chart", 
	"dojox/charting/action2d/_IndicatorElement",
	"dojox/charting/axis2d/Default", 	
	"dojox/charting/plot2d/Lines",
    "dojox/charting/widget/Legend",
    "dojox/charting/widget/SelectableLegend"	
	
], function(declare, array, event, lang, domGeom, request, win, has, aspect, locale, dom, domClass, domConstruct, domStyle, domAttr, topic, on, query, dojoWindow, Carousel, CarouselItem, Button, sprintf, SwapView, 
		TextBox, CheckBox, Accordion, RoundRectList, ListItem, MContentPane, Video, TableContainer, Dialog, TitlePane, 
		DijitTextBox, BorderContainer, ContentPane, TabContainer, Contained, Container,
		WidgetBase, Alert,
		LVDigitiser,LVBasePlaneDigitiser,AlertDialog, SpeckleVerifier, Uuid, generateRandomUuid,
		HorizontalSlider,ProgressBar,Chart, IndicatorElement, Default, Lines, Legend,SelectableLegend){

	// module:
	//		ABI/app/ICMA/Scan

	return declare("ABI.app.ICMA.Scan", [ContentPane],{
		// summary:
		//		A widget that presents a scan
		// description:
		//		
	   server: null,
	   setFromVideo: true,
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
	   patientID: null,
	   studyID: null,
	   clientWidth : 0,
	   viewready: false,
	   plotColours : ["", "#00a9a9", "#0000fd", "#dd0000", "#db7700", "#8f8e0d", "#00b200", "#04e4e4", "#ff003f", "#ff4343", "#fc8c04", "#ffff00", "#00ff00", "#0000ff", "#fb999a", "#fcfc6e", "#74b774"],
       plotStyles  : ["", "Solid",   "Solid",   "Solid",   "Solid",   "Solid",    "Solid",  "ShortDash","ShortDash", "ShortDash", "ShortDash","ShortDash", "ShortDash", "ShortDashDotDot", "ShortDashDotDot", "ShortDashDotDot", "ShortDashDotDot", "ShortDashDotDot" ],

		
		loadFirstView: function(nval){
			//This function gets called for all the child tabs
			//as they subscribe to the topic
			//so check which child was selected and respond
			//else error occur
			if(this.title==nval.title){
				try{
					var itemWidget = this.viewsCarousel.getItemWidgetByIndex(0);
					if(itemWidget){
						var studyviews = itemWidget.params.studyObj.views[itemWidget.params.value];
						//Load of video is different from one already being shown
						if(this.view==undefined || this.view.imageid!=studyviews.imageid){
			                this.viewsCarousel.select(itemWidget);
						    this.onViewClick(this.viewsCarousel, itemWidget);
						}
					}
				}catch(e){
					console.debug(e);
				}
				topic.publish("ABI/lib/Widgets/ready");
			}
		},
       
		
		constructor: function(params){
			dojo.mixin(this, params);
			this.uuid = new Uuid(dojox.uuid.generateRandomUuid()).toString();
			this.patientID = ABI.app.ICMA.PatientManagerSingleton.getCurrentPatient();
			this.studyID = this.study.studyUid;
		},
		
		startup: function(){
			if(this._started){ return; }
			this.inherited(arguments);
			if(this.playerId)
            	clearInterval(this.playerId);
			this.viewready = false;
			topic.subscribe("/" + this.uuid + "/Time", lang.hitch( this, this.setSpeckleTime));
			topic.subscribe("/" + this.uuid + "/PlayStop", lang.hitch( this, this.playStop));
			//topic.subscribe("/" + this.uuid + "/destroy", lang.hitch( this, this.destroy));
			topic.subscribe("/" + this.uuid + "/RefreshStrainGraph",lang.hitch(this,this.refreshGraph));
			topic.subscribe("/loadFirstUSView",lang.hitch( this, this.loadFirstView));
			try{
				var itemWidget = this.viewsCarousel.getItemWidgetByIndex(0);
				if(itemWidget){
	                this.viewsCarousel.select(itemWidget);
				    this.onViewClick(this.viewsCarousel, itemWidget);
				}
			}catch(e){
				console.debug(e);
			}
		},
		
		buildRendering: function(){
			this.inherited(arguments);
			
			this.bc = new BorderContainer({
	            style: "height: 100%; width: 100%;"
	        });
	        
	        this.addChild(this.bc);
				        
	        this.bc.startup();
	        	        
			this.fbc = new BorderContainer({
	            style: "height: 100%; width: 100%;display: none;"
	        });
	        
	        this.addChild(this.fbc);
				        
	        this.fbc.startup();

	        this.vbc = new BorderContainer({
	            style: "height: 100%; width: 100%;display: none;"
	        });
	        
	        this.addChild(this.vbc);
	        	        
	        this.vbc.startup();
	        
	        this.addLeftRightPanes();
	        
	        this.addLeftPaneLayout();
	        
	        this.addStudyDetails();
	        
	        this.addViewsCarousel();
	        
			this.addFitSelectionTarget();
		},
		
		postCreate: function(){
			this.inherited(arguments);
		},
		
		addLeftRightPanes: function(){			
	        this.center = new ContentPane({
	        	 region: "center",
	             //content: "some content center"
	        });
	        this.bc.addChild(this.center);
	        
	        this.right = new ContentPane({
	        	 region: "right",
	        	 splitter: true
	             //content: "some content right"
	        });
	        this.bc.addChild(this.right);
	        
			this.fittingPaneBc = new ContentPane({
				region: "center",
	            style: "height: 100%; width: 98%;"
	        });
	        
			this.fbc.addChild(this.fittingPaneBc);
	        
	        this.vbccenter = new ContentPane({
	        	 region: "center",
	             //content: "some content center"
	        });
	        this.vbc.addChild(this.vbccenter);
		},
		
		addLeftPaneLayout: function(){
			this.leftPaneBc = new BorderContainer({
	            style: "height: 100%; width: 100%;overflow: auto; min-height: 600px; min-width: 800px"
	        });
	        
			this.center.addChild(this.leftPaneBc);
	        this.leftPaneBc.startup();
	        

	        this.leftPaneBcLeft = new ContentPane({
	        	 region: "left",
	        	 splitter: true
	             //content: "some content left"
	        	 //style: "width: 500px"
	        });
	        this.leftPaneBc.addChild(this.leftPaneBcLeft);
	        
	        this.leftPaneBcCenter = new ContentPane({
	        	 region: "center",
	        	 style: "height: 100%"
	        });
	        this.leftPaneBc.addChild(this.leftPaneBcCenter);
	        
	        aspect.after(this.leftPaneBcCenter, "resize", lang.hitch(this, this.resizeVideoView));

	        this.leftPaneBcBottom = new ContentPane({
	        	 region: "bottom",
	        	 splitter: true
	             //content: "some content bottom"
	        });
	        this.leftPaneBc.addChild(this.leftPaneBcBottom);

		},
		
		
		createFittingPane: function(){
			var targets = query("td", this.selectionTable);
			var params = new Array();
			var sortedTargets = new Array();
			var viewOrder = new Array("4CH","2CH","APLAX","SAXAPEX","SAXMID","SAXBASE");
			for(var j=0;j<viewOrder.length;j++){
				for(var i=0;i<targets.length;i++){
					var viewname = targets[i].getAttribute("viewname");
					if(viewname==viewOrder[j]){
						sortedTargets.push(targets[i]);
						break;
					}
				}
			}
			targets = sortedTargets;
			//Sort them in long axis to short axis order
            for(var i = 0; i < 6; i++){
            	if(targets[i].data){ 
					params.push({
						viewname: targets[i].getAttribute("viewname"),
						data: targets[i].data
					});
            	}
            }
            var numSelectedViews = params.length;
			if(numSelectedViews == 0){
				ABI.lib.Widgets.AlertSingleton.alert("Fit Model", "At least 1 view must be chosen");
				return;
			}
			//Create tables
			if(this.contentTable){
				domConstruct.destroy(this.contentTable);
				domConstruct.destroy(this.actionTable);
				domConstruct.destroy(this.createFittingPaneBC);
			}
			
			var bc = new BorderContainer({
	            style: "height: 100%; width: 100%;"
	        });
	        
			this.fittingPaneBc.addChild(bc);
	        bc.startup();
	        this.createFittingPaneBC = bc;
	        var centerPane = new ContentPane({
	        	 region: "center",
	        });
	        bc.addChild(centerPane);
	        
	        var bottomPane = new ContentPane({
	        	 region: "bottom",
	        	 overflow: "hidden"
	        });
	        bc.addChild(bottomPane);
			this.fittingcontainer = bc;
			//Note that bc is the visible container and will have a client width others will not have
			this.clientWidth = this.bc.domNode.clientWidth;
			var numcols = Math.floor(this.clientWidth/this.xDIM);
			if(numcols<1)
				numcols = 1;
			if(numcols>3)
				numcols = 3;
			if(numcols>numSelectedViews)
				numcols = numSelectedViews;
			var numRows = Math.floor((numSelectedViews+numcols-1)/numcols);
			var graphDivs = new Array();
			
			//this.contentTable = domConstruct.create("table", {"class":"","style":"margin-left: "+marginLeft+"px"}, centerPane.containerNode);
			this.contentTable = domConstruct.create("table", {"class":"","style":"margin-left: auto; margin-right: auto; margin-top: auto"}, centerPane.containerNode);

			var offsetRow = numRows -1;
			var celloffset = -1;
			var offsetmargin = 0;
			var numcells = numRows*numcols;

			//Give numbers based on size
			var freeCells = numcells - numSelectedViews;
			if(freeCells==2){
				celloffset = 0;
				offsetmargin = this.xDIM;
			}else if(freeCells==1){
				celloffset = 0;
				offsetmargin = 0.5*this.xDIM;
			}
			
			for(var ctr=0;ctr<offsetRow;ctr++){
				var tr = domConstruct.create("tr", {"class":""}, this.contentTable);
				for(var cc=0;cc<numcols;cc++){
					var td = domConstruct.create("td", {"class":""}, tr);
					graphDivs.push(domConstruct.create("div", {"class":""}, td));
				}
			}
			{
				var tr = domConstruct.create("tr", {"class":""}, this.contentTable);
				for(var cc=0;cc<numcols;cc++){
					var td = domConstruct.create("td", {"class":""}, tr);
					if(cc>celloffset){
						graphDivs.push(domConstruct.create("div", {"class":""}, td));
					}else{
						//domStyle.set(td,"width",offsetmargin+"px");
						domConstruct.create("div", {"class":"","style":"width:"+offsetmargin+"px"}, td);
					}
				}
			}
            var tablectr = 0;			

            this.activeKJS = new Array();
			for(var i = 0; i < 6; i++){
	                if(targets[i].data){
	                	var viewname = targets[i].getAttribute("viewname");
	                	var image = targets[i].image.src;
	                	var imageid = targets[i].data.imageid;
                        var wavestart = targets[i].data.beginningRwave;
                        var waveend = targets[i].data.followingRwave;
                        var beatsPerMin = parseInt(targets[i].data.beatsPerMin);
                        var mFrameRate = parseInt(targets[i].data.movieFrameRate);
                        //var frameRate   = targets[i].data.movieFrameRate;
                        //var startTime = wavestart/frameRate;
                        //var endTime = waveend/frameRate;
                        var startTime = targets[i].data.beginningRwaveTime;
                        var endTime   = targets[i].data.followingRwaveTime;
                        if((this.imagerwaveend - this.imagerwavestart)<(endTime-startTime)){
                        	this.imagerwavestart = startTime;
                        	this.imagerwaveend   = endTime;
                        }

	                	switch(viewname){
	                        case "4CH":{
		                            this.fchData = {
		                                files: [image],
		                                uid: [imageid],
		                                rwavestart: wavestart,
		                                rwaveend: waveend,
		                                bpm: beatsPerMin,
		                                frame: startTime,
		                                movieFrameRate: mFrameRate
		                            };

		                			if(this.fchData.files){
		                				if(this.fchData.files.length > 0){
		                					var cfstep = 6.0/this.fchData.bpm;
		                					if(isNaN(cfstep))
		                						cfstep = 0.1;
		                					this.fchKJS = new LVBasePlaneDigitiser({
		                						element: graphDivs[tablectr++],
		                						img: this.fchData.files[0],
		                						frame: this.fchData.frame,
		                						bpm: this.fchData.bpm,
		                						xdim: this.xDIM,
		                						ydim: this.yDIM,
		                						type:"long",
		                						fstep: cfstep,
		                					});
		                					this.activeKJS.push(this.fchKJS);
		                				}
		                			}
		                			this.fchSelected = true;
		                            break;
	                        	}
	                        case "2CH": {
		                            this.tchData = {
		                                files: [image],
		                                uid: [imageid],
		                                rwavestart: wavestart,
		                                rwaveend: waveend,
		                                bpm: beatsPerMin,
		                                frame: startTime,
		                                movieFrameRate: mFrameRate
		                            };
		            				if(this.tchData.files.length > 0){
		            					var cfstep = 6.0/this.tchData.bpm;
		            					if(isNaN(cfstep))
		            						cfstep = 0.1;
		            					this.tchKJS = new LVBasePlaneDigitiser({
		            						element: graphDivs[tablectr++],
		            						img: this.tchData.files[0],
		            						frame: this.tchData.frame,
		            						bpm: this.tchData.bpm,
		            						xdim: this.xDIM,
		            						ydim: this.yDIM,
		            						type:"long",
		            						fstep: cfstep,
		            					});
		            					this.activeKJS.push(this.tchKJS);
		            				}
		            				this.tchSelected = true;
		                            break;
	                        		}
	                        case "APLAX": {
		                            this.aplaxData = {
		                                files: [image],
		                                uid: [imageid],
		                                rwavestart: wavestart,
		                                rwaveend: waveend,
		                                bpm: beatsPerMin,
		                                frame: startTime,
		                                movieFrameRate: mFrameRate
		                            };
		            				if(this.aplaxData.files.length > 0){
		            					var cfstep = 6.0/this.aplaxData.bpm;
		            					if(isNaN(cfstep))
		            						cfstep = 0.1;
		            					this.aplaxKJS = new LVBasePlaneDigitiser({
		            						element: graphDivs[tablectr++],
		            						img: this.aplaxData.files[0],
		            						frame: this.aplaxData.frame,
		            						bpm: this.aplaxData.bpm,
		            						xdim: this.xDIM,
		            						ydim: this.yDIM,
		            						type:"long",
		            						fstep: cfstep,
		            					});
		            					this.activeKJS.push(this.aplaxKJS);
		            				}
		            				this.aplaxSelected = true;
		                            break;
	                        	}
	                        case "SAXAPEX": {
		                            this.saxApexData = {
		                                files: [image],
		                                uid: [imageid],
		                                rwavestart: wavestart,
		                                rwaveend: waveend,
		                                bpm: beatsPerMin,
		                                frame: startTime
		                            };
		            				if(this.saxApexData.files.length > 0){
		            					var cfstep = 6.0/this.saxApexData.bpm;
		            					if(isNaN(cfstep))
		            						cfstep = 0.1;
		            					this.saxapexKJS = new LVDigitiser({
		            						element: graphDivs[tablectr++],
		            						img: this.saxApexData.files[0],
		            						frame: this.saxApexData.frame,
		            						xdim: this.xDIM,
		            						ydim: this.yDIM,
		            						type:"SAX",
		            						fstep: cfstep
		            					});
		            					this.activeKJS.push(this.saxapexKJS);
		            				}
		            				this.saxapexSelected = true;
		                            break;
	                        	}
	                        case "SAXMID":{
		                            this.saxMidData = {
		                                files: [image],
		                                uid: [imageid],
		                                rwavestart: wavestart,
		                                rwaveend: waveend,
		                                bpm: beatsPerMin,
		                                frame: startTime
		                            };
		            				if(this.saxMidData.files.length > 0){
		            					var cfstep = 6.0/this.saxMidData.bpm;
		            					if(isNaN(cfstep))
		            						cfstep = 0.1;
		            					this.saxmidKJS = new LVDigitiser({
		            						element: graphDivs[tablectr++],
		            						img: this.saxMidData.files[0],
		            						frame: this.saxMidData.frame,
		            						xdim: this.xDIM,
		            						ydim: this.yDIM,
		            						type:"SAX",
		            						fstep: cfstep
		            					});
		            					this.activeKJS.push(this.saxmidKJS);
		            				}
		            				this.saxmidSelected = true;
		                            break;
	                        	}
	                        case "SAXBASE": {
		                            this.saxBaseData = {
		                                files: [image],
		                                uid: [imageid],
		                                rwavestart: wavestart,
		                                rwaveend: waveend,
		                                bpm: beatsPerMin,
		                                frame: startTime
		                            };
		            				if(this.saxBaseData.files.length > 0){
		            					var cfstep = 6.0/this.saxBaseData.bpm;
		            					if(isNaN(cfstep))
		            						cfstep = 0.1;
		            					this.saxbaseKJS = new LVDigitiser({
		            						element: graphDivs[tablectr++],
		            						img: this.saxBaseData.files[0],
		            						frame: this.saxBaseData.frame,
		            						xdim: this.xDIM,
		            						ydim: this.yDIM,
		            						type:"SAX",
		            						fstep: cfstep
		            					});
		            					this.activeKJS.push(this.saxbaseKJS);
		            				}		
		            				this.saxbaseSelected = true;
		                            break;
	                            }
	                    }
	                }
			}
			//Sync long axis view time syncing
			if(this.fchKJS && this.tchKJS){
				this.fchKJS.addTimeSlave(this.tchKJS);
				if(this.aplaxKJS){
					this.tchKJS.addTimeSlave(this.aplaxKJS);
				}
			}else if(this.fchKJS && this.aplaxKJS){
				this.fchKJS.addTimeSlave(this.aplaxKJS);
			}
			if(this.tchKJS && this.aplaxKJS){
				this.tchKJS.addTimeSlave(this.aplaxKJS);
			}
			
			
			
			//Create controls
		
			//this.actionTable = domConstruct.create("table", {"class":"dijitDialogPaneActionBar"}, this.fittingPaneBc.containerNode);
			this.actionTable = domConstruct.create("table", {"class":""}, bottomPane.containerNode);
			tr = domConstruct.create("tr", {"class":""}, this.actionTable);
			var td = domConstruct.create("td", {"class":""}, tr);
			var d1 = domConstruct.create("div", {"class":""}, td);
			td = domConstruct.create("td", {"class":""}, tr);
			var d2 = domConstruct.create("div", {"class":""}, td);
			td = domConstruct.create("td", {"class":"", style:"float:right"}, tr);
			var d3 = domConstruct.create("div", {"class":""}, td);

			
			var cellengths = this.clientWidth/3-5;

			this.flipRate = 0; 
			if(this.saxapexSelected || this.saxmidSelected ||this.saxbaseSelected){
	            var flipSlider = HorizontalSlider({
	            	label: "Flip Speed",
	            	style: "width:"+(cellengths-70)+"px;",
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
	            
	            this.myFlipTableContainer = new TableContainer({  
	                labelWidth: cellengths+"px",
	                style: "float: left;",
	                cols:2
	            });
	            flipSlider.placeAt(this.myFlipTableContainer.containerNode);
			}
            
            this.myModelTableContainer = new TableContainer({    
                //customClass: "",
            	labelWidth: cellengths+"px",
            	cols: 2,
            	spacing: 0
            });
            
            this.myModelTextBox = new TextBox({
                label: "Model Name",
                value: "",
                style: "width: 17em; margin-left:10px",
            });
            this.myModelTextBox.placeAt(this.myModelTableContainer.containerNode);


            this.TrackButton = new Button({
				label: "Track Motion",
				"class": "",
				onClick: lang.hitch(this, this.onTrackMotion)
			});
            this.TrackButton.placeAt(d3);
            
            
			var FitButton = new Button({
				label: "Fit",
				"class": "",
				onClick: lang.hitch(this, this.onFit)
			});
			FitButton.placeAt(d3);
			
			var returnToVideo = new Button({
				label: "Return to View",
				"class": "",
				onClick: lang.hitch(this, function(){
		            this.bc.domNode.style.display = '';
		            this.fbc.domNode.style.display = 'none';
		            this.fittingPaneBc.removeChild(this.fittingcontainer);
					domConstruct.destroy(this.fittingcontainer);
					this.resizeVideoView();
				})
			});
			returnToVideo.placeAt(d3);
            
			if(this.saxapexSelected || this.saxmidSelected ||this.saxbaseSelected){
				this.myFlipTableContainer.placeAt(d2);
		        this.myFlipTableContainer.startup();
			}

            this.myModelTableContainer.placeAt(d1);
            this.myModelTableContainer.startup();
            
            
			//Hide the video and fitting pane
            this.bc.domNode.style.display = 'none';
            this.fbc.domNode.style.display = '';
            this.fbc.resize();
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
			if(this.aplaxKJS&&this.aplaxSelected){
	            if(this.aplaxKJS.orderedAnchorPoints.length > 0){
	                data.aplax = {
	                    imageid: this.aplaxData.uid[0],
	                    rwavestart: this.aplaxData.rwavestart,
	                    rwaveend: this.aplaxData.rwaveend,
	                    bpm: this.aplaxData.bpm,
	                    coordinates: new Array(),
	                    escoordinates: new Array()
	                };
					var coords = this.aplaxKJS.getOrderedCoordinates(800,600);
					for(var i = 0; i < coords.length; i++){
						data.aplax.coordinates.push({x: coords[i][0], y : coords[i][1]});
					}
					this.epiApex = [coords[1][0], coords[1][1]];
					coords = this.aplaxKJS.getOrderedESCoordinates(800,600);
					if(coords){	//Ensure that es was chosen
						for(var i = 0; i < coords.length; i++){
							data.aplax.escoordinates.push({x: coords[i][0], y : coords[i][1]});
						}
						data.aplax.esframe = ""+(Math.floor(this.aplaxKJS.getEStime()*this.aplaxData.movieFrameRate)+1);
						pointsChosen = true;
					}else{
						 var ald = new AlertDialog({message:"ES base plane landmarks for Aplax view has not been set",buttonLabel:"Ok"});
						 ald.show();
					}
	            }
			}
			if(this.tchKJS&&this.tchSelected){
	            if(this.tchKJS.orderedAnchorPoints.length > 0){
	                data.tch = {
	                    imageid: this.tchData.uid[0],
	                    rwavestart: this.tchData.rwavestart,
	                    rwaveend: this.tchData.rwaveend,
	                    bpm: this.tchData.bpm,
	                    coordinates: new Array(),
	                    escoordinates: new Array()
	                };
					var coords = this.tchKJS.getOrderedCoordinates(800,600);
					for(var i = 0; i < coords.length; i++){
						data.tch.coordinates.push({x: coords[i][0], y : coords[i][1]});
					}
					coords = this.tchKJS.getOrderedESCoordinates(800,600);
					if(this.epiApex==null)
						this.epiApex = [coords[1][0], coords[1][1]];
					if(coords){	//Ensure that es was chosen
						for(var i = 0; i < coords.length; i++){
							data.tch.escoordinates.push({x: coords[i][0], y : coords[i][1]});
						}
						data.tch.esframe = ""+(Math.floor(this.tchKJS.getEStime()*this.tchData.movieFrameRate)+1);
						pointsChosen = true;
	            	}else{
						 var ald = new AlertDialog({message:"ES base plane landmarks for Two Chamber view has not been set",buttonLabel:"Ok"});
						 ald.show();
					}
				}
			}
			if(this.fchKJS&&this.fchSelected){
				if(this.fchKJS.orderedAnchorPoints.length > 0){
					data.fch = {
						imageid: this.fchData.uid[0],
						rwavestart: this.fchData.rwavestart,
						rwaveend: this.fchData.rwaveend,
						bpm: this.fchData.bpm,
						coordinates: new Array(),
						escoordinates: new Array()
					};
					var coords = this.fchKJS.getOrderedCoordinates(800,600);
					for(var i = 0; i < coords.length; i++){
						data.fch.coordinates.push({x: coords[i][0], y : coords[i][1]});
					}
					if(this.epiApex==null)
						this.epiApex = [coords[1][0], coords[1][1]];
					coords = this.fchKJS.getOrderedESCoordinates(800,600);
					if(coords){//Ensure that es was chosen
						for(var i = 0; i < coords.length; i++){
							data.fch.escoordinates.push({x: coords[i][0], y : coords[i][1]});
						}
						data.fch.esframe = ""+(Math.floor(this.fchKJS.getEStime()*this.fchData.movieFrameRate)+1);
						pointsChosen = true;
					}else{
						 var ald = new AlertDialog({message:"ES base plane landmarks for Four Chamber view has not been set",buttonLabel:"Ok"});
						 ald.show();
					}
				}
			}
			if(this.saxbaseKJS&&this.saxbaseSelected){
	            if(this.saxbaseKJS.orderedAnchorPoints.length > 0){
	                data.saxbase = {
	                    imageid: this.saxBaseData.uid[0],
	                    rwavestart: this.saxBaseData.rwavestart,
	                    rwaveend: this.saxBaseData.rwaveend,
	                    bpm: this.saxBaseData.bpm,
	                    coordinates: new Array(),
	                };
					var coords = this.saxbaseKJS.getOrderedCoordinates(800,600);
					for(var i = 0; i < coords.length; i++){
						data.saxbase.coordinates.push({x: coords[i][0], y : coords[i][1]});
					}
					saxPointsChosen = true;
	            }
			}
			if(this.saxmidKJS&&this.saxmidSelected){
	            if(this.saxmidKJS.orderedAnchorPoints.length > 0){
	                data.saxmid = {
	                    imageid: this.saxMidData.uid[0],
	                    rwavestart: this.saxMidData.rwavestart,
	                    rwaveend: this.saxMidData.rwaveend,
	                    bpm: this.saxMidData.bpm,
	                    coordinates: new Array(),
	                };
					var coords = this.saxmidKJS.getOrderedCoordinates(800,600);
					for(var i = 0; i < coords.length; i++){
						data.saxmid.coordinates.push({x: coords[i][0], y : coords[i][1]});
					}
					saxPointsChosen = true;
	            }
			}
			if(this.saxapexKJS&&this.saxapexSelected){
	            if(this.saxapexKJS.orderedAnchorPoints.length > 0){
	                data.saxapex = {
	                    imageid: this.saxApexData.uid[0],
	                    rwavestart: this.saxApexData.rwavestart,
	                    rwaveend: this.saxApexData.rwaveend,
	                    bpm: this.saxApexData.bpm,
	                    coordinates: new Array(),
	                };
					var coords = this.saxapexKJS.getOrderedCoordinates(800,600);
					for(var i = 0; i < coords.length; i++){
						data.saxapex.coordinates.push({x: coords[i][0], y : coords[i][1]});
					}
					saxPointsChosen = true;
	            }
			}


			if(pointsChosen == false ){
				if(!saxPointsChosen){
					 var ald = new AlertDialog({message:"No/Incomplete landmarks where chosen: Unable to submit request",buttonLabel:"Ok"});
					 ald.show();
				}else{
					 var ald = new AlertDialog({message:"A completed long axis view with all landmarks is required: Unable to submit request",buttonLabel:"Ok"});
					 ald.show();
				}
				 return null;
			}else{
				data.formatType="SINGLEFRAME";
				return data;
			}

		},
		
		resizeVideoView: function(){
			try{
				this.clientWidth = this.bc.domNode.clientWidth;
				
				var brect = domGeom.position(this.leftPaneBcCenter.domNode);
				//var vrect = domGeom.position(this.video.domNode);
				var btrect = domGeom.position(this.leftPaneBcBottom.domNode);
				//var ratio = vrect.w/vrect.h;
				var ratio = 1.5;
				var nvh = brect.h - btrect.h*2/3 - 10;
				var nvw = nvh*ratio;
				var margin = (brect.w - nvw - 10)/2;
				if(nvw<200 && brect.w < 200){
					nvw = 200;
					margin = 0;
				}
				if(nvh<150)
					nvh = 150;
				this.video.domNode.style.width = nvw + "px";
				this.video.domNode.style.height = nvh + "px";
				brect = domGeom.position(this.video.domNode);
				this.video.domNode.style.marginLeft = margin + "px";
			}catch(e){
				
			}
		},
		
		addStudyDetails: function(){

			this.detailsTableContainer = new TableContainer({				
			});

		    //console.log("studyDate: " + this.study.studyDate);
            var dateTokens = this.study.studyDate.split(" "); //eg "12-10-2000 10:44:30.0000"
            dateTokens[1] = dateTokens[1].replace(/\./g, ":"); // replace . with : in the time field
            //console.log("dateTokens 0 : " + dateTokens[0]);
            //console.log("dateTokens 1 : " + dateTokens[1]);
			
			var studyDate = locale.parse(dateTokens[0],{
				selector: "date",
				datePattern: "dd-MM-yyyy",
			}); 

			var studyTime = locale.parse(dateTokens[1],{
				selector: "time",
				timePattern: "KK:mm:ss:SSSS",
			}); 

			this.studyDateTextBox = new DijitTextBox({
				label: "Study Date",
				//readOnly: true,
				disabled: true,
				style: "width: 6em",
				value: locale.format(studyDate,{
					selector: "date",
				    formatLength: "short"
				})
			});
			this.studyDateTextBox.placeAt(this.detailsTableContainer.containerNode);

			this.studyTimeTextBox = new DijitTextBox({
				label: "Study Time",
				disabled: true,
				style: "width: 6em",
				value: locale.format(studyTime,{
					selector: "time",
				    formatLength: "short"
				})
			});
			this.studyTimeTextBox.placeAt(this.detailsTableContainer.containerNode);

			this.detailsTableContainer.placeAt(this.leftPaneBcLeft);
			this.detailsTableContainer.startup();
			
			this.videoDetailsTableContainer = new TableContainer({				
			});
			this.videoDetailsTableContainer.placeAt(this.leftPaneBcLeft);
			this.videoDetailsTableContainer.startup();
			
	        //Setup view properties
		    this.propertiesFieldset = new Dialog({
			        title: "Scan metadata",
			        style: "width: 450px"
			});
	
		    this.showpropertiesButton = new Button({
					label: "Show Properties",
					onClick: lang.hitch(this,function(){this.propertiesFieldset.show();})
			});
				
			this.showpropertiesButton.placeAt(this.leftPaneBcLeft.domNode);
	        
			
			
		},
		
		addViewProperties: function(){
			if(this.propertiesFieldset){
				domConstruct.destroy(this.propertiesFieldset);
			}
				this.propertiesFieldset = new Dialog({
			        title: "Scan metadata",
			        style: "width: 450px"
			    });
				   
			    this.propertiesCP = new ContentPane({
			    	style:"overflow: auto; height: auto;"
	            });
	
			    this.propertiesCP.placeAt(this.propertiesFieldset);
			    	    
				this.propertiesTableContainer = new TableContainer({	
					labelWidth: "160px"
				});
			
				for(var i =0; i < this.view.properties.length; i++){
					var label = Object.getOwnPropertyNames(this.view.properties[i])[0];
					var propertyTextBox = new DijitTextBox({
						label: label,
						value: this.view.properties[i][label],
						disabled: true
					});
					propertyTextBox.placeAt(this.propertiesTableContainer.containerNode);
				}
	
				this.propertiesTableContainer.placeAt(this.propertiesCP);
				this.propertiesTableContainer.startup();
		},

		addViewVideo: function(){
			if(this.video){
				this.video.stop();
				this.video.destroyRecursive();
			}
			
			this.video = new Video({
		          source: [{src: this.server + this.view.video.mp4, type:"video/mp4"},
		                   {src: this.server + this.view.video.ogg, type:"video/ogg"},
		                   {src: this.server + this.view.video.webm, type:"video/webm"}],
				//style: "height: 100%",
		        "class": "viewVideo",
				controls: true,
				loop: true,
				startpos: (this.view.beginningRwave)/this.view.movieFrameRate
		    });
			
			this.video.placeAt(this.leftPaneBcCenter.domNode);

		    this.video.startup();
		    
			if(this.videoFieldset){
				domConstruct.destroy(this.videoFieldset);
			}
			
			//this.videoFieldset = domConstruct.create("div", {"class":"viewPropertiesFieldset"}, this.leftPaneBcLeft.domNode);
			this.videoFieldset = domConstruct.create("div", {"class":"viewPropertiesFieldset"}, this.videoDetailsTableContainer.containerNode);
			
			this.videoTableContainer = new TableContainer({		
				//customClass: "viewVideo",
				labelWidth: "150px"
			});
		
			var zoomTextBox = new DijitTextBox({
				label: "Zoom",
				value: this.view.zoom === undefined ? "NA" : this.view.zoom ,
				//readOnly: true,
				disabled: true,
				style: "width: 4em",
			});
			zoomTextBox.placeAt(this.videoTableContainer.containerNode);

			var time = (this.view.beginningRwave)/this.view.movieFrameRate;
			this.view.beginningRwaveTime = time;
			var pTime = sprintf("%.4f",time);
			this.beginningRwaveTextBox = new DijitTextBox({
				label: "Beginning R Wave (secs)",
				//value: this.view.beginningRwave,
				value: pTime,
				readOnly: false,
				style: "width: 4em"
			});
			this.beginningRwaveTextBox.placeAt(this.videoTableContainer.containerNode);
			
			time = (this.view.followingRwave)/this.view.movieFrameRate;	
			this.view.followingRwaveTime = time;
			
			pTime = sprintf("%.4f",time);
			this.followingRwaveTextBox = new DijitTextBox({
				label: "Following R Wave (secs)",
				//value: this.view.followingRwave,
				value: pTime,
				readOnly: false,
				style: "width: 4em",
			});
			this.followingRwaveTextBox.placeAt(this.videoTableContainer.containerNode);

			this.bpmTextBox = new DijitTextBox({
				label: "BPM",
				value: this.view.beatsPerMin,
				//readOnly: true,
				readOnly: false,
				style: "width: 4em",
				onChange: lang.hitch(this, function(value){
					this.view.beatsPerMin = parseInt(value);
				})
			});
			this.bpmTextBox.placeAt(this.videoTableContainer.containerNode);

			this.videoTableContainer.placeAt(this.videoFieldset);
			this.videoTableContainer.startup();
			
			var waveButtonsTable = domConstruct.create("table", {"class":"waveButtonsTable"}, this.videoFieldset);
			var waveButtonsTr = domConstruct.create("tr", {"class":""}, waveButtonsTable);
			//Add a checkbox to toggle between set from video or set from box
			waveButtonsTd = domConstruct.create("td", {"class":""}, waveButtonsTr);
			domConstruct.create("label", {"for":"toggleChkBox",innerHTML:"Set values from Video"},waveButtonsTd);

            waveButtonsTd = domConstruct.create("td", {"class":""}, waveButtonsTr);
            this.toggleChkBox = new CheckBox({
                label: "Set From Video",
                checked: true,
                onChange: lang.hitch(this, this.ontoggleChkBox)
            },waveButtonsTd);
			
			waveButtonsTr = domConstruct.create("tr", {"class":""}, waveButtonsTable);
			waveButtonsTd = domConstruct.create("td", {"class":""}, waveButtonsTr);
			var beginningRwaveButton = new Button({
				label: "Beginning R Wave",
				onClick: lang.hitch(this, this.onBeginningRwaveClick)
			});
			beginningRwaveButton.placeAt(waveButtonsTd);
			
			//To place it one below the other to save space
			waveButtonsTr = domConstruct.create("tr", {"class":""}, waveButtonsTable);
			waveButtonsTd = domConstruct.create("td", {"class":""}, waveButtonsTr);
			var followingRwaveButton = new Button({
				label: "Following R Wave",
				onClick: lang.hitch(this, this.onFollowingRwaveClick)
			});
			followingRwaveButton.placeAt(waveButtonsTd);
			
		},
		
		ontoggleChkBox: function() {
				this.setFromVideo = this.toggleChkBox.get('value');
		},
		
		onBeginningRwaveClick: function(){
			this.setFromVideo = this.toggleChkBox.get('value');

			if(this.setFromVideo){
				var frame = Math.floor(this.video.videoNode.currentTime * this.view.movieFrameRate) + 1;
				if(frame>this.view.lastFrame*1.0)
					frame = this.view.lastFrame*1.0 -1;
				//var upperFrame = parseInt(this.followingRwaveTextBox.get("value"));
				var upperFrame = Math.floor(parseFloat(this.followingRwaveTextBox.get("value")) * this.view.movieFrameRate) + 1;
	
				if(frame < upperFrame){
					 this.view.beginningRwave = ""+frame; //Convert to string else json parsing will bomb
				     //this.beginningRwaveTextBox.set("value", frame);
					 var time = (frame-1)/this.view.movieFrameRate;
					 this.view.beginningRwaveTime = time;
					 var pTime = sprintf("%.4f",time);
					 this.beginningRwaveTextBox.set("value", pTime);
				}else{
					 this.view.beginningRwave = ""+frame; //Convert to string else json parsing will bomb
				     
					 var time = (frame-1)/this.view.movieFrameRate;
					 this.view.beginningRwaveTime = time;
					 var pTime = sprintf("%.4f",time);
					 this.beginningRwaveTextBox.set("value", pTime);
					 //Set the following rwave to be up by 500 msecs
					 var followFrame = Math.floor((this.video.videoNode.currentTime+0.5) * this.view.movieFrameRate);
					 if(followFrame>this.view.lastFrame*1.0)
						 followFrame= this.view.lastFrame -1;
					 this.view.followingRwave = ""+followFrame;
					 time = followFrame/this.view.movieFrameRate;
					 this.view.followingRwaveTime = time;
					 pTime = sprintf("%.4f",time);
					 this.followingRwaveTextBox.set("value", pTime);
				}
			}else{
				var beginningRwaveValue = parseFloat(this.beginningRwaveTextBox.get("value"));
				var followingRwaveValue = parseFloat(this.followingRwaveTextBox.get("value"));
				if(beginningRwaveValue>followingRwaveValue){
					alert("Begining R wave should occur prior to the following R wave! Following R wave position is adjusted");
					var followFrame = Math.floor((beginningRwaveValue+0.5) * this.view.movieFrameRate);
					if(followFrame>this.view.lastFrame*1.0)
						 followFrame= this.view.lastFrame -1;
					this.view.followingRwave = ""+followFrame;
					var time = followFrame/this.view.movieFrameRate;
					this.view.followingRwaveTime = time;
					var pTime = sprintf("%.4f",time);
					this.followingRwaveTextBox.set("value", pTime);
				}
				var frame = Math.floor(beginningRwaveValue*this.view.movieFrameRate);
				if(frame>this.view.lastFrame*1.0){
					frame = this.view.lastFrame*1.0-1.0;
					beginningRwaveValue = frame/this.view.movieFrameRate;
				}
				this.view.beginningRwave = ""+frame;
				//this.video.videoNode.currentTime = beginningRwaveValue;
				this.video.setTime(beginningRwaveValue);
				this.view.beginningRwaveTime = beginningRwaveValue; 
				var pTime = sprintf("%.4f",beginningRwaveValue);
				this.beginningRwaveTextBox.set("value", pTime);
			}
		},

		onFollowingRwaveClick: function(){
			this.setFromVideo = this.toggleChkBox.get('value');

			if(this.setFromVideo){
				var frame = Math.floor(this.video.videoNode.currentTime * this.view.movieFrameRate) + 1;
				if(frame>this.view.lastFrame*1.0 || frame <1)
					frame = this.view.lastFrame*1.0;
				
				//var lowerFrame = parseInt(this.beginningRwaveTextBox.get("value"));
				var lowerFrame = Math.floor(parseFloat(this.beginningRwaveTextBox.get("value")) * this.view.movieFrameRate) + 1;
				
				if(lowerFrame < frame){
					 this.view.followingRwave = ""+frame;
	                 //this.followingRwaveTextBox.set("value", frame);
					 var time = (frame-1)/this.view.movieFrameRate;
					 this.view.followingRwaveTime = time;
					 var pTime = sprintf("%.4f",time);
					 this.followingRwaveTextBox.set("value", pTime);
	            }else{
	            	 this.view.followingRwave = ""+frame; //Convert to string else json parsing will bomb
				     
					 var time = (frame-1)/this.view.movieFrameRate;
					 this.view.followingRwaveTime = time;
					 var pTime = sprintf("%.4f",time);
					 this.followingRwaveTextBox.set("value", pTime);
					 //Set the begining rwave to be down by 500 msecs
					 var followFrame = Math.floor((this.video.videoNode.currentTime- 0.5) * this.view.movieFrameRate);
					 if(followFrame<0)
						 followFrame = 0;
					 this.view.beginningRwave = ""+followFrame;
					 time = followFrame/this.view.movieFrameRate;
					 this.view.beginningRwaveTime = time;
					 pTime = sprintf("%.4f",time);
					 this.beginningRwaveTextBox.set("value", pTime);
	            }
			}else{
				var beginningRwaveValue = parseFloat(this.beginningRwaveTextBox.get("value"));
				var followingRwaveValue = parseFloat(this.followingRwaveTextBox.get("value"));
				if(beginningRwaveValue>followingRwaveValue){
					alert("Following R wave should occur after the begining R wave! Begining R wave position is adjusted");
					var followFrame = Math.floor((followingRwaveValue-0.5) * this.view.movieFrameRate) -1;
					 if(followFrame<0)
						 followFrame = 0;
					this.view.beginningRwave = ""+followFrame;
					var time = followFrame/this.view.movieFrameRate;
					this.view.beginningRwaveTime = time;
					var pTime = sprintf("%.4f",time);
					this.beginningRwaveTextBox.set("value", pTime);
				}
				var frame = Math.floor(followingRwaveValue*this.view.movieFrameRate);
				if(frame>this.view.lastFrame*1.0){
					frame = this.view.lastFrame*1.0;
					followingRwaveValue = frame/this.view.movieFrameRate;
				}
				this.view.followingRwave = ""+frame;
				//this.video.videoNode.currentTime = followingRwaveValue;
				this.video.setTime(followingRwaveValue);
				this.view.followingRwaveTime = followingRwaveValue;
				var pTime = sprintf("%.4f",followingRwaveValue);
				this.followingRwaveTextBox.set("value", pTime);
			}
		},

		addViewsCarousel: function(){
			this.viewsCarousel = new Carousel({
				height: "150px",
				//height: "",
	            navButton: true,
	            numVisible: 5,
	            title: "Views",
	            // Overriding resizeItems() to patch broken height calculation
	    		resizeItems: lang.hitch( this.viewsCarousel, function(){
	    			// summary:
	    			//		Resizes the child items of the carousel.
	    			var idx = 0;
	    			//debugger;
	    			//var h = this.domNode.offsetHeight - (this.headerNode ? this.headerNode.offsetHeight : 0);
	    			var m = has("ie") ? 5 / this.numVisible-1 : 5 / this.numVisible;
	    			array.forEach(this.getChildren(), function(view){
	    				if(!(view instanceof SwapView)){ return; }
	    				if(!(view.lazy || view.domNode.getAttribute("lazy"))){
	    					view._instantiated = true;
	    				}
	    				var ch = view.containerNode.childNodes;
	    				for(var i = 0, len = ch.length; i < len; i++){
	    					var node = ch[i];
	    					if(node.nodeType !== 1){ continue; }
	    					var item = this.items[idx] || {};
	    					domStyle.set(node, {
	    						width: item.width || (90 / this.numVisible + "%"),
	    						//height: item.height || h + "px",
	    						margin: "0 " + (item.margin || m + "%")
	    					});
	    					domClass.add(node, "mblCarouselSlot");
	    					idx++;
	    				}
	    			}, this);

	    			if(this.piw){
	    				this.piw.refId = this.containerNode.firstChild;
	    				this.piw.reset();
	    			}
	    		})
			});

			var nPages = Math.ceil(Object.getOwnPropertyNames(this.study.views).length / this.viewsCarousel.numVisible);
			for(var i = 1; i <= nPages; i++){
				var w = new SwapView({
					lazy:true
				});
				this.viewsCarousel.addChild(w);
				var j = 1;
				for(entry in this.study.views){
					if((j > (i-1)*this.viewsCarousel.numVisible) && (j <= i*this.viewsCarousel.numVisible)){
						var view = this.study.views[entry];
						//console.debug(this.study.studyUID+' '+i+','+j+' '+entry+ ' imageid '+view.imageid);
						var item = new CarouselItem({
								src: this.server + view.image, 
								value: entry,
								studyObj: this.study //Added by jagir to get access to the study object
							});
						item.placeAt(w.containerNode);
					}
					j++;
					
				}
			}
			
			this.viewsCarousel.placeAt(this.leftPaneBcBottom);
			this.viewsCarousel.startup();

			topic.subscribe("/dojox/mobile/carouselSelect", lang.hitch(this, this.onViewClick));
		},
		
		addFitSelectionTarget: function(){
			this.selectionTable = domConstruct.create("table", {"class":""}, this.right.containerNode);
			
			var tr = domConstruct.create("tr", {"class":""}, this.selectionTable);

			this.fchSelectionTd = domConstruct.create("td", {
				viewname: "4CH",
				onclick: lang.hitch(this, this.onSelectionTargetClick),
				"class":"fitSelectionTarget fchTarget"
			}, tr);

			this.saxSelectionTd = domConstruct.create("td", {
				viewname: "SAXAPEX",
				onclick: lang.hitch(this, this.onSelectionTargetClick),
				"class":"fitSelectionTarget saxapexTarget"
			}, tr);
			
			tr = domConstruct.create("tr", {"class":""}, this.selectionTable);
			this.tchSelectionTd = domConstruct.create("td", {
				viewname: "2CH",
				onclick: lang.hitch(this, this.onSelectionTargetClick),
				"class":"fitSelectionTarget tchTarget"
			}, tr);
			
			this.saxSelectionTd = domConstruct.create("td", {
				viewname: "SAXMID",
				onclick: lang.hitch(this, this.onSelectionTargetClick),
				"class":"fitSelectionTarget saxmidTarget"
			}, tr);

			tr = domConstruct.create("tr", {"class":""}, this.selectionTable);
			this.aplaxSelectionTd = domConstruct.create("td", {
				viewname: "APLAX",
				onclick: lang.hitch(this, this.onSelectionTargetClick),
				"class":"fitSelectionTarget aplaxTarget"
			}, tr);
			
			this.saxSelectionTd = domConstruct.create("td", {
				viewname: "SAXBASE",
				onclick: lang.hitch(this, this.onSelectionTargetClick),
				"class":"fitSelectionTarget saxbaseTarget"
			}, tr);
			
			tr = domConstruct.create("tr", {"class":""}, this.selectionTable);
			var fitButtonTd = domConstruct.create("td", {"class":"","colspan":"2"}, tr);

			var fitButton = new Button({
				label: "Fit Model",
				"class": "fitSelectionButton",
				onClick: lang.hitch(this, this.onFitClick)
			});
			fitButton.placeAt(fitButtonTd);
		},
		
		onSelectionTargetClick: function(evt){
			var target  = evt.currentTarget;
			
			if(target.image){
				alert("A view has been assigned to this imaging plane. Remove the assigned view to continue.");
				return;
				//this.destroySelectedTargetImage(target);
			}
			
			if(this.view){
				if(this.isViewValid()){

                    // delete any other targets with the same image
                    var targets = query("td", this.selectionTable);
                    for(var i = 0; i < 4; i++){
                        if(targets[i].data){ 
                            if(this.view.imageid == targets[i].data.imageid){
                				alert("This view has already been assigned to an imaging plane. Remove the view from that imaging plane to continue.");
                				return;
                            	//this.destroySelectedTargetImage(targets[i]);
                            }
                        }
                    }
                    
					target.image = domConstruct.create("img", {
						src: this.server + this.view.image, 
						"class":"fitSelectionImg"
					}, target);
                                    					
/*					target.dataX = {
							imageid: this.view.imageid,
							beginningRwave: this.view.beginningRwave,
							followingRwave: this.view.followingRwave,
							beginningRwaveTime: this.view.beginningRwaveTime,
							followingRwaveTime: this.view.followingRwaveTime,							
							beatsPerMin: parseInt(this.bpmTextBox.get("value")),
							movieFrameRate: this.view.movieFrameRate
					};*/
					target.dataX = {
							beatsPerMin: parseInt(this.bpmTextBox.get("value")),
					};
					
					target.data = this.view;
					
					target.deleteDiv = domConstruct.create("div", {
						onclick: lang.hitch(this, this.onDeleteSelectedTargetClick),
						"class":"fitSelectionDelete"
					}, target);	
					

				}else{
					this.alertViewInvalidShow();
				}
			}
		},
		
		destroySelectedTargetImage: function(target){
			if(target.image){
				domConstruct.destroy(target.image);
			}
			target.image = null;
			target.data = null;
			if(target.deleteDiv){
				domConstruct.destroy(target.deleteDiv);
			}
		},
		
		onDeleteSelectedTargetClick: function(evt){
			event.stop(evt); 
			this.destroySelectedTargetImage(evt.currentTarget.parentNode);	
		},
		
		isViewValid: function(){
			return ((this.view.beginningRwave != "NA") && (this.view.followingRwave != "NA") && (this.view.beatsPerMin != "NA")) ? true : false;
		},
		
		alertViewInvalidShow: function(){
			ABI.lib.Widgets.AlertSingleton.alert("Invalid Parameters", "Beginning and Following R Wave, and BPM parameters must be set");
			return;
		},
		
		onViewClick: function(carousel, itemWidget, itemObject, index){
			//All the necessary data is in itemWidget, Code modified by Jagir
			var studyviews = itemWidget.params.studyObj.views[itemWidget.params.value];
			//Only if view has changed
			if(this.view!=undefined && this.view.imageid==studyviews.imageid){
				return;
			}
			try{
				// Append a % to the zoom value string if there isnt one already
				if(!/%$/.test(studyviews[itemWidget.params.value].zoom)){
					studyviews[itemWidget.params.value].zoom = studyviews[itemWidget.params.value].zoom + "%";
				}
			}catch(e){
				//this.study.views[itemWidget.params.value].zoom = "NA" ;
				//This occurs when the object to view mapping is corrupt
			}
			
			try{
				if(!studyviews[itemWidget.params.value].beginningRwave){
					studyviews[itemWidget.params.value].beginningRwave = "NA" ;
				}
			}catch(ex){
				
			}

			try{
				if(!studyviews[itemWidget.params.value].followingRwave){
					studyviews[itemWidget.params.value].followingRwave = "NA" ;
				}
			}catch(ex2){
				
			}

			try{
				if(!studyviews[itemWidget.params.value].beatsPerMin){
					studyviews[itemWidget.params.value].beatsPerMin = "NA" ;
				}
			}catch(ex3){
				
			}

			this.view = lang.clone(studyviews);

			this.displayView();
		},
		
		displayView: function(){
			//console.dir(this.view);
			this.addViewVideo();
			this.addViewProperties();	
			this.resizeVideoView();
		},
		
		renderFittingResult : function(){
			request.post(this.SXFIT+"?workflowID="+this.POLLHANDLE+"&result=true").then(lang.hitch(this, function(data){

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
		
		
        fitModel: function(dataX){
        	var data = dataX;
        	data.GELEM_CENTROID = [this.xDIM/2,this.yDIM/2];
        	//Start the progress bar here, since the ctx return happens after speckle tracking
			this.progressIndicator = new ProgressBar({headline:"Model Fitting Progress"});
       		this.progressIndicator.show();
          	request.post(this.SXFIT, {
        		data : dojo.toJson(data),
        	}).then(lang.hitch(this, function(data){
        		this.fitRequestInProgress = false;
        		try{
	        		var jsonD = dojo.fromJson(data);
	        		if(jsonD.workflowID){
	        			this.POLLHANDLE = jsonD.workflowID;
	        			this.POLLTIMERHANDLE = setInterval(lang.hitch(this,this.pollForFittingStatus),this.POLLINTERVAL);
	        			this.pollForFittingStatus();
	        		}else{
	       			 var ald = new AlertDialog({message:"Service failed "+data,iconClass:"Error"});
	    			 ald.show();
	        		}
        		}catch(e){
		        	try{
	        			this.progressIndicator.hide();
		        	}catch(e){
		        		
		        	}
	       			 var ald = new AlertDialog({message:"Service failed "+data,iconClass:"Error"});
	    			 ald.show();        			
        		}
            }), function(err){
        		this.fitRequestInProgress = false;
	        	try{
        			this.progressIndicator.hide();
	        	}catch(e){
	        		
	        	}
      			var ald = new AlertDialog({message:"Service failed "+err,iconClass:"Error"});
    			ald.show();        						
            	console.log("Error occured"+err);
            });
          	this.fitRequestInProgress = false;
        },
		
		
		onFit: function(){
			if(!this.fitRequestInProgress){
				this.fitRequestInProgress = true;
				if(this.SPECKLETRACKINGDATA==null){
					var data = this.collateData();
					if(data!=null){
						this.fitModel(data);
					}else{
						this.fitRequestInProgress = false;
					}
				}else{
					//Collect the data from verifiers
					//Clone object
					var serverdata = JSON.parse(JSON.stringify(this.SPECKLETRACKINGDATA));
					serverdata.views = new Array();
					var vctr = 0;
					if(this.aplaxSPEC){ 
						if(this.aplaxViewSelected){
							serverdata.APLAX.ENDO.MARKERS=this.aplaxSPEC.getMarkers();
							serverdata.views.push("APLAX");
							vctr++;
						}else{
							delete serverdata.APLAX;
						}
					}
					if(this.tchSPEC){
						if(this.tchViewSelected){
							serverdata.TCH.ENDO.MARKERS=this.tchSPEC.getMarkers();
							vctr++;
							serverdata.views.push("TCH");
							vctr++;
						}else{
							delete serverdata.TCH;
						}
					}
					if(this.fchSPEC){
						if(this.fchViewSelected){
							serverdata.FCH.ENDO.MARKERS=this.fchSPEC.getMarkers();
							serverdata.views.push("FCH");
							vctr++;
						}else{
							delete serverdata.FCH;
						}
					}
					if(this.saxapexSPEC){
						if(this.saxapexViewSelected){
							serverdata.SAXAPEX.ENDO.MARKERS=this.saxapexSPEC.getMarkers();
							serverdata.views.push("SAXAPEX");
							vctr++;
						}else{
							delete serverdata.SAXAPEX;
						}
					}
					if(this.saxmidSPEC){
						if(this.saxmidViewSelected){
							serverdata.SAXMID.ENDO.MARKERS=this.saxmidSPEC.getMarkers();
							serverdata.views.push("SAXMID");
							vctr++;
						}else{
							delete serverdata.SAXMID;
						}
					}
					if(this.saxbaseSPEC){
						if(this.saxbaseViewSelected){
							serverdata.SAXBASE.ENDO.MARKERS=this.saxbaseSPEC.getMarkers();
							serverdata.views.push("SAXBASE");
							vctr++;
						}else{
							delete serverdata.SAXBASE;
						}
					}
					if(vctr == 0){
						 this.fitRequestInProgress = false;
						 var ald = new AlertDialog({message:"No Views selected",iconClass:"Error"});
						 ald.show();
						 return null;
					}
					serverdata.selectedStrains = this.selectedStrains;
					
					serverdata.MODELNAME = this.speckleModelTextBox.get("value"); //In case name is changed
					if(serverdata.MODELNAME.length == 0){
					 this.fitRequestInProgress = false;
					 var ald = new AlertDialog({message:"Specify a model name",iconClass:"Error"});
					 ald.show();
					 return null;
					}
					
					serverdata.formatType="SPECKLETRACKED";
					//serverdata.workflowID=this.POLLHANDLE;
					serverdata.workflowID = this.speckleTrackingWorkflowID;
					this.fitModel(serverdata);
			   }
			}
		},
		
		playStop: function(){
			if(!this.playerId){
				//console.log("Play");
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
            	//console.log("Stop");
                clearInterval(this.playerId);
                this.playerId = null;
                this.playButton.set("iconClass", "playIcon");
                this.playButton.set("label", "Play");
            }     
        },
		
		
		setSpeckleTime : function(){
			for(var i=0;i<this.activeSPEC.length;i++){
				this.activeSPEC[i].setStep(Math.round(this.time));
            }
            
            var plot = this.strainchart.chart.getPlot("indicator");
            if(plot){
            	try{
            		var ptime = (Math.round(this.time))/(this.maxCAPFrames-1);
                    plot.opt.values = [ptime];
                    plot.dirty = true;
                    this.strainchart.chart.render();
            	}catch(e){
            		//console.log("error indicator update");
            	}
            }   
		}, 
		
		computePGLS: function(strainData){
			var avg = new Array();
			for(k in strainData){
				if(this.selectedStrains[k]){
					var sd = strainData[k];
					for(var i=0;i<sd.length;i++)
						avg.push(0.0);
					break;
				}
			}
			var ctr  = 0;
			for(k in strainData){
				if(this.selectedStrains[k]){
					if(!isNaN(parseFloat(k)) && isFinite(k)){
						var sd = strainData[k];
						for(var i=0;i<sd.length;i++){
							avg[i] += sd[i].y;
						}
						ctr++;
					}
				}
			}
			var pk = avg[0];
			for(var i=1;i<avg.length;i++){
				if(Math.abs(pk)<Math.abs(avg[i])){
					pk = avg[i];
				}
			}
			pk /= ctr;
			return pk;
		},
		
		
		refreshGraph: function (){
			if(!this.refreshingGraph && this.strainchart != undefined){
				this.refreshingGraph = true;
				var strainData = new Object();
				var selected = new Object();
				if(arguments[1]){
					if(arguments[1]=="TCH"||arguments[1]=="tch"){
						selected["TCH"] = this.tchSelected;
					}
					if(arguments[1]=="FCH"||arguments[1]=="fch"){
						selected["FCH"] = this.fchSelected;
					}
					if(arguments[1]=="APLAX"||arguments[1]=="APLAX"){
						selected["APLAX"] = this.aplaxSelected;
					}
				}else{
					selected["TCH"] = this.tchViewSelected;
					selected["FCH"] = this.fchViewSelected;
					selected["APLAX"] = this.aplaxViewSelected;			
				}
	
				if(this.tchSPEC && selected["TCH"]){
					var sd =  this.tchSPEC.computeStrains();
					strainData["15"] = sd[2];
					strainData["13"] = sd[3];
					strainData["10"] = sd[1];
					strainData["7"] = sd[4];
					strainData["4"] = sd[0];
					strainData["1"] = sd[5];
				}
				if(this.fchSPEC && selected["FCH"]){
					var sd =  this.fchSPEC.computeStrains();
					strainData["16"] = sd[3];
					strainData["14"] = sd[2];
					strainData["12"] = sd[4];
					strainData["9"] = sd[1];
					strainData["6"] = sd[5];
					strainData["3"] = sd[0];
				}
				if(this.aplaxSPEC && selected["APLAX"]){
					var sd =  this.aplaxSPEC.computeStrains();
					strainData["8"] = sd[4];
					strainData["11"] = sd[1];
					strainData["2"] = sd[5];
					strainData["5"] = sd[0];
	/*				strainData["15p16"] = sd[2];
					strainData["13p14"] = sd[3];*/
				}
				
				for(var k in strainData){
					try{
						this.strainchart.chart.removeSeries(k);
					}catch(e){
						
					}
					if(this.selectedStrains[k]){
		            	if(/^[0-9]+$/.test(k)){ //Ignore 14p15 etc
		            		this.strainchart.chart.addSeries(k,strainData[k],{stroke: {color: this.plotColours[k],style: this.plotStyles[k]}});
		            	}else{
		            		var id = k.substring(0,2);
		            		this.strainchart.chart.addSeries(k,strainData[k],{stroke: {color: this.plotColours[id],style: this.plotStyles[id]}});
		            	}
					}
	            }
				var pgls = this.computePGLS(strainData);
				this.specklePGLSTextBox.set("value",pgls);
				
				this.strainchart.chart.render();
				this.strainchartLegend.refresh();
				this.refreshingGraph = false;
			}
		},

		selectAplaxCBXEvt : function (isChecked){
			this.aplaxViewSelected = isChecked;
			var arr = new Array("2","8","11","5");
			if(!isChecked){
				for(var i=0;i<arr.length;i++){
	        		this.strainchart.chart.removeSeries(arr[i]);
	        		this.selectedStrainsCbxs[arr[i]].set('checked',false);
				}
			}else{
				for(var i=0;i<arr.length;i++){
	        		this.selectedStrainsCbxs[arr[i]].set('checked',true);
				}
			}
			this.refreshGraph("APLAX");
		},
		
		selectTCHCBXEvt: function (isChecked){
			this.tchViewSelected = isChecked;
			var arr = new Array("4","10","15","13","7","1");
			if(!isChecked){
				for(var i=0;i<arr.length;i++){
	        		this.strainchart.chart.removeSeries(arr[i]);
	        		this.selectedStrainsCbxs[arr[i]].set('checked',false);
				}
			}else{
				for(var i=0;i<arr.length;i++){
	        		this.selectedStrainsCbxs[arr[i]].set('checked',true);
				}
			}
			this.refreshGraph("TCH");
		},
		
		selectFCHCBXEvt: function (isChecked){
			this.fchViewSelected = isChecked;
			var arr = new Array("3","9","14","16","12","6");
			if(!isChecked){
				for(var i=0;i<arr.length;i++){
	        		this.strainchart.chart.removeSeries(arr[i]);
	        		this.selectedStrainsCbxs[arr[i]].set('checked',false);
				}
			}else{
				for(var i=0;i<arr.length;i++){
	        		this.selectedStrainsCbxs[arr[i]].set('checked',true);
				}
			}
			this.refreshGraph("FCH");
		},
		
		selectSaxApexCBXEvt: function (isChecked){
			this.saxapexViewSelected = isChecked;
		},
		
		selectSaxMidCBXEvt: function (isChecked){
			this.saxmidViewSelected = isChecked;
		},
		
		selectSaxBaseCBXEvt: function (isChecked){
			this.saxbaseViewSelected = isChecked;
		},
		
		createSpeckleVerificationPane: function(){
			
			if(this.speckleContainer){
				domConstruct.destroy(this.createSpeckleVerificationPaneBC);
				domConstruct.destroy(this.speckleContainer);
				domConstruct.destroy(this.graphContainer);
			}
			this.aplaxViewSelected = false;
			this.tchViewSelected = false;
			this.fchViewSelected = false;
			this.saxbaseViewSelected = false;
			this.saxmidViewSelected = false;
			this.saxapexViewSelected = false;
			
			var bc = new BorderContainer({
	            style: "height: 100%; width: 100%;"
	        },td);
			this.vbccenter.addChild(bc);
			bc.startup();
			this.createSpeckleVerificationPaneBC = bc;
			
	        this.speckleContainer = new ContentPane({
	        	 region: "center",
	        });
	        bc.addChild(this.speckleContainer);
	        
	        this.graphContainer = new ContentPane({
	        	 region: "right",
	        });
	        bc.addChild(this.graphContainer);
	        			
			var longAxisViews = new Array("APLAX","TCH","FCH");
			var shortAxisViews = new Array("SAXAPEX","SAXMID","SAXBASE");
			var activeViews = new Object();
			//Setup maxCAPFrames
			this.maxCAPFrames = 17;
			
			var numSelectedViews = 0;
			for(var c=0;c<longAxisViews.length;c++){
				if(this.SPECKLETRACKINGDATA[longAxisViews[c]]){
					var obj = this.SPECKLETRACKINGDATA[longAxisViews[c]]; 
					if(obj.ENDO){
						this.maxCAPFrames = obj.ENDO.MARKERS.length;
						activeViews[longAxisViews[c]] = obj;
						numSelectedViews++;
					}
				}	
			}
			for(var c=0;c<shortAxisViews.length;c++){
				if(this.SPECKLETRACKINGDATA[shortAxisViews[c]]){
					var obj = this.SPECKLETRACKINGDATA[shortAxisViews[c]]; 
					if(obj.ENDO){
						activeViews[shortAxisViews[c]] = obj;
						numSelectedViews++;
					}
				}	
			}
			if(numSelectedViews==0){//Some error occured during speckle tracking
	            this.vbccenter.removeChild(bc);
				domConstruct.destroy(bc);
				this.fittingPaneBc.removeChild(this.fittingcontainer);
				domConstruct.destroy(this.fittingcontainer);
	            this.bc.domNode.style.display = '';
	            this.vbc.domNode.style.display = 'none';
	            console.log(this.SPECKLETRACKINGDATA);
	            alert("Speckle tracking failed!! Contact administrator if problem persists");
			}
			//Create tables
			if(this.speckleVerificatonTable){
				domConstruct.destroy(this.speckleVerificatonTable);
				domConstruct.destroy(this.speckleActionTable);
			}
			//Note that fbc is the visible container and will have a client width others will not have
			var numcols = Math.floor(this.fbc.domNode.clientWidth/this.xDIM)-1;
			if(numcols<1)
				numcols = 1;
			if(numcols>2)
				numcols = 2;
			if(numcols>numSelectedViews)
				numcols = numSelectedViews;
			var numRows = Math.floor((numSelectedViews+numcols-1)/numcols);
			var graphDivs = new Array();

			this.speckleVerificatonTable = domConstruct.create("table", {"class":"", "style":"margin-left: auto; margin-right: auto; margin-top: auto"}, this.speckleContainer.containerNode);
			
			var offsetRow = numRows -1;
			var celloffset = -1;
			var offsetmargin = 0;
			//var numcells = numRows*numcols;
			
			//Give numbers based on size

			for(var ctr=0;ctr<offsetRow;ctr++){
				var tr = domConstruct.create("tr", {"class":""}, this.speckleVerificatonTable);
				for(var cc=0;cc<numcols;cc++){
					var td = domConstruct.create("td", {"class":""}, tr);
					graphDivs.push(domConstruct.create("div", {"class":""}, td));
				}
			}
			{
				var tr = domConstruct.create("tr", {"class":""}, this.speckleVerificatonTable);
				for(var cc=0;cc<numcols;cc++){
					var td = domConstruct.create("td", {"class":""}, tr);
					if(cc>celloffset){
						graphDivs.push(domConstruct.create("div", {"class":""}, td));
					}else{
						domStyle.set(td,"width",offsetmargin+"px");
						domConstruct.create("div", {"class":"","style":"width:"+offsetmargin+"px"}, td);
					}
				}
			}
            var tablectr = 0;			
		
			this.activeSPEC = new Array();
			//Kill anything that exists
			this.aplaxSPEC = null; 
			this.tchSPEC = null;
			this.fchSPEC = null;
			this.saxapexSPEC = null;
			this.saxmidSPEC = null;
			this.saxbaseSPEC = null;
			//Setup view ordering
			var viewOrder = new Array("FCH","TCH","APLAX","SAXAPEX","SAXMID","SAXBASE");
			for(var vo=0;vo<viewOrder.length;vo++){
				var kv = viewOrder[vo];
				if(activeViews[kv]===undefined)
					continue;
					var SPECKLETRACKINGDATA = activeViews[kv];
					var vtype = "long";
					if(vo>2)
						vtype = "sax";
					this.aSPEC = new SpeckleVerifier({
							element: graphDivs[tablectr++],
							xdim: this.xDIM,
							ydim: this.yDIM,
							xscalefactor: this.xscalefactor,
							yscalefactor: this.yscalefactor,
					        urls: SPECKLETRACKINGDATA.URLS,
					        markerArray: SPECKLETRACKINGDATA.ENDO.MARKERS,
					        type: vtype,
					        viewPlane: kv,
					        framevector: this.SPECKLETRACKINGDATA.FRAMEVECTORS[kv],
					        parentuuid: "/" + this.uuid
					});
					this.activeSPEC.push(this.aSPEC);
					switch(kv){
						case "APLAX": this.aplaxSPEC = this.aSPEC; this.aplaxViewSelected = true; break;
						case "TCH": this.tchSPEC = this.aSPEC; this.tchViewSelected = true; break;
						case "FCH": this.fchSPEC = this.aSPEC; this.fchViewSelected = true; break;
						case "SAXAPEX": this.saxapexSPEC = this.aSPEC; this.saxapexViewSelected = true; break;
						case "SAXMID": this.saxmidSPEC = this.aSPEC; this.saxmidViewSelected = true; break;
						case "SAXBASE": this.saxbaseSPEC = this.aSPEC; this.saxbaseViewSelected = true; break;
					}
			}
			
			//Create controls panel
			
			//Create graph
			this.selectedStrains = new Object();
			var strainData = new Object();
			if(this.tchSPEC){
				var sd =  this.tchSPEC.computeStrains();
				strainData["15"] = sd[2];
				strainData["13"] = sd[3];
				strainData["10"] = sd[1];
				strainData["7"] = sd[4];
				strainData["4"] = sd[0];
				strainData["1"] = sd[5];
			}
			if(this.fchSPEC){
				var sd =  this.fchSPEC.computeStrains();
				strainData["16"] = sd[3];
				strainData["14"] = sd[2];
				strainData["12"] = sd[4];
				strainData["9"] = sd[1];
				strainData["6"] = sd[5];
				strainData["3"] = sd[0];
			}
			if(this.aplaxSPEC){
				var sd =  this.aplaxSPEC.computeStrains();
				strainData["8"] = sd[4];
				strainData["11"] = sd[1];
				strainData["2"] = sd[5];
				strainData["5"] = sd[0];
/*				strainData["15p16"] = sd[2];
				strainData["13p14"] = sd[3];*/
			}
			var numstrains = 0;
			for(var k in strainData){
				this.selectedStrains[k] = true;
				numstrains = numstrains + 1;
			}
			
			
			var graphTableDiv = domConstruct.create("table", {"class":""}, this.graphContainer.containerNode);
			
			var tr = domConstruct.create("tr", {style:"width:"+this.xDIM+"px; height:"+this.yDIM+"px",}, graphTableDiv);
			
			var td = domConstruct.create("td", {style:"width:90%; height: 100%"}, tr);
			
			var sContainer = domConstruct.create("div", null, td);
			
			var outer = new BorderContainer({ 
			    style:"width:"+this.xDIM+"px; height:"+this.yDIM+"px",
			    design: "sidebar"
		    }).placeAt(sContainer); 

		   
			var center = new ContentPane({ 
			        region:"center", 
			        style: "border: 0px; padding: 0px; top: 0px; left: 0px; overflow: hidden; width: 90%; height: 100%"
		    }).placeAt(outer); 
			
			var lright = new ContentPane({ 
		        region:"right", 
		        style: "border: 0px; padding: 0px; top: 0px; left: 0px; overflow: hidden; float: right;width: 10%; height: 100%;"
			}).placeAt(outer); 
			

			this.strainchart = new Chart({
				srcNodeRef:center.domNode,
				region:"center"
				});
            this.strainchart.chart.addPlot("default", {type: Lines, tension: "S"});
            
            this.strainchart.chart.addAxis("x", {
                title: "Time", 
                titleGap: 0,
                titleOrientation: "away",
                majorTickStep: 1, // setting major tick step to 1 and minorTicks false is to make it work correctly under FF. If not only major ticks rendered of which our end time is not one
                minorTicks: true
            });
            this.strainchart.chart.addAxis("y", {vertical: true, fixLower:"major", includeZero: true});
            
            for(var k in strainData){
            	if(/^[0-9]+$/.test(k)){ //Ignore 14p15 etc
            		this.strainchart.chart.addSeries(k,strainData[k],{stroke: {color: this.plotColours[k],style: this.plotStyles[k]}});
            	}else{
            		var id = k.substring(0,2);
            		this.strainchart.chart.addSeries(k,strainData[k],{stroke: {color: this.plotColours[id],style: this.plotStyles[id]}});
            	}
            }

            
            this.strainchart.chart.addPlot("indicator", { 
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
            this.strainchart.chart.render();
                       
            this.strainchartLegend = new Legend({
            	chart: this.strainchart,
            	style: "padding: 0px; width: 4em;",
            	horizontal: false
            });
            this.strainchartLegend.placeAt(lright.containerNode);
            this.strainchartLegend.refresh();
			outer.startup();
            on(graphTableDiv, "resize", this.strainchart, "resize"); 
            on(graphTableDiv, "resize", this.strainchartLegend, "resize"); 
            outer.resize();
            
			
			//Create Controls
			
			this.speckleActionTable = domConstruct.create("table", {"class":"dijitDialogPaneActionBar"}, this.graphContainer.containerNode);
			tr = domConstruct.create("tr", {"class":""}, this.speckleActionTable);
			var td = domConstruct.create("td", {"class":""}, tr);
			var dselect = domConstruct.create("div", {"class":""}, td);
			tr = domConstruct.create("tr", {"class":""}, this.speckleActionTable);
			td = domConstruct.create("td", {"class":""}, tr);
			var d0 = domConstruct.create("div", {"class":""}, td);
			tr = domConstruct.create("tr", {"class":""}, this.speckleActionTable);
			td = domConstruct.create("td", {"class":""}, tr);
			var fieldset = domConstruct.create("div", {"class":"","style":"border: 1px solid black;"}, td);
			tr = domConstruct.create("tr", {"class":""}, this.speckleActionTable);
			td = domConstruct.create("td", {"class":""}, tr);
			var d1 = domConstruct.create("div", {"class":""}, td);
			tr = domConstruct.create("tr", {"class":""}, this.speckleActionTable);
			td = domConstruct.create("td", {"class":""}, tr);
			var d2 = domConstruct.create("div", {"class":""}, td);
			tr = domConstruct.create("tr", {"class":""}, this.speckleActionTable);
			td = domConstruct.create("td", {"class":""}, tr);
			var d3 = domConstruct.create("div", {"class":""}, td);
			tr = domConstruct.create("tr", {"class":""}, this.speckleActionTable);
			td = domConstruct.create("td", {"class":""}, tr);
			var d4 = domConstruct.create("div", {"class":""}, td);
			tr = domConstruct.create("tr", {"class":""}, this.speckleActionTable);
			td = domConstruct.create("td", {"class":""}, tr);
			var d5 = domConstruct.create("div", {"class":""}, td);
			var d6 = domConstruct.create("div", {"class":""}, this.graphContainer.containerNode);
			domConstruct.create("img", {
        		src:"ABI/app/ICMA/images/bullseye-200.png",
        		style: "display:block; margin-left: auto; margin-right: auto; margin-top: 10px",
        		height:"100",
        		width:"100"
        	}, d6);
			
			
			var numcols = Math.round(numstrains/2);
			this.speckleStrainSelectionTableContainer = new TableContainer({    
                customClass: "labelsAndValues",
            	labelWidth: "10px",
            	cols: numcols,
			});
			
			this.selectedStrainsCbxs = new Object();
			
			for(var key in this.selectedStrains)
			{
				var cbxparent = this;
				var cbx = new CheckBox({
	                label: key,
	                checked: true,
	                value: key,
	                ctxparent: cbxparent,
	                onChange: function(){
	                	var mkey = this.value;
	                	this.ctxparent.selectedStrains[mkey] = ! this.ctxparent.selectedStrains[mkey];
	                	this.ctxparent.refreshGraph();
	                }
	            });
				this.selectedStrainsCbxs[key] = cbx;
				this.speckleStrainSelectionTableContainer.addChild(cbx);
			}
			
			var cellengths = this.clientWidth/4;
			
			this.specklePGLSTableContainer = new TableContainer({    
                customClass: "labelsAndValues",
            	labelWidth: "100px",
            	cols: 2,
            });
            
			var pgls = this.computePGLS(strainData);
            this.specklePGLSTextBox = new TextBox({
                label: "PGLS",
                value: pgls,
                //style: "width: 7em; float: left",
                disabled: true
            });
            this.specklePGLSTextBox.placeAt(this.specklePGLSTableContainer.containerNode);
			//Create a checkbox selection grid for which views to use
            this.speckleSelectionTableContainer = new TableContainer({    
                customClass: "labelsAndValues",
            	labelWidth: "50px",
            	cols: 3
            });
            
            var vctr = 0;
            if(this.aplaxSelected){
	            this.selectAplaxCBX = new CheckBox({
	                label: "APLAX",
	                checked: this.aplaxViewSelected,
	                disabled: !this.aplaxViewSelected,
	                onChange: lang.hitch(this, this.selectAplaxCBXEvt)
	            });
	            this.selectAplaxCBX.placeAt(this.speckleSelectionTableContainer.containerNode);
	            vctr++;
            }
            
            if(this.tchSelected){
	            this.selectTCHCBX = new CheckBox({
	                label: "2CH",
	                checked: this.tchViewSelected,
	                disabled: !this.tchViewSelected,
	                onChange: lang.hitch(this, this.selectTCHCBXEvt)
	            });
	            this.selectTCHCBX.placeAt(this.speckleSelectionTableContainer.containerNode);
	            vctr++;
            }

            if(this.fchSelected){
	            this.selectFCHCBX = new CheckBox({
	                label: "4CH",
	                checked: this.fchViewSelected,
	                disabled: !this.fchViewSelected,
	                onChange: lang.hitch(this, this.selectFCHCBXEvt)
	            });
	            this.selectFCHCBX.placeAt(this.speckleSelectionTableContainer.containerNode);
	            vctr++;
            }
            
            if(this.saxapexSelected){
	            this.selectSaxApexCBX = new CheckBox({
	                label: "SAX@APEX",
	                checked: this.saxapexViewSelected,
	                disabled: !this.saxapexViewSelected,
	                onChange: lang.hitch(this, this.selectSaxApexCBXEvt)
	            });
	            this.selectSaxApexCBX.placeAt(this.speckleSelectionTableContainer.containerNode);
	            vctr++;
            }
            
            if(this.saxmidSelected){
	            this.selectSaxMidCBX = new CheckBox({
	                label: "Sax@Mid",
	                checked: this.saxmidViewSelected,
	                disabled: !this.saxmidViewSelected,
	                onChange: lang.hitch(this, this.selectSaxMidCBXEvt)
	            });
	            this.selectSaxMidCBX.placeAt(this.speckleSelectionTableContainer.containerNode);
	            vctr++;
            }
            
            if(this.saxbaseSelected){
	            this.selectSaxBaseCBX = new CheckBox({
	                label: "Sax@Base",
	                checked: this.saxbaseViewSelected,
	                disabled: !this.saxbaseViewSelected,
	                onChange: lang.hitch(this, this.selectSaxBaseCBXEvt)
	            });
	            this.selectSaxBaseCBX.placeAt(this.speckleSelectionTableContainer.containerNode);
	            vctr++;
            }
            
            
            this.speckleModelTableContainer = new TableContainer({    
                //customClass: "",
            	labelWidth: cellengths+"px",
            	cols: 2,
            	spacing: 0
            });
            
            this.speckleModelTextBox = new TextBox({
                label: "Model Name",
                value: this.myModelTextBox.get("value"),
                style: "width: 17em;",
            });
            this.speckleModelTextBox.placeAt(this.speckleModelTableContainer.containerNode);
            
            
			var FitButton = new Button({
				label: "Fit to Tracking",
				"class": "",
				style: "float:left",
				onClick: lang.hitch(this, this.onFit)
			});
			FitButton.placeAt(d2);
			
			var returnToVideo = new Button({
				label: "Return to View",
				"class": "",
				onClick: lang.hitch(this, function(){
					this.aplaxSelected = false;
					this.tchSelected = false;
					this.fchSelected = false;
					this.saxbaseSelected = false;
					this.saxmidSelected = false;
					this.saxapexSelected = false;
					if(this.playerId)
						clearInterval(this.playerId);
		            this.vbccenter.removeChild(bc);
					domConstruct.destroy(bc);
		            this.fittingPaneBc.removeChild(this.fittingcontainer);
					domConstruct.destroy(this.fittingcontainer);
		            this.bc.domNode.style.display = '';
		            this.vbc.domNode.style.display = 'none';
		            this.resizeVideoView();
				})
			});
			returnToVideo.placeAt(d3);
			
			
			var returnToTracking = new Button({
				label: "Return to Tracking",
				"class": "",
				style: "float:left",
				onClick: lang.hitch(this, function(){
		            this.vbccenter.removeChild(bc);
					domConstruct.destroy(bc);
					//Enable the animation
	                for(var i =0;i<this.activeKJS.length;i++){
	              	   this.activeKJS[i].setFlipRate(this.flipRate);
	                }
		            this.fbc.domNode.style.display = '';
		            this.vbc.domNode.style.display = 'none';
				})
			});
			returnToTracking.placeAt(d3);

			
			//Create a border with title to say selection of strains
			var strainfieldset = domConstruct.create("fieldset",{}, dselect);
			domConstruct.create("legend",{style:"color:black;font-weight:bold;", innerHTML:"Selected Strains"}, strainfieldset);
			
			this.speckleStrainSelectionTableContainer.placeAt(strainfieldset);
			domConstruct.create("hr",{style:"color:black;margin-bottom:10px"}, strainfieldset);
			this.speckleStrainSelectionTableContainer.startup();
			
            this.speckleModelTableContainer.placeAt(d1);
            this.speckleModelTableContainer.startup();
      
            this.specklePGLSTableContainer.placeAt(d0);
            this.specklePGLSTableContainer.startup();
            
            this.speckleSelectionTableContainer.placeAt(fieldset);
            this.speckleSelectionTableContainer.startup();
            
			if(vctr<2){ //Dont show view selection if there is just one view
				fieldset.hidden = true;
			}
			
			var playStopDiv = domConstruct.create("div", {style: "margin-bottom: 5px; float: left"}, d4);
            this.playButton = new Button({
                label: "Play",
                showLabel: false,
                iconClass: "playIcon",
                onClick: lang.hitch( this, function(){ 
                    topic.publish("/" + this.uuid + "/PlayStop");
                })
            });
            this.playButton.placeAt(playStopDiv);
            
			var playSliderDiv = domConstruct.create("div", {"class":"", style : "width:"+this.xDIM+"px;padding-top: 8px;"}, d4);
			
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
            
            var aniDiv = domConstruct.create("div", {style: "margin-bottom: 5px; padding-top: 8px;"}, d5);
            
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
			
			
			
			//Show the pane and hide fbc
            
			//Hide the video and fitting pane
            this.fbc.domNode.style.display = 'none';
            this.vbc.domNode.style.display = '';
            //Call resize to ensure that the contents are displayed appropriately
            this.vbc.resize();

			this.progressIndicator.hide();
			this.progressIndicator = null;
            topic.subscribe("/" + this.uuid + "/Time", lang.hitch(this, function(time){
            		//console.debug("topic for time "+this.time); 
            		this.time = time;
                    this.setSpeckleTime();    
            }));
		},
		
		
		
		renderSpeckleTrackingResult : function(){
			this.trackMotionInProgress = false;
			request.post(this.SXSPECKLETRACK+"?workflowID="+this.POLLHANDLE+"&result=true").then(lang.hitch(this, function(data){
        		//console.debug(dojo.fromJson(data));
				// do something with handled data
				this.SPECKLETRACKINGDATA = dojo.fromJson(data);
				this.speckleTrackingWorkflowID = this.POLLHANDLE;
				this.createSpeckleVerificationPane();
			}), lang.hitch(this, function(err){
            	// handle an error condition
				try{
	            	this.progressIndicator.hide();
	            	//this.progressIndicator.destroyRecursive();
	            	clearInterval(this.POLLTIMERHANDLE);
	    			this.POLLTIMERHANDLE = null;
	            	console.log("Error occured"+err);
	            	//this.TrackButton.set('disabled',false);
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
					if(this.progressIndicator.getValue()<data*0.9)
						this.progressIndicator.setValue(data*0.9);
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
            	//this.TrackButton.set('disabled',false);
	        }));

		}, 
		
		
		onTrackMotion: function(){
			if(!this.trackMotionInProgress){
				this.trackMotionInProgress = true;
				//Construct the speckle motion display table
				var modelname = this.myModelTextBox.get("value");
				if(modelname.length == 0){
					this.myModelTextBox.set("value","TempModel");
				}
				var fitInput = this.collateData();
				if(fitInput!=null){
					//Disable the animation
	                for(var i =0;i<this.activeKJS.length;i++){
	              	   this.activeKJS[i].setFlipRate(0);
	                }
	                //Disable the button
					//this.TrackButton.set('disabled',true);
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
			            	//this.TrackButton.set('disabled',false);
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
		
		onFitClick: function(){
			//Store the diaply width for subsequent planes
			this.clientWidth = this.bc.domNode.clientWidth;
			this.clientHeight = this.bc.domNode.clientHeight;
			this.createFittingPane();
		}
		
	

	});
});
