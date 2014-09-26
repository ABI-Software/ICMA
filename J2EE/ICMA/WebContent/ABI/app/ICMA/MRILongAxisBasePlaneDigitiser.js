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
define(
["dojo/_base/declare",
 "dojo/_base/lang",
 "dojo/topic",
 "dojo/dom-construct",
 "dojo/dom",
 "dojox/uuid/Uuid",
 "dojo/on",
 "dojox/uuid/generateRandomUuid",
 "dijit/form/HorizontalSlider"], 
function (declare, lang, topic, domConstruct, dom, Uuid, on, generateRandomUuid, HorizontalSlider) {

    // module:
    // ABI/app/ICMA/MRILongAxisBasePlaneDigitiser

    return declare(
        "ABI.app.ICMA.MRILongAxisBasePlaneDigitiser",
        null, {
        // summary:
        // A widget that presents an Long axis digitiser for MRI with base plane
        // description:
        // Only handles Long axis views		

        element: null,
        img: null,
        xdim: null,
        ydim: null,
        type: null,
        edtime: 0,
        estime: 0,
        
        constructor: function (params) {
            dojo.mixin(this, params);

            this.uuid = new Uuid(dojox.uuid.generateRandomUuid()).toString();
            
            var parentElementYDim = this.ydim+20;
            this.maxPoints = 3;
            this.edtime = this.frame;
            
            this.parentElement = domConstruct.create("div",{
        		style:"width:"+this.xdim+"; height:"+parentElementYDim+"position:relative;",
            	id: this.uuid+"EL",
            },dom.byId(this.element));
	
            
            //Create Kinetic Stage element
            this.videoElement = domConstruct.create("div",{
        		style:"position:absolute;",
            	id: this.uuid+"KSEL",
            },dom.byId(this.parentElement));
            
            //Create Video element
            var videoLoc = this.img.substring(0,this.img.lastIndexOf("/")+1)+"us";
           
            var vidObj = '<video id="'+ this.uuid+"KSVL"+'" width="' + this.xdim + '" height="' + this.ydim + ' preload="auto">';
            	vidObj += '<source src="' + videoLoc+ '.webm" type="video/webm"></source>';
            	vidObj += '<source src="' + videoLoc+ '.mp4" type="video/mp4"></source>';
            	vidObj += '<source src="' + videoLoc+ '.ogv" type="video/ogg"></source>';
                vidObj += '</video>';
            
            this.videoDiv =	domConstruct.create("div",{
            		style:"z-index:-1;",
                	innerHTML: vidObj
            },dom.byId(this.parentElement));
			
            this.videoCanvas = document.getElementById(this.uuid+"KSVL"); 
            
            this.videoCanvas.load();

            //Get the duration
			this.videoCanvas.addEventListener('loadedmetadata', lang.hitch(this, function() {
				this.framefactor = this.videoCanvas.duration/this.bpm;
				this.duration = this.frame + 20*this.framefactor;
				this.edextent = (this.edtime+this.framefactor);
			    if(this.playSlider){
			    	this.playSlider.set({maximum: this.duration});
			    }
			    this.videoCanvas.currentTime = this.frame;
			}));
            

            
            var playSliderDiv = domConstruct.create("div", {style : "width: "+ this.xdim +"px;padding-top: 8px;"}, this.parentElement);

			this.time = this.frame; //video start time 
            this.playSlider = HorizontalSlider({
                value: this.frame,
                minimum: this.frame, 
                maximum: this.duration, //initial video end time, 
                intermediateChanges: true,
                onChange: lang.hitch( this, function(value){
                	this.setTime(value);
                })
            });
            this.playSlider.placeAt(playSliderDiv);
            
            
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

            this.myXDim = this.xdim;
            this.myYDim = this.ydim;
            this.myType = this.type;

            //framefactor is available only after video is loaded so set temporary values
        	this.edextent = this.frame+6.0/this.bpm;
        	this.eslow = 2;
        	this.eshigh = 2;

        	//Create button for going to ESframe
        	var resetOption = new Kinetic.Text({
                x: 5,
                y: 5,
                text: 'Go to ES frame',
                fontSize: 12,
                fontFamily: 'Calibri',
                fill: 'black',
                padding: 5,
                align: 'center'
            });

            var maxMWidth = resetOption.getWidth();
            //Bounding Rect
            var rbRect = new Kinetic.Rect({
                x: 5,
                y: 5,
                stroke: '#555',
                strokeWidth: 2,
                fill: '#9dc1e0',
                width: maxMWidth + 3,
                height: resetOption.getHeight(),
                shadowColor: 'black',
                shadowBlur: 3,
                shadowOffset: [10, 10],
                shadowOpacity: 0.2,
                cornerRadius: 3
            });

            this.rbo = new Kinetic.Group();
            this.rbo.add(rbRect);
            this.rbo.add(resetOption);
            this.rbo.on('click', lang.hitch(this, "gotoesframe"));
            this.rbo.on('mouseover', function(){
            	rbRect.setStrokeWidth(4);
            });
            this.rbo.on('mouseout', function(){
            	rbRect.setStrokeWidth(2);
            });
            this.rbo.hide();        
            this.layer.add(this.rbo);
    
            
            // add the layer to the stage
            this.kineticStage.add(this.imageLayer);
            this.kineticStage.add(this.imageLayer1);
            this.kineticStage.add(this.imageLayer2);
            this.kineticStage.add(this.layer);

            this.kineticStage.on("click",lang.hitch(this, function () {
            	var mousePos = this.kineticStage.getPointerPosition();
                var anchorlayer = this.imageLayer1;
                var apts = this.anchorPoints;
                var aptsMeta = this.anchorPointsMeta;
                var allowCreation = false;
                var esframe = false;
            	if((this.estime==0&&this.edextent<this.time)||(this.estime>0 && this.time>this.eslow && this.time<this.eshigh)){
            		anchorlayer = this.imageLayer2;
            		apts = this.esanchorPoints;
            		aptsMeta = this.esanchorPointsMeta;
            		this.imageLayer2.show();
            		if(this.esanchorPoints.length==0){//Do not change time if points have been choosen
	            		this.estime = this.time;
	                	this.eslow = (this.estime-this.framefactor/2);
	                	this.eshigh = (this.estime+this.framefactor/2);
	                	this.rbo.show();
	                	this.layer.draw();
            		}
            		allowCreation = true;
            		esframe = true;
            	}
            	if(this.time<this.edextent){
            		allowCreation = true;
            	}
            	
                if(apts.length < 3 && allowCreation){
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
	
	                        var a1dist = Math.sqrt((x1) * (x1) + (y1) * (y1));
	                        var a2dist = Math.sqrt((x2) * (x2) + (y2) * (y2));
	                        var a3dist = Math.sqrt((x3) * (x3) + (y3) * (y3));
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
	                		mks.push({x:orderedapts[0].getX(),y:orderedapts[0].getY()});
	                		mks.push({x:orderedapts[1].getX(),y:orderedapts[1].getY()});
	                		mks.push({x:orderedapts[2].getX(),y:orderedapts[2].getY()});
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

            var image = new Kinetic.Image({
                image: this.videoCanvas,
                width: this.xdim,
                height: this.ydim
            });

            // add the image to the layer
            this.imageLayer.add(image);

            this.imageLayer.show();
            this.imageLayer.draw();
        },

        startup: function () {

        },

        buildRendering: function () {

        },

        postCreate: function () {

        },

        
        close : function(){

        },
        
        gotoesframe : function(){
        	this.playSlider.set("value", this.estime);
        },
        
        setTime : function(value){
        	this.time = value;
        	this.videoCanvas.currentTime = this.time;
        	if(this.edextent>this.time){
        		this.imageLayer1.show();
        	}else{
        		this.imageLayer1.hide();
        	}
        	if(this.time<this.eshigh&&this.time>this.eslow){
        		this.imageLayer2.show();
        	}else{
        		this.imageLayer2.hide();
        	}
        	//this.kineticStage.draw();
        },
        

        getEStime: function(){
        	return this.estime;
        },
        
        getOrderedCoordinates: function (txdim, tydim) {
            if (this.anchorPoints.length < 2) {
                return;
            }
            var xratio = this.myXDim / txdim;
            var yratio = this.myYDim / tydim;

            var coords = new Array();
            // console.debug(anchorPointsMeta);
            for (var i = 0; i < this.maxPoints; i++) {
                coords.push([
                    this.orderedAnchorPoints[i].getX() / xratio,
                    this.orderedAnchorPoints[i].getY() / yratio, 0.0
                ]);
            }
            return coords;
        },
        
        getOrderedESCoordinates: function (txdim, tydim) {
            if (this.esanchorPoints.length < 2) {
                return;
            }
            var xratio = this.myXDim / txdim;
            var yratio = this.myYDim / tydim;

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

        setFlipRate: function(e){
        	
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
                        
    });
});