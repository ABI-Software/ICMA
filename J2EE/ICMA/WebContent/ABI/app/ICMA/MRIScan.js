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
	"dijit/form/Button",
	"dojox/string/sprintf",
	"dojox/mobile/SwapView",
	"dojox/mobile/TextBox",
	"dijit/form/CheckBox",
	"dojox/mobile/Accordion",
	"dojox/mobile/RoundRectList",
	"dojox/mobile/ListItem",
	"dojox/mobile/ContentPane",
	//"ABI/lib/Widgets/VideoPlayer",
	"ABI/lib/Widgets/MRIDigitiserAndVideoPlayer",
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
	"dojox/dialog/AlertDialog",
	"dojox/uuid/Uuid",
	"dojox/uuid/generateRandomUuid",
	"dijit/form/HorizontalSlider",
	"ABI/lib/Widgets/ProgressIndicator",
	//"ABI/app/ICMA/MRIZincDigitiserWidget"
    "ABI/app/ICMA/MRIZincWidget"
], function(declare, array, event, lang, domGeom, request, win, has,
		aspect, locale, dom, domClass, domConstruct, domStyle, domAttr,
		topic, on, query, dojoWindow, Carousel, CarouselItem, Button, sprintf, SwapView, 
		TextBox, CheckBox, Accordion, RoundRectList, ListItem, MContentPane,
		Video, TableContainer, Dialog, TitlePane, 
		DijitTextBox, BorderContainer, ContentPane, TabContainer, Contained, Container,
		WidgetBase, AlertDialog, Uuid, generateRandomUuid,
		HorizontalSlider,ProgressBar,ZincWidget
	){

	// module:
	//		ABI/app/ICMA/Scan

	return declare("ABI.app.ICMA.MRIScan", [ContentPane],{
		// summary:
		//		A widget that presents a mri scan
		// description:
		//		
	   server: null,
	   image_elem: "ABI/app/ICMA/Imagetexture_block.exregion",
	   MRIFIT: "",
       POLLINTERVAL: 10000,
       POLLTIMERHANDLE: null,
	   POLLHANDLE: null,
	   uuid: "",
	   anirate: 1,
	   rate: 0.05, // player time increment
	   patientID: null,
	   studyID: null,
	   clientWidth : 0,
	   viewready: false,
	   visibleRegions: null,
	   videoPlayers: null,
	   
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
				//Show the screen
				topic.publish("ABI/lib/Widgets/ready");
			}
		},
       
		
		constructor: function(params){
			dojo.mixin(this, params);
			this.videoPlayers = new Object();
			this.uuid = new Uuid(dojox.uuid.generateRandomUuid()).toString();
			this.patientID = ABI.app.ICMA.PatientManagerSingleton.getCurrentPatient();
			this.studyID = this.study.studyUid;
            this.regionLoaded = new Object();
            this.visibleRegions = new Object();
            topic.subscribe("/" + this.uuid + "/Time", lang.hitch( this, this.setTime));
			topic.subscribe("/" + this.uuid + "/PlayStop", lang.hitch( this, this.playStop));
		},
		
		startup: function(){
			if(this._started){ return; }
			this.inherited(arguments);
			if(this.playerId)
            	clearInterval(this.playerId);
			this.viewready = false;
			//topic.subscribe("/" + this.uuid + "/destroy", lang.hitch( this, this.destroy));
			topic.subscribe("/loadFirstMRIView",lang.hitch( this, this.loadFirstView));
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
	        	           
	        this.addLeftRightPanes();
	        
	        this.addLeftPaneLayout();
	        
	        this.addStudyDetails();
	        
	        this.addViewsCarousel();
	        
	        this.addModelView();
		},
		
		postCreate: function(){
			this.inherited(arguments);
		},
		
		onShow: function(){
			//This causes freezing
			//this.addZincPlugins();
			setTimeout(lang.hitch(this,this.addZincPlugins), 100);
		},
		
		
		addLeftRightPanes: function(){			
	        this.center = new ContentPane({
	        	 region: "center",
	        	 style: "height:100%; width:70%"
	             //content: "some content center"
	        });
	        this.bc.addChild(this.center);
	        
	        this.right = new ContentPane({
	        	 region: "right",
	        	 style: "height:100%; width:30%"
	        	 //splitter: true
	             //content: "some content right"
	        });
	        this.bc.addChild(this.right);
	        
		},
		
		addLeftPaneLayout: function(){
			this.leftPaneBc = new BorderContainer({
	            style: "height: 100%; width: 100%;overflow: auto; min-height: 600px; min-width: 800px"
	        });
	        
			this.center.addChild(this.leftPaneBc);
	        this.leftPaneBc.startup();
	        

	        this.leftPaneBcLeft = new ContentPane({
	        	 region: "left",
	        	 style: "width: 30%",
	        	 splitter: true
	             //content: "some content left"
	        	 //style: "width: 500px"
	        });
	        this.leftPaneBc.addChild(this.leftPaneBcLeft);
	        
	        this.leftPaneBcCenter = new ContentPane({
	        	 region: "center",
	        	 style: "height: 100%; width: 70%"
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
		
		
		addStudyDetails: function(){

			this.detailsTableContainer = new TableContainer({ });

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
			
			this.videoDetailsTableContainer = new TableContainer({ });
			this.videoDetailsTableContainer.placeAt(this.leftPaneBcLeft);
			this.videoDetailsTableContainer.startup();
			
	        //Setup view properties

		},
		
		addViewProperties: function(){
			if(this.videoFieldset){
				domConstruct.destroy(this.videoFieldset);
			}
			
			//this.videoFieldset = domConstruct.create("div", {"class":"viewPropertiesFieldset"}, this.leftPaneBcLeft.domNode);
			this.videoFieldset = domConstruct.create("div", {"class":"viewPropertiesFieldset"}, this.videoDetailsTableContainer.containerNode);
			
			this.propertiesTableContainer = new TableContainer({		
				//customClass: "viewVideo",
				labelWidth: "150px"
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
/*			var propertyTextBox = new DijitTextBox({
				label: "Plane ",
				value: this.view.planeid,
				disabled: true
			});
			propertyTextBox.placeAt(this.propertiesTableContainer.containerNode);*/
			this.propertiesTableContainer.placeAt(this.videoFieldset);
			this.propertiesTableContainer.startup();
		},

		addViewVideo: function(){
			document.body.style.cursor = 'wait';
			//Check if the videoPlayer for the plane already exists
			var nextVideo = this.videoPlayers[this.view.planeid];
			if(this.video){
				this.video.stop();
				domConstruct.empty(this.leftPaneBcCenter.domNode);
			}

			if(nextVideo){
				nextVideo.placeAt(this.leftPaneBcCenter.domNode);
			}else{
				nextVideo = new Video({
			          source: [{src: this.server + this.view.video.mp4, type:"video/mp4"},
			                   {src: this.server + this.view.video.ogg, type:"video/ogg"},
			                   {src: this.server + this.view.video.webm, type:"video/webm"}],
					//style: "height: 100%",
			        "class": "viewVideo",
					controls: true,
					loop: true,
					startpos: 0,
					rate: 0.01,
					viewObject: this.view
			    });
				nextVideo.placeAt(this.leftPaneBcCenter.domNode);
				nextVideo.startup();
				this.videoPlayers[this.view.planeid] = nextVideo;
			}
			this.video = nextVideo;
			//this.video.placeAt(this.leftPaneBcCenter.domNode);

		    //this.video.startup();
			document.body.style.cursor = 'default';
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
					var view = this.study.views[entry];
					if(view.type!=undefined){
						if(view.type=="MR"){
							if((j > (i-1)*this.viewsCarousel.numVisible) && (j <= i*this.viewsCarousel.numVisible)){

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
				}
			}
			
			this.viewsCarousel.placeAt(this.leftPaneBcBottom);
			this.viewsCarousel.startup();

			topic.subscribe("/dojox/mobile/carouselSelect", lang.hitch(this, this.onViewClick));
		},
		
		
		
		onViewClick: function(carousel, itemWidget, itemObject, index){
			//All the necessary data is in itemWidget, Code modified by Jagir
			var studyviews = itemWidget.params.studyObj.views[itemWidget.params.value];
			//Only if view and plane has changed
			if(this.view!=undefined && this.view.planeid==studyviews.planeid){
				return;
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
		
		addModelView: function(){
			this.modelPaneBc = new BorderContainer({
	            style: "height: 100%; width: 100%;"
	        });
	        
			this.right.addChild(this.modelPaneBc);
	        this.modelPaneBc.startup();
            this.modelPaneBc.resize();

	        this.modelPaneBcRight = new ContentPane({
	        	 region: "right",
	        	 style: "width:10%",
	        	 splitter: true
	             //content: "some content left"
	        	 //style: "width: 500px"
	        });
	        this.modelPaneBc.addChild(this.modelPaneBcRight);
	        
	        this.modelPaneBcCenter = new ContentPane({
	        	 region: "center",
	        	 style: "height: 100%;width: 90%"
	        });
	        this.modelPaneBc.addChild(this.modelPaneBcCenter);
	        this.modelDiv = domConstruct.create("div", {"class":"", style: "width: 100%; height: 100%;"}, this.modelPaneBcCenter.containerNode);

	        this.modelPaneBcBottom = new ContentPane({
	        	 region: "bottom",
	        	 splitter: true
	             //content: "some content bottom"
	        });
	        this.modelPaneBc.addChild(this.modelPaneBcBottom);
       
	        //Create check boxes
	        this.planesTableContainer = new TableContainer({ });
			var ctr = 0;
			for(entry in this.study.views){
				var view = this.study.views[entry];
				if(view.type!=undefined){
					if(view.type=="MR"){
						var planename = "O"+ctr;
						var regionName = "PLANE"+ctr;
						ctr++;
						var checkBox = new CheckBox({
							label: planename,
							checked: true,
							onChange: lang.hitch(this, this.planetoggled,regionName)
						});
						checkBox.placeAt(this.planesTableContainer.containerNode);
						this.visibleRegions[regionName] = true;
					}
				}
			}
			this.planesTableContainer.placeAt(this.modelPaneBcRight);
			this.planesTableContainer.startup();
			//Create the player
	        this.playerDiv = domConstruct.create("div", {"class":""}, this.modelPaneBcBottom.containerNode);
	        this.addPlayer();
	        this.modelFittingDiv = domConstruct.create("div", {"class":""}, this.modelPaneBcBottom.containerNode);
	        this.addModelFittingControls();
		},
		
		planetoggled: function(regionName) {
			if(this.visibleRegions[regionName]) {
				this.zincwidgetModel.clear(regionName);
				this.visibleRegions[regionName]=false;
			}else{
				this.visibleRegions[regionName] = true;
				this.zincwidgetModel.showRegion(regionName);
			}
		},
		
		
        image_loaded: function(zincWidget) {
        	this.planecounter--;
	        if (this.planecounter<1) {
	        	zincWidget.createHeartMaterials();
	            zincWidget.showAllRegions();
	            zincWidget.viewAll();
	            //zincWidget.setupInteractionTool();
	            this.modelPaneBc.resize();
	        } 
        },
		
		
		loadModel: function(widget){
			var zincWidget = widget;
			this.planecounter = 0;
			for(entry in this.study.views){
				var view = this.study.views[entry];
				if(view.type!=undefined){
					if(view.type=="MR"){
						var planename = "PLANE"+this.planecounter;
						this.planecounter++;
						//Get the frame urls
						var numframes = parseInt(view.frames.NUMFRAMES);
						
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
						zincWidget.addRegion(planename);
						zincWidget.loadImages(planename, imagefiles, this.image_elem, "JPG", planecoordinates, lang.hitch(this, this.image_loaded));
					}
				}
			}
			
		},
		
        reattachCallbackModel: function(){
        	//console.log("Model.reattachCallbackModel: Not implemented. This is a modelling function call to be completed by Jagir.");
        	// eg use this.zincwidgetModel to re setup desired view
        	//this.zincwidgetModel.filterScene();
        	this.zincwidgetModel.showAllRegions();
        	this.zincwidgetModel.viewAll();
        	//console.debug(this.zincwidgetModel);
        },
		
		resizeVideoView: function(){
			try{
				/*this.clientWidth = this.bc.domNode.clientWidth;
				
				var brect = domGeom.position(this.leftPaneBcCenter.domNode);
				//var vrect = domGeom.position(this.video.domNode);
				var btrect = domGeom.position(this.leftPaneBcBottom.domNode);
				//var ratio = vrect.w/vrect.h;
				var ratio = 1.25;
				var nvh = brect.h - btrect.h*2/3 - 10;
				var nvw = nvh*ratio;
				//Fix the size as digitiser does not handle resize events
				nvh = 350; nvw = 350;
				var margin = (brect.w - nvw - 10)/2;
				if(nvw<200 && brect.w < 200){
					nvw = 200;
					margin = 0;
				}
				if(nvh<150)
					nvh = 150;*/
				var brect = domGeom.position(this.leftPaneBcCenter.domNode);
				var nvh = 350; var nvw = 350;
				var margin = (brect.w - nvw - 10)/2;
				this.video.domNode.style.width = nvw + "px";
				this.video.domNode.style.height = nvh + "px";
				//brect = domGeom.position(this.video.domNode);
				this.video.domNode.style.marginLeft = margin + "px";
			}catch(e){
				
			}
		},
		
        addZincPlugins: function(){
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
			    })); 
			this.pluginsNeedCreated = false;
		},
		
		setTime: function(value) {
			this.modelTime = value; //Used by onLandMarkCheck
        	if(this.debug)
        		console.log("time: " + value);
	        try{
	        	this.zincwidgetModel.setTime(value);
	        }catch(e){
	        	this.playStop();
	        }
        },

		addPlayer: function(){
			var playStopDiv = domConstruct.create("div", {style: "margin-bottom: 5px; float: left"}, this.playerDiv);
            this.playButton = new Button({
                label: "Play",
                showLabel: false,
                iconClass: "playIcon",
                onClick: lang.hitch( this, function(){ 
                	console.debug("/" + this.uuid + "/PlayStop");
                    topic.publish("/" + this.uuid + "/PlayStop");
                })
            });
            this.playButton.placeAt(playStopDiv);
            
			var playSliderDiv = domConstruct.create("div", {"class":"", style : "width: 100%;padding-top: 8px;"}, this.playerDiv);

			this.time = 0; //!!!this.model.startTime; 
            var playSlider = HorizontalSlider({
                value: 0,
                minimum: 0.0, 
                maximum: 1.0, //!!!this.model.endTime, 
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
		},
		
		playStop: function(){
			if(!this.playerId){
            	if(this.debug)
            		console.log("Play");
                this.playButton.set("iconClass", "pauseIcon");
                this.playButton.set("label", "Pause");
	            this.playerId = setInterval(lang.hitch(this, function(){
	                this.time = this.time + this.rate;
	                if(this.time > 1.0){ //!! this.model.endTime
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
        
        addModelFittingControls: function(){
        		this.myModelTableContainer = new TableContainer({    
                    //customClass: "",
                	//labelWidth: cellengths+"px",
        			style: "float:left;margin-top:10px",
                	cols: 2,
                	spacing: 0
                });
                
                this.myModelTextBox = new TextBox({
                    label: "Model Name",
                    value: "",
                    style: "width: 15em; margin-left:10px",
                });
                this.myModelTextBox.placeAt(this.myModelTableContainer.containerNode);
                this.myModelFit = new Button({
                	label: "Fit Model",
                	style: "float:right;margin-top:5px",
                	onClick: lang.hitch(this,this.fitTolandMarks)
                });
                //this.myModelFit.placeAt(this.myModelTableContainer.containerNode);
                this.myModelTableContainer.placeAt(this.modelFittingDiv);
                this.myModelTableContainer.startup();
                this.myModelFit.placeAt(this.modelFittingDiv);
        },
		
        fitTolandMarks: function(){
			if(!this.fitRequestInProgress){
				this.fitRequestInProgress = true;
				this.landMarkData = new Object();
				
				this.landMarkData.modelname = this.myModelTextBox.get("value");
				if(this.landMarkData.modelname.length == 0){
				 var ald = new AlertDialog({message:"Specify a model name",iconClass:"Error"});
				 ald.show();
				 return;
				}
				this.landMarkData.planes = new Array();

				var longaxislm = false;
				var shortaxislm = false;
				var loadedPlanes = new Object();
				for(k in this.videoPlayers){ //Note that some views may not be added by this loop if the video object has not been created
					var video = this.videoPlayers[k];
					//Get the dimensions from frame information
	            	/*var blc = video.viewObject.frames.BLC.split(",");
	            	var x1 = parseFloat(blc[0]);
	                var y1 = parseFloat(blc[1]);
	                var z1 = parseFloat(blc[2]);
	                
	            	var brc = video.viewObject.frames.BRC.split(",");
	            	var x2 = parseFloat(brc[0]);
	                var y2 = parseFloat(brc[1]);
	                var z2 = parseFloat(brc[2]);
	
	            	var tlc = video.viewObject.frames.TLC.split(",");
	            	var x3 = parseFloat(tlc[0]);
	                var y3 = parseFloat(tlc[1]);
	                var z3 = parseFloat(tlc[2]);
	
	                var xdim = Math.sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2));
	                var ydim = Math.sqrt((x1-x3)*(x1-x3) + (y1-y3)*(y1-y3) + (z1-z3)*(z1-z3));*/
					var xdim = parseFloat(video.viewObject.frames.WIDTH);
					var ydim = parseFloat(video.viewObject.frames.HEIGHT);
	                loadedPlanes[video.viewObject.planeid] = true;
					var edlm = video.getOrderedCoordinates(xdim,ydim);
					if(edlm){
						var eslm = video.getOrderedESCoordinates(xdim,ydim);
						if(eslm){
							if(edlm.length==3){
								var edtime = video.getNormalisedEDTime();
								var estime = video.getNormalisedESTime();
								var endcycle = video.getNormalisedEndofCycleTime();
								var fitObject = new Object();
								fitObject["edtime"] = edtime*1.0;
								fitObject["estime"] = estime*1.0;
								fitObject["endcycle"] = endcycle*1.0;
								fitObject["server"] = this.server;
								fitObject["frames"] = video.viewObject.frames;
								fitObject["seriesid"] = video.viewObject.seriesid;
								fitObject["EDLM"] = edlm;
								fitObject["ESLM"] = eslm;
								fitObject["type"] = "long";
								fitObject["planeid"] = video.viewObject.planeid; 
								this.landMarkData.planes.push(fitObject);
								longaxislm = true;
							}else if(edlm.length==2){
								var edtime = video.getNormalisedEDTime();
								var endcycle = video.getNormalisedEndofCycleTime();
								var fitObject = new Object();
								fitObject["edtime"] = edtime*1.0;
								fitObject["endcycle"] = endcycle*1.0;
								fitObject["server"] = this.server;
								fitObject["frames"] = video.viewObject.frames;
								fitObject["seriesid"] = video.viewObject.seriesid;
								fitObject["EDLM"] = edlm;
								fitObject["ESLM"] = eslm;
								fitObject["type"] = "short";
								fitObject["planeid"] = video.viewObject.planeid; 
								this.landMarkData.planes.push(fitObject);
								shortaxislm = true;
							}
						}else if(edlm.length==2){//Short Axis
							var edtime = video.getNormalisedEDTime();
							var endcycle = video.getNormalisedEndofCycleTime();
							var fitObject = new Object();
							fitObject["edtime"] = edtime*1.0;
							fitObject["endcycle"] = endcycle*1.0;
							fitObject["server"] = this.server;
							fitObject["frames"] = video.viewObject.frames;
							fitObject["seriesid"] = video.viewObject.seriesid;
							fitObject["EDLM"] = edlm;
							fitObject["ESLM"] = eslm;
							fitObject["type"] = "short";
							fitObject["planeid"] = video.viewObject.planeid; 
							this.landMarkData.planes.push(fitObject);						
							shortaxislm = true;
						}
					}else{//Just add the plane and frame information
						var fitObject = new Object();
						fitObject["server"] = this.server;
						fitObject["frames"] = video.viewObject.frames;
						fitObject["seriesid"] = video.viewObject.seriesid;
						fitObject["planeid"] = video.viewObject.planeid; 
						this.landMarkData.planes.push(fitObject);
					}
				}
				for(entry in this.study.views){
					var view = this.study.views[entry];
					if(!loadedPlanes[view.planeid]){
						var fitObject = new Object();
						fitObject["server"] = this.server;
						fitObject["frames"] = view.frames;
						fitObject["seriesid"] = view.seriesid;
						fitObject["planeid"] = view.planeid; 
						this.landMarkData.planes.push(fitObject);
					}
				}
				this.landMarkData["studyUID"] = this.study.studyUID;
				this.landMarkData["patientID"] = ABI.app.ICMA.PatientManagerSingleton.currentPatient.patientid;
/*				console.debug(this.study);
				console.debug(dojo.toJson(this.landMarkData));
				this.fitRequestInProgress = false;
				return;*/
				if(longaxislm){
					if(!shortaxislm){
						shortaxislm = confirm("No/incomplete short axis landmarks provided. MRI model may not be accurate.\nProceed with fitting?");
					}
					
					if(shortaxislm){
			          	request.post(this.MRIFIT, {
			        		data : dojo.toJson(this.landMarkData),
			        	}).then(lang.hitch(this, function(data){
			        		this.fitRequestInProgress = false;
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
			          	this.fitRequestInProgress = false;
					}
				}else{//No long axis landmarks have been choosen, cannot continue
					ABI.lib.Widgets.AlertSingleton.alert("Fit MRI Model", "All landmarks for at least one long axis view must be provided");
				}
			}
        },
        
		pollForFittingStatus: function(){
			request.post(this.MRIFIT+"?workflowID="+this.POLLHANDLE).then(lang.hitch(this, function(data){
	            	// do something with handled data

	        		var progress = dojo.fromJson(data)*1.0;
	        		
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
        
        
		renderFittingResult : function(){
			request.post(this.MRIFIT+"?workflowID="+this.POLLHANDLE+"&result=true").then(lang.hitch(this, function(data){

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

	});
});
