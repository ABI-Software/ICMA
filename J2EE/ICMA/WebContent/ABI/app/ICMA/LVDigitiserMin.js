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
["dojo/_base/declare", "dojo/_base/lang", "dojo/topic",
 "dojo/dom-construct", "dojo/dom","dojox/uuid/Uuid",
 "dojo/on","dojox/uuid/generateRandomUuid"], 
function (declare, lang, topic, domConstruct,
		  dom, Uuid, on, generateRandomUuid) {

    // module:
    // ABI/app/ICMA/LVDigitiser

    return declare(
        "ABI.app.ICMA.LVDigitiser",
        null, {
        // summary:
        // A widget that presents an LV digitiser
        // description:
        //		

        element: null,
        img: null,
        xdim: null,
        ydim: null,
        type: null,
        frame: 1.2,
        fstep: 0.1,
        flipFlag: 0,
        refreshRate: 500,
        
        constructor: function (params) {
            dojo.mixin(this, params);

            this.uuid = new Uuid(dojox.uuid.generateRandomUuid()).toString();
            
            this.parentElement = domConstruct.create("div",{
        		style:"width:"+this.xdim+"; height:"+this.ydim+"position:relative;",
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
            //},dom.byId(this.uuid+"KSEL"));
            },dom.byId(this.parentElement));
			
            this.videoCanvas = document.getElementById(this.uuid+"KSVL"); 
            
            this.videoCanvas.load();
            
			//console.debug(this.videoCanvas);
			
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
            this.lineGroup = new Kinetic.Group();
            this.myXDim = this.xdim;
            this.myYDim = this.ydim;
            this.myType = this.type;
            if(this.type=='SAXBASE' || this.type=='SAXMID' || this.type=='SAXAPEX')
            	this.myType = 'SAX';
            this.maxPoints = 3;
            if (this.myType == 'SAX') {
                this.maxPoints = 2;
            }

            // add the layer to the stage
            this.layer.add(this.lineGroup);
            this.kineticStage.add(this.imageLayer);
            this.kineticStage.add(this.imageLayer1);
            this.kineticStage.add(this.imageLayer2);
            this.kineticStage.add(this.layer);

            this.kineticStage.on('click',lang.hitch(this, function () {
                var mousePos = this.kineticStage.getPointerPosition();

                if (this.anchorPoints.length < this.maxPoints) {
                	var paruid = this.uuid;
                    var anchor = new Kinetic.Circle({
                    	uuid: paruid,
                        x: mousePos.x,
                        y: mousePos.y,
                        radius: 4,
                        stroke: "#666",
                        fill: "#ddd",
                        strokeWidth: 2,
                        draggable: true,
                        ox: mousePos.x,
                        oy: mousePos.y
                    });

                    // add hover
                    // styling
                    anchor.on("mouseover", function () {
                        document.body.style.cursor = "pointer";
                        this.setStrokeWidth(4);
                        this.parent.draw();
                    });
                    anchor.on("mouseout", function () {
                        document.body.style.cursor = "default";
                        this.setStrokeWidth(2);
                        this.parent.draw();
                    });
                    anchor.on("dragend", function (b) {
                        var nx = Math.round(this.getX());
                        var ny = Math.round(this.getY());
                        if(nx!=this.ox||ny!=this.oy){
                        	this.ox = nx;
                        	this.oy = ny;
                        	topic.publish(this.attrs.uuid+"/drawSpline");
                        }
                        //topic.publish("drawSpline");
                        this.parent.draw();
                    });

                    this.layer.add(anchor);
                    this.anchorPoints.push(anchor);
                    this.layer.draw();
                    if (this.myType != 'SAX' && this.anchorPoints.length == 3) { // Determine
                        var x1 = this.anchorPoints[0].getX();
                        var x2 = this.anchorPoints[1].getX();
                        var x3 = this.anchorPoints[2].getX();

                        var y1 = this.anchorPoints[0].getY();
                        var y2 = this.anchorPoints[1].getY();
                        var y3 = this.anchorPoints[2].getY();

                        var a1dist = Math.sqrt((x1) * (x1) + (y1) * (y1));
                        var a2dist = Math.sqrt((x2) * (x2) + (y2) * (y2));
                        var a3dist = Math.sqrt((x3) * (x3) + (y3) * (y3));
                        if (a1dist < a2dist && a1dist < a3dist) {
                            if (x2 < x3) {
                                this.anchorPointsMeta[1] = 0;
                                this.anchorPointsMeta[2] = this.maxPoints - 1;
                            } else {
                                this.anchorPointsMeta[2] = 0;
                                this.anchorPointsMeta[1] = this.maxPoints - 1;
                            }
                            apx = x1;
                            apy = y1;
                            this.anchorPointsMeta[0] = 1;
                        }
                        if (a2dist < a1dist && a2dist < a3dist) {
                            if (x1 < x3) {
                                this.anchorPointsMeta[0] = 0;
                                this.anchorPointsMeta[2] = this.maxPoints - 1;
                            } else {
                                this.anchorPointsMeta[2] = 0;
                                this.anchorPointsMeta[0] = this.maxPoints - 1;
                            }
                            apx = x2;
                            apy = y2;
                            this.anchorPointsMeta[1] = 1;
                        }
                        if (a3dist < a2dist && a3dist < a1dist) {
                            if (x2 < x1) {
                                this.anchorPointsMeta[1] = 0;
                                this.anchorPointsMeta[0] = this.maxPoints - 1;
                            } else {
                                this.anchorPointsMeta[0] = 0;
                                this.anchorPointsMeta[1] = this.maxPoints - 1;
                            }
                            apx = x3;
                            apy = y3;
                            this.anchorPointsMeta[2] = 1;
                        }

                        this.layer.draw();
                        this.orderAnchorPoints(); // Order them
                        
                		var mks = new Array();
                		mks.push({x:this.orderedAnchorPoints[0].getX(),y:this.orderedAnchorPoints[0].getY()});
                		mks.push({x:this.orderedAnchorPoints[1].getX(),y:this.orderedAnchorPoints[1].getY()});
                		mks.push({x:this.orderedAnchorPoints[2].getX(),y:this.orderedAnchorPoints[2].getY()});
                        this.nbSpline = new Kinetic.Line({
                            points: mks,
                            stroke: 'yellow',
                            strokeWidth: 5,
                            lineCap: 'round',
                            tension: 1.03,
                            dashArray: [10, 10],
                            opacity: 0.5
                          });
                        this.layer.add(this.nbSpline);
                        this.nbSpline.setZIndex(0); //Since the index search looks up for parent, do this after adding to layer
                        topic.subscribe(this.uuid+"/drawSpline",lang.hitch(this,this.drawSpline));
                    } else if (this.myType == 'SAX' && this.anchorPoints.length == 2) { // Construct
                        var x1 = this.anchorPoints[0].getX();
                        var x2 = this.anchorPoints[1].getX();
                        var y1 = this.anchorPoints[0].getY();
                        var y2 = this.anchorPoints[1].getY();
                        var a1dist = Math.sqrt((x1) * (x1) + (y1) * (y1));
                        var a2dist = Math.sqrt((x2) * (x2) + (y2) * (y2));
                        if (a1dist < a2dist) {
                                this.anchorPointsMeta[0] = 0;
                                this.anchorPointsMeta[1] = 1;
                        } else {
                                this.anchorPointsMeta[1] = 0;
                                this.anchorPointsMeta[0] = 1;
                        }
                        this.orderAnchorPoints();
                        
                        //Draw circle
                        var xm = (x1+x2)*0.5;
                        var ym = (y1+y2)*0.5;
                        var rad = Math.sqrt((x1-xm) * (x1-xm) + (y1-ym) * (y1-ym));
                        this.nbCircle = new Kinetic.Circle({
                        	x: xm,
                        	y: ym,
                        	radius: rad,
                        	stroke: 'yellow',
                        	strokeWidth: 5,
                        	dashArray: [10, 10],
                        	opacity: 0.5,
                        });
                        this.layer.add(this.nbCircle);
                        this.nbCircle.setZIndex(0); //Since the index search looks up for parent, do this after adding to layer
                        topic.subscribe(this.uuid+"/drawSpline",lang.hitch(this,this.drawSpline));
                    }
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
            
            this.refreshInterval = setInterval(lang.hitch(this, function () {
                this.flipImage();
           		this.imageLayer.draw();
                this.layer.draw();
           		this.kineticStage.draw();
            }), this.refreshRate); 
            
        },
        
        
        drawSpline: function(){
        	if(this.myType!='SAX'){
	        	var mks = new Array();
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
	            this.layer.add(this.nbSpline);
	            this.nbSpline.setZIndex(0);
        	}else{
        		//Draw circle
        		var x1 = this.orderedAnchorPoints[0].getX();
        		var y1 = this.orderedAnchorPoints[0].getY();
                var xm = (x1+this.orderedAnchorPoints[1].getX())*0.5;
                var ym = (y1+this.orderedAnchorPoints[1].getY())*0.5;
                var rad = Math.sqrt((x1-xm) * (x1-xm) + (y1-ym) * (y1-ym));
                this.nbCircle.destroy();
                this.nbCircle = new Kinetic.Circle({
                	x: xm,
                	y: ym,
                	radius: rad,
                	stroke: 'yellow',
                	strokeWidth: 5,
                	dashArray: [10, 10],
                	opacity: 0.5
                });
                this.layer.add(this.nbCircle);
	            this.nbCircle.setZIndex(0);
        	}
        },

        startup: function () {

        },

        buildRendering: function () {

        },

        postCreate: function () {

        },

        flipImage: function () {
        	try{
	        	this.flipFlag = 1.0 - this.flipFlag;
	       	    var time = this.frame + this.flipFlag*this.fstep;
	       		this.videoCanvas.currentTime = time;
        	}catch(e){
        		
        	}
        },
        
        setFlipRate: function(rate){
        	if(rate>0){
        		clearInterval(this.refreshInterval);
        		this.refreshInterval = setInterval(lang.hitch(this, function () {
                    this.flipImage();
               		this.imageLayer.draw();
                    this.layer.draw();
               		this.kineticStage.draw();
                }), 5000/rate);         		
        	}else{
        		this.videoCanvas.currentTime = this.frame;
        		clearInterval(this.refreshInterval);
        		this.refreshInterval = setInterval(lang.hitch(this, function () {
               		this.imageLayer.draw();
                    this.layer.draw();
               		this.kineticStage.draw();
                }), this.refreshRate);         		
        	}
        },
        
        close : function(){
        	clearInterval(this.refreshInterval);
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
        
        orderAnchorPoints: function () {
            for (var i = 0; i < this.maxPoints; i++) {
                for (var j = 0; j < this.maxPoints; j++) {
                    if (this.anchorPointsMeta[j] == i) {
                        this.orderedAnchorPoints.push(this.anchorPoints[j]);
                        break;
                    }
                }
            }
        }

    });
});