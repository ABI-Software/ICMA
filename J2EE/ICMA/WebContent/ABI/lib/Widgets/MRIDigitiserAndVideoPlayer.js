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
    "dojo/_base/lang",
	"dojo/_base/declare",
	"dojo/dom-construct",
	"dojo/_base/sniff",
	"dijit/_Contained",
	"dijit/_WidgetBase",
	"dijit/form/Button",
	"dijit/form/HorizontalSlider",
	"dojox/uuid/Uuid",
	"dojo/on",
	"dojox/uuid/generateRandomUuid",
	"dojo/topic",
	"dojo/dom",
	"dojo/dom-style",
], function(lang, declare, domConstruct, has, Contained, 
		     WidgetBase, Button,HorizontalSlider,
		     Uuid, on, generateRandomUuid, topic, dom, domStyle){

	// module:
	//		ABI/lib/Widgets/MRIDigitiserAndVideoPlayer

	return declare("ABI.lib.Widgets.MRIDigitiserAndVideoPlayer", [WidgetBase, Contained],{
		// summary:
		//		A thin wrapper around the HTML5 `<video>` element.
		
		// source: Array
		//		An array of src and type,
		//		ex. [{src:"a.webm",type:"video/mpeg"},{src:"a.ogg",type:"video/ogg"},...]
		//		The src gives the path of the media resource. The type gives the
		//		type of the media resource.
		//      !!!!! does not handle resize events post creation
		
		source: null,

		// width: String
		//		The width of the embed element.
		width: "200px",

		// height: String
		//		The height of the embed element.
		height: "15px",

		// _playable: [private] Boolean
		//		Internal flag.
		_playable: false,
		
		// _tag: [private] String
		//		The name of the tag ("audio").
		_tag: "div",
		
		duration: 0,
		
		rate: 0.05,
		
		startpos: 0.0,	//Starting position of the video
		
		xdim: 200,
		
		ydim: 200,
		
		maxPoints: 3,
		
        edtime: -1,
        
        estime: -1,
		
		rendering: false,
		
		constructor: function(params){
            dojo.mixin(this, params);
            
            this.uuid = new Uuid(dojox.uuid.generateRandomUuid()).toString();
			// summary:
			//		Creates a new instance of the class.
			this.source = [];
		},

		buildRendering: function(){
			this.domNode = this.srcNodeRef || domConstruct.create(this._tag);
            this.parentElement = domConstruct.create("div",{
        		style:"width:inherit;height:inherit",
            	id: this.uuid+"EL",
            },this.domNode);
            
                        
            //Create Kinetic Stage element
            this.videoElement = domConstruct.create("div",{
        		style:"position:absolute;",
            	id: this.uuid+"KSEL",
            },this.parentElement);
            
			this.videoNode = domConstruct.create("video",{
				preload: "auto",
				style: "width:inherit;z-index:-1",
				controls: false,
				loop: true
				},this.parentElement);
			
			this.controlsNode = domConstruct.create("div",{style: "width:inherit;"},this.parentElement);
			
			var playStopDiv = domConstruct.create("div", {style: "margin-bottom: 5px; float: left"}, this.controlsNode);
			
            this.playButton = new Button({
                label: "Play",
                showLabel: false,
                iconClass: "playIcon",
                onClick: lang.hitch( this, function(){ 
                    this.playStop();
                })
            });
            this.playButton.placeAt(playStopDiv);
            
			var playSliderDiv = domConstruct.create("div", {"class":"", style : "width: 100%;padding-top: 8px;"}, this.controlsNode);

			this.time = 0; //video start time 
            this.playSlider = HorizontalSlider({
                value: 0.0,
                minimum: 0.0, 
                maximum: this.duration, //initial video end time, 
                intermediateChanges: true,
                onChange: lang.hitch( this, function(value){
                	this.setTime(value);
                })
            });
            this.playSlider.placeAt(playSliderDiv);
            
			var edesframeselect = domConstruct.create("div", {style: "margin-bottom: 5px;width:100%; float: left"}, this.controlsNode);
            this.resetButton = new Button({
                label: "Reset",
                showLabel: true,
                style: "float:left;font-size:10pt;margin-top:3px;",
                onClick: lang.hitch( this, function(){ 
                    this.reset();
                })
            });
            this.resetButton.placeAt(edesframeselect);
            this.gotoed = new Button({
                label: "ED",
                showLabel: true,
                style: "font-size:10pt;margin-left:10px;",
                disabled: true,
                onClick: lang.hitch( this, function(){ 
                    this.gotoedframe();
                })
            });
            this.gotoed.placeAt(edesframeselect);
            this.gotoes = new Button({
                label: "ES",
                showLabel: true,
                style: "font-size:10pt;margin-left: 170px;",
                disabled: true,
                onClick: lang.hitch( this, function(){ 
                    this.gotoesframe();
                })
            });
            this.gotoes.placeAt(edesframeselect);
            
            this.setCycleEnd = new Button({
                label: "&laquo;",
                showLabel: true,
                style: "float:right;font-size:10pt;margin-top:2px;",
                onClick: lang.hitch( this, function(){ 
                    this.setEndofCycle();
                })
            });
            this.setCycleEnd.placeAt(edesframeselect);
            
            
            
			this.videoNode.addEventListener('loadedmetadata', lang.hitch(this, function() {
				this.duration = this.videoNode.duration*0.99;
			    if(this.playSlider){
			    	this.playSlider.set({maximum: this.duration});
			    	this.setTime(this.startpos);
			    }
			    //console.debug(this.duration);
	            this.ydim = domStyle.get(this.videoNode,"height");
	            this.xdim = domStyle.get(this.videoNode,"width");
	            
	            this.nodeWidth = this.domNode.style.width.substring(0,this.domNode.style.width.length -2);
	            this.nodeHeight = this.domNode.style.height.substring(0,this.domNode.style.height.length -2);
	            //console.debug("At creation video "+this.xdim+"\t"+this.ydim);
	            //console.debug("At creation "+this.nodeWidth+"\t"+this.nodeHeight);
	            this.setupKineticJS();
			}));
            
		},
		
		setupKineticJS: function(){
            //Create the kinetic layers
			if(this.debug)
				console.debug("Element size at creation "+this.xdim+"\t"+this.ydim);
            this.kineticStage = new Kinetic.Stage({
                container: this.videoElement,
                width: this.xdim,
                height: this.ydim
            });
            
            this.layer = new Kinetic.Layer();
            this.imageLayer = new Kinetic.Layer();
            this.imageLayer1 = new Kinetic.Layer();
            this.imageLayer2 = new Kinetic.Layer();
            
            this.anchorPoints = new Array();
            this.orderedAnchorPoints = new Array();
            this.anchorPointsMeta = new Array();
            this.esanchorPoints = new Array();
            this.esorderedAnchorPoints = new Array();
            this.esanchorPointsMeta = new Array();

            //framefactor is available only after video is loaded so set temporary values
        	this.framefactor = this.duration*0.02;
        	this.edlow = 1;
        	this.edhigh = 1;

        	this.eslow = 2;
        	this.eshigh = 2;

            // add the layer to the stage
            this.kineticStage.add(this.imageLayer);
            this.kineticStage.add(this.imageLayer1);
            this.kineticStage.add(this.imageLayer2);
            this.kineticStage.add(this.layer);

            this.kineticStage.on("click",lang.hitch(this, function () {
                var mousePos = this.kineticStage.getPointerPosition();
                if(mousePos.y<20)
                	return;
                var anchorlayer = this.imageLayer1;
                var apts = this.anchorPoints;
                var aptsMeta = this.anchorPointsMeta;
                var allowCreation = false;
                var esframe = false;
                if(this.edtime==-1){
                	this.edtime = this.time;
                	this.edlow = (this.edtime-this.framefactor/2);
                	this.edhigh = (this.edtime+this.framefactor/2);
                	this.gotoed.setAttribute('disabled',false);
                }
            	if((this.estime==-1&& this.time>this.edhigh)||(this.estime>0 && this.time>this.eslow && this.time<this.eshigh)){
            		anchorlayer = this.imageLayer2;
            		apts = this.esanchorPoints;
            		aptsMeta = this.esanchorPointsMeta;
            		this.imageLayer2.show();
            		if(this.esanchorPoints.length==0){//Do not change time if points have been choosen
	            		this.estime = this.time;
	                	this.eslow = (this.estime-this.framefactor/2);
	                	this.eshigh = (this.estime+this.framefactor/2);
	                	this.gotoes.setAttribute('disabled',false);
            		}
            		allowCreation = true;
            		esframe = true;
            	}
            	if(this.time>this.edlow && this.time<this.edhigh){
            		allowCreation = true;
            	}
            	
                if(apts.length < 3 && allowCreation){
                	//Ensure that the click is not close to an existing point
                	for(var pp=0;pp<apts.length;pp++){
                		var x = mousePos.x - apts[pp].getX();
                		var y = mousePos.y - apts[pp].getY();
                		var dist = Math.sqrt(x*x+y*y);
                		if(dist<30){
                			apts[pp].setX(mousePos.x);
                			apts[pp].setY(mousePos.y);
                			anchorlayer.draw();
                			return;
                		}
                	}
                	
	                    var anchor = new Kinetic.Circle({
	                        x: mousePos.x,
	                        y: mousePos.y,
	                        radius: 4,
	                        stroke: "#666",
	                        fill: "#ddd",
	                        strokeWidth: 2,
	                        draggable: true,
	                        ftype: esframe,
	                        uuid: this.uuid
	                    });
	
	                    // add hover
	                    // styling
	                    anchor.on("mouseover", function () {
	                        document.body.style.cursor = "pointer";
	                        this.setStrokeWidth(4);
	                        this.parent.draw();
	                    });
	                    anchor.on("mouseout", function (b) {
	                        document.body.style.cursor = "default";
	                        this.setStrokeWidth(2);
	                        this.parent.draw();
	                    });
	                    anchor.on("dragend", function (b) {
	                   		topic.publish(this.attrs.uuid+"/drawSpline",this.attrs.ftype);
	                        this.parent.draw();
	                    });
	
	                    anchorlayer.add(anchor);
	                    apts.push(anchor);
	                    
	                    if (apts.length == 3) { // Determine
	                        var x1 = apts[0].getX();
	                        var x2 = apts[1].getX();
	                        var x3 = apts[2].getX();
	
	                        var y1 = apts[0].getY();
	                        var y2 = apts[1].getY();
	                        var y3 = apts[2].getY();
	                        
	                        var dy1 = y1 - this.ydim;
	                        var dy2 = y2 - this.ydim;
	                        var dy3 = y3 - this.ydim;
	
	                        var a1dist = Math.sqrt((x1) * (x1) + (dy1) * (dy1));
	                        var a2dist = Math.sqrt((x2) * (x2) + (dy2) * (dy2));
	                        var a3dist = Math.sqrt((x3) * (x3) + (dy3) * (dy3));
	                        if (a1dist < a2dist && a1dist < a3dist) { 
	                            if (x2 < x3) {
	                            	aptsMeta[1] = 0;
	                            	aptsMeta[2] = 2;
	                            } else {
	                                aptsMeta[2] = 0;
	                                aptsMeta[1] = 2;
	                            }
	                            apx = x1;
	                            apy = y1;
	                            aptsMeta[0] = 1;
	                        }
	                        if (a2dist < a1dist && a2dist < a3dist) {
	                            if (x1 < x3) {
	                            	aptsMeta[0] = 0;
	                            	aptsMeta[2] = 2;
	                            } else {
	                            	aptsMeta[2] = 0;
	                            	aptsMeta[0] = 2;
	                            }
	                            apx = x2;
	                            apy = y2;
	                            aptsMeta[1] = 1;
	                        }
	                        if (a3dist < a2dist && a3dist < a1dist) {
	                            if (x2 < x1) {
	                            	aptsMeta[1] = 0;
	                            	aptsMeta[0] = 2;
	                            } else {
	                            	aptsMeta[0] = 0;
	                            	aptsMeta[1] = 2;
	                            }
	                            apx = x3;
	                            apy = y3;
	                            aptsMeta[2] = 1;
	                        }
	
	
	                        var orderedapts = this.orderAnchorPoints(apts,aptsMeta,esframe); // Order them
	                        
	                		var mks = new Array();
/*	                		mks.push({x:orderedapts[0].getX(),y:orderedapts[0].getY()});
	                		mks.push({x:orderedapts[1].getX(),y:orderedapts[1].getY()});
	                		mks.push({x:orderedapts[2].getX(),y:orderedapts[2].getY()});
*/
	                		mks.push(orderedapts[0].getX());mks.push(orderedapts[0].getY());
	                		mks.push(orderedapts[1].getX());mks.push(orderedapts[1].getY());
	                		mks.push(orderedapts[2].getX());mks.push(orderedapts[2].getY());

	                		if(!esframe){
		                        this.nbSpline = new Kinetic.Line({
		                            points: mks,
		                            stroke: 'yellow',
		                            strokeWidth: 5,
		                            lineCap: 'round',
		                            tension: 1.03,
		                            dashArray: [10, 10],
		                            opacity: 0.5
		                          });
		                        anchorlayer.add(this.nbSpline);
		                        this.nbSpline.setZIndex(0); //Since the index search looks up for parent, do this after adding to layer
	                		}else{
		                        this.nbESSpline = new Kinetic.Line({
		                            points: mks,
		                            stroke: 'yellow',
		                            strokeWidth: 5,
		                            lineCap: 'round',
		                            tension: 1.03,
		                            dashArray: [10, 10],
		                            opacity: 0.5
		                          });
		                        anchorlayer.add(this.nbESSpline);
		                        this.nbESSpline.setZIndex(0); //Since the index search looks up for parent, do this after adding to layer               			
	                		}
	                        topic.subscribe(this.uuid+"/drawSpline",lang.hitch(this,this.drawSpline));
                      }
                    anchorlayer.draw();
                }
            }));

//This should ideally work but chrome in windows fails to play video 09 Jun 2014  Version 35.0.1916.114 m           
/*            var image = new Kinetic.Image({
                image: this.videoNode,
                width: this.xdim,
                height: this.ydim
            });
 */
            
            
            var image = new Kinetic.Image({
                image: this.videoNode.play(),
                width: this.xdim,
                height: this.ydim
            });

            this.videoNode.pause();
            this.videoNode.currentTime = this.startpos; 
            
            // add the image to the layer
            this.imageLayer.add(image);

            this.imageLayer.show();
            this.imageLayer.draw();		
		},

		setTime: function(time){
			if(!this.rendering){
				this.rendering = true;
				try{
					this.time = time;
					this.playSlider.set("value",this.time,false);
					this.videoNode.currentTime = this.time;
					if(this.edtime>-1){
			        	if(this.time>this.edlow && this.time<this.edhigh){
			        		this.imageLayer1.show();
			        	}else{
			        		this.imageLayer1.hide();
			        	}
					}
					if(this.estime>-1){
			        	if(this.time<this.eshigh&&this.time>this.eslow){
			        		this.imageLayer2.show();
			        	}else{
			        		this.imageLayer2.hide();
			        	}
					}
				}catch(e){
					
				}
				this.rendering = false;
			}
		},
		
		gotoesframe: function(){
			this.playSlider.set("value", this.estime);
		},
		
		gotoedframe: function(){
			this.playSlider.set("value", this.edtime);
		},
		
		setEndofCycle: function(){
			if(this.time>this.estime && this.time>this.edtime)
				this.endofCycle = this.time;
			else
				alert("Current choices for end systole/diastole are inconsistent with the chosen end of cardiac-cycle time, reset and procced if you wish to set current time point as end of the cycle");
		},
		
		reset: function(){
            for(var i=0;i<this.anchorPoints.length;i++){
            	this.anchorPoints[i].destroy();
            }
            this.anchorPoints = new Array();
            this.anchorPointsMeta = new Array();
            this.orderedAnchorPoints = new Array();
            if(this.nbSpline)
            	this.nbSpline.destroy();
            for(var i=0;i<this.esanchorPoints.length;i++){
            	this.esanchorPoints[i].destroy();
            }
            this.esanchorPoints = new Array();
            this.esanchorPointsMeta = new Array();
            this.esorderedAnchorPoints = new Array();
            if(this.nbESSpline)
            	this.nbESSpline.destroy();
            this.imageLayer1.removeChildren();
            this.imageLayer2.removeChildren();
            this.imageLayer1.draw();
            this.imageLayer2.draw();
            this.gotoed.setAttribute('disabled',true);
            this.gotoes.setAttribute('disabled',true);
            if(this.edtime>-1){
            	this.setTime(this.edtime);
            }
			this.edtime = -1;
			this.estime = -1;
		},
		
		getNormalisedEDTime: function(){
			return this.edtime/this.duration/0.99;
		},

		getNormalisedESTime: function(){
			return this.estime/this.duration/0.99;
		},

		getNormalisedEndofCycleTime: function(){
			if(this.endofCycle){
				return this.endofCycle/this.duration/0.99;
			}else{
				return 1.0;
			}
		},
		
        getOrderedCoordinates: function (txdim, tydim) {
            if (this.anchorPoints.length < 2) {
                return;
            }
            var xratio = this.xdim / txdim;
            var yratio = this.ydim / tydim;

            var coords = new Array();
            if(this.orderedAnchorPoints.length>2){ //Ordering is available for long axis 
	            // console.debug(anchorPointsMeta);
	            for (var i = 0; i < this.maxPoints; i++) {
	                coords.push([
	                    this.orderedAnchorPoints[i].getX() / xratio,
	                    this.orderedAnchorPoints[i].getY() / yratio, 0.0
	                ]);
	            }
            }else{
            	for (var i = 0; i < this.anchorPoints.length; i++) {
	                coords.push([
	                    this.anchorPoints[i].getX() / xratio,
	                    this.anchorPoints[i].getY() / yratio, 0.0
	                ]);
	            }
            }
            return coords;
        },
        
        getOrderedESCoordinates: function (txdim, tydim) {
            if (this.esanchorPoints.length < 2) {
                return;
            }
            var xratio = this.xdim / txdim;
            var yratio = this.ydim / tydim;

            var coords = new Array();
            if(this.esanchorPoints.length==3){
	            for (var i = 0; i < this.esorderedAnchorPoints.length; i++) {
	                coords.push([
	                    this.esorderedAnchorPoints[i].getX() / xratio,
	                    this.esorderedAnchorPoints[i].getY() / yratio, 0.0
	                ]);
	            }
            }else{
                var x1 = this.esanchorPoints[0].getX();
                var x2 = this.esanchorPoints[1].getX();

                var y1 = this.esanchorPoints[0].getY();
                var y2 = this.esanchorPoints[1].getY();
                //Distance from origin(top left)
                var a1dist = Math.sqrt((x1) * (x1) + (y1) * (y1));
                var a2dist = Math.sqrt((x2) * (x2) + (y2) * (y2));
                if(a1dist<a2dist){
                	coords.push([
         	                    this.esanchorPoints[0].getX() / xratio,
         	                    this.esanchorPoints[0].getY() / yratio, 0.0
         	                ]);
                	coords.push([
          	                    this.esanchorPoints[1].getX() / xratio,
          	                    this.esanchorPoints[1].getY() / yratio, 0.0
          	                ]);
                }else{
                	coords.push([
          	                    this.esanchorPoints[1].getX() / xratio,
          	                    this.esanchorPoints[1].getY() / yratio, 0.0
          	                ]);
                	coords.push([
          	                    this.esanchorPoints[0].getX() / xratio,
          	                    this.esanchorPoints[0].getY() / yratio, 0.0
          	                ]);
                }
            }
            return coords;
        },
        

        orderAnchorPoints: function (apts,aptsMeta,esframe) {
        	var opts = this.orderedAnchorPoints;
        	if(esframe)
        		opts = this.esorderedAnchorPoints;
        	var len = apts.length;
            for (var i = 0; i < len; i++) {
                for (var j = 0; j < len; j++) {
                    if (aptsMeta[j] == i) {
                        opts.push(apts[j]);
                        break;
                    }
                }
            }
            return opts;
        },

        
        drawSpline: function (e) {
        	var mks = new Array();
        	if(!e){
	    		mks.push({x:this.orderedAnchorPoints[0].getX(),y:this.orderedAnchorPoints[0].getY()});
	    		mks.push({x:this.orderedAnchorPoints[1].getX(),y:this.orderedAnchorPoints[1].getY()});
	    		mks.push({x:this.orderedAnchorPoints[2].getX(),y:this.orderedAnchorPoints[2].getY()});
	    		this.nbSpline.destroy();
	            this.nbSpline = new Kinetic.Line({
	                points: mks,
	                stroke: 'yellow',
	                strokeWidth: 5,
	                lineCap: 'round',
	                tension: 1.03,
	                dashArray: [10, 10],
	                opacity: 0.5
	              });
	            this.imageLayer1.add(this.nbSpline);
	            this.nbSpline.setZIndex(0);
        	}else{
	    		mks.push({x:this.esorderedAnchorPoints[0].getX(),y:this.esorderedAnchorPoints[0].getY()});
	    		mks.push({x:this.esorderedAnchorPoints[1].getX(),y:this.esorderedAnchorPoints[1].getY()});
	    		mks.push({x:this.esorderedAnchorPoints[2].getX(),y:this.esorderedAnchorPoints[2].getY()});
	    		this.nbESSpline.destroy();
	            this.nbESSpline = new Kinetic.Line({
	                points: mks,
	                stroke: 'yellow',
	                strokeWidth: 5,
	                lineCap: 'round',
	                tension: 1.03,
	                dashArray: [10, 10],
	                opacity: 0.5
	              });
	            this.imageLayer2.add(this.nbESSpline);
	            this.nbESSpline.setZIndex(0);        		
        	}
        },

		
        resize: function(event){
/*        	if(event.w!=this.curWidth||event.h!=this.curHeight){
            	this.curWidth = event.w;
            	this.curHeight = event.h;
        	}*/
        },
		
		stop: function(){
			if(this.playing!=null){
	        	//console.log("Stop");
	            this.playButton.set("iconClass", "playIcon");
	            this.playButton.set("label", "Play");
	            clearInterval(this.playing);
	            this.playing = null;
			}
		},
		
		playStop : function(){
			if(!this.playing){
				//console.log("Play");
                this.playButton.set("iconClass", "pauseIcon");
                this.playButton.set("label", "Pause");
	            this.playing = setInterval(lang.hitch(this, function(){
	                this.time = this.time + this.rate;
	                if(this.time > this.duration){ 
	                    this.time = 0;  
	                }
	                //this.playSlider.set("value",this.time,false);
	                if(!this.rendering){
	                	this.setTime(this.time);
	                }
	            }), 50); // 50ms for attempted 20fps  
            }else{
            	this.stop();
            }     
		},
		
		_getEmbedRegExp: function(){
			return has('ff') ? /video\/mp4/i :
				   has.isIE >= 9 ? /video\/webm/i :
				   //has("safari") ? /video\/webm/i : //Google is gooing to provide webm plugin for safari
				   null;
		},

		startup: function(){
			if(this._started){ return; }
			this.inherited(arguments);
		 	if(this.videoNode.canPlayType){
				if(this.source.length > 0){
					for(var i = 0, len = this.source.length; i < len; i++){
						domConstruct.create("source", {src:this.source[i].src, type:this.source[i].type}, this.videoNode);
						this._playable = this._playable || !!this.videoNode.canPlayType(this.source[i].type);
					}
				}else{
					for(var i = 0, len = this.domNode.childNodes.length; i < len; i++){
						var n = this.domNode.childNodes[i];
						if(n.nodeType === 1 && n.nodeName === "SOURCE"){
							this.source.push({src:n.src, type:n.type});
							this._playable = this._playable || !!this.videoNode.canPlayType(n.type);
						}
					}
				}
			}
			has.add("mobile-embed-audio-video-support", true);	//It should move to staticHasFeatures
		 	if(has("mobile-embed-audio-video-support")){
				if(!this._playable){
					for(var i = 0, len = this.source.length, re = this._getEmbedRegExp(); i < len; i++){
					 	if(this.source[i].type.match(re)){
							var node = domConstruct.create("embed", {
								src: this.source[0].src,
								type: this.source[0].type,
								width: this.width,
								height: this.height
							});
							this.domNode.parentNode.replaceChild(node, this.videoNode);
							this.videoNode = node;
							this._playable = true;
							break;
						}
					}
				}
			}
		 	if(this._playable){
		 		this.videoNode.load();
		 	}else{//do not display controls bar
		 		
		 	}
		}
		
	});
	   
});
