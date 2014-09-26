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
/*            this.imageLayer1 = new Kinetic.Layer();
            this.imageLayer2 = new Kinetic.Layer();
*/            
            this.anchorPoints = new Array();
            this.orderedAnchorPoints = new Array();
            this.anchorPointsMeta = new Array();
            this.lineGroup = new Kinetic.Group();
            this.myXDim = this.xdim;
            this.myYDim = this.ydim;
            this.myType = this.type;
            if(this.type=='SAXBASE' || this.type=='SAXMID' || this.type=='SAXAPEX')
            	this.myType = 'SAX';
            this.maxPoints = 9;
            if (this.myType == 'SAX') {
                this.maxPoints = 8;
            }

            // add the layer to the stage
            this.layer.add(this.lineGroup);
            this.kineticStage.add(this.imageLayer);
/*            this.kineticStage.add(this.imageLayer1);
            this.kineticStage.add(this.imageLayer2);*/
            this.kineticStage.add(this.layer);

            this.kineticStage.on("click",lang.hitch(this, function () {
                var mousePos = this.kineticStage.getPointerPosition();

                if (this.anchorPoints.length < this.maxPoints) {
                    var anchor = new Kinetic.Circle({
                        x: mousePos.x,
                        y: mousePos.y,
                        radius: 4,
                        stroke: "#666",
                        fill: "#ddd",
                        strokeWidth: 2,
                        draggable: true
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
                        topic.publish("drawLongAxisLines");
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
                        var lbx = 0,
                            rbx = 0,
                            lby = 0,
                            rby = 0,
                            apx = 0;
                        	apy = 0;
                        if (a1dist < a2dist && a1dist < a3dist) {
                            if (x2 < x3) {
                                lbx = x2;
                                rbx = x3;
                                lby = y2;
                                rby = y3;
                                this.anchorPointsMeta[1] = 0;
                                this.anchorPointsMeta[2] = this.maxPoints - 1;
                            } else {
                                rbx = x2;
                                lbx = x3;
                                rby = y2;
                                lby = y3;
                                this.anchorPointsMeta[2] = 0;
                                this.anchorPointsMeta[1] = this.maxPoints - 1;
                            }
                            apx = x1;
                            apy = y1;
                            this.anchorPointsMeta[0] = 4;
                        }
                        if (a2dist < a1dist && a2dist < a3dist) {
                            if (x1 < x3) {
                                lbx = x1;
                                rbx = x3;
                                lby = y1;
                                rby = y3;
                                this.anchorPointsMeta[0] = 0;
                                this.anchorPointsMeta[2] = this.maxPoints - 1;
                            } else {
                                rbx = x1;
                                lbx = x3;
                                rby = y1;
                                lby = y3;
                                this.anchorPointsMeta[2] = 0;
                                this.anchorPointsMeta[0] = this.maxPoints - 1;
                            }
                            apx = x2;
                            apy = y2;
                            this.anchorPointsMeta[1] = 4;
                        }
                        if (a3dist < a2dist && a3dist < a1dist) {
                            if (x2 < x1) {
                                lbx = x2;
                                rbx = x1;
                                lby = y2;
                                rby = y1;
                                this.anchorPointsMeta[1] = 0;
                                this.anchorPointsMeta[0] = this.maxPoints - 1;
                            } else {
                                rbx = x2;
                                lbx = x1;
                                rby = y2;
                                lby = y1;
                                this.anchorPointsMeta[0] = 0;
                                this.anchorPointsMeta[1] = this.maxPoints - 1;
                            }
                            apx = x3;
                            apy = y3;
                            this.anchorPointsMeta[2] = 4;
                        }

                        var stepSZ = 0.25;
                        var biasDist = Math.sqrt(lbx * lbx + lby * lby) / 60;
                        for (var pts = 1; pts < 4; pts++) { // Build
                            // the
                            // lhs
                            var mx = (1.0 - pts * stepSZ) * lbx + pts * stepSZ * apx - pts * biasDist;
                            var my = (1.0 - pts * stepSZ) * lby + pts * stepSZ * apy;
                            this.anchorPointsMeta[2 + pts] = pts;
                            var anchor = new Kinetic.Circle({
                                x: mx,
                                y: my,
                                radius: 4,
                                stroke: "#666",
                                fill: "#ddd",
                                strokeWidth: 2,
                                draggable: true
                            });

                            // add
                            // hover
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
                                topic.publish("drawLongAxisLines");
                                this.parent.draw();
                            });
                            

                            this.layer.add(anchor);
                            this.anchorPoints.push(anchor);
                        }

                        for (var pts = 1; pts < 4; pts++) { // Build
                            // the
                            // rhs
                            var mx = (1.0 - pts * stepSZ) * rbx + pts * stepSZ * apx + pts * biasDist;
                            var my = (1.0 - pts * stepSZ) * rby + pts * stepSZ * apy;
                            this.anchorPointsMeta[5 + pts] = 8 - pts;
                            var anchor = new Kinetic.Circle({
                                x: mx,
                                y: my,
                                radius: 4,
                                stroke: "#666",
                                fill: "#ddd",
                                strokeWidth: 2,
                                draggable: true
                            });

                            // add
                            // hover
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
                                topic.publish("drawLongAxisLines");
                            });
                            this.layer.add(anchor);
                            this.anchorPoints.push(anchor);
                        }
                        //var baseX = 0.5*(rbx+lbx);
                        //var baseY = 0.5*(rby+lby);
                        //var abvX = (baseX-apx);
                        //var abvY = (baseY-apy);
                        //var abvLength = Math.sqrt(abvX*abvX + abvY*abvY);
                                               
                        this.layer.draw();
                        this.orderAnchorPoints(); // Order them

                        topic.subscribe("drawLongAxisLines",lang.hitch(this,this.drawLongAxisLines));
                        this.drawLongAxisLines();

                    } else if (this.myType == 'SAX' && this.anchorPoints.length == 2) { // Construct
                        // a
                        // circle
                        var x1 = this.anchorPoints[0].getX();
                        var x2 = this.anchorPoints[1].getX();
                        var y1 = this.anchorPoints[0].getY();
                        var y2 = this.anchorPoints[1].getY();

                        var midx = 0.5 * (x1 + x2),
                            midy = 0.5 * (y1 + y2);
                        var rad = Math.sqrt((x1 - midx) * (x1 - midx) + (y1 - midy) * (y1 - midy));

                        this.anchorPoints[0].hide();
                        this.anchorPoints[1].hide();
                        this.anchorPoints = new Array();
                        var theta = 0;
                        var delta = Math.PI / 4;
                        for (var pts = 0; pts < this.maxPoints; pts++) {
                            var mx = rad * Math.cos(theta) + midx;
                            var my = rad * Math.sin(theta) + midy;
                            theta += delta;
                            this.anchorPointsMeta[pts] = pts;

                            var anchor = new Kinetic.Circle({
                                x: mx,
                                y: my,
                                radius: 4,
                                stroke: "#666",
                                fill: "#ddd",
                                strokeWidth: 2,
                                draggable: true
                            });
                            
                          
/*                          var anchor = new Kinetic.Label({
                              x: mx,
                              y: my,
                              opacity: 0.75
                            });
                            
                            anchor.add(new Kinetic.Text({
                                text: ''+pts,
                                fontFamily: 'Calibri',
                                fontSize: 18,
                                padding: 5,
                                fill: 'white'
                              }));*/

                            // add
                            // hover
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
                                topic.publish("drawShortAxisLines");
                                this.parent.draw();
                            });
                            this.layer.add(anchor);
                            this.anchorPoints.push(anchor);
                        }
                        this.layer.draw();
                        this.orderAnchorPoints();

                        topic.subscribe("drawShortAxisLines",lang.hitch(this,this.drawShortAxisLines));
                        this.drawShortAxisLines();
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

        startup: function () {

        },

        buildRendering: function () {

        },

        postCreate: function () {

        },

        flipImage: function () {
        	this.flipFlag = 1.0 - this.flipFlag;
       	    var time = this.frame + this.flipFlag*this.fstep;
       		this.videoCanvas.currentTime = time;
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
        
        getEpiApex: function(txdim, tydim){
            var xratio = this.myXDim / txdim;
            var yratio = this.myYDim / tydim;
        	/*var x = ""+this.epiCoord.getX()/xratio;
        	var y = ""+this.epiCoord.getY()/yratio;*/
        	
            var x = this.orderedAnchorPoints[4].getX() / xratio;
            var y = this.orderedAnchorPoints[4].getY() / yratio;
            return [x,y,"0.0"];
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
        },

        drawLongAxisLines: function () {
            if (this.anchorPoints.length < 3)
                return;
            // Draw lines
            this.lineGroup.removeChildren();

            for (var ac = 0; ac < 4; ac++) {
                var ap1 = this.orderedAnchorPoints[ac];
                var ap2 = this.orderedAnchorPoints[8 - ac];
                var pts = Array(ap1.getX(), ap1.getY(), ap2.getX(), ap2.getY());
                var midx = 0.5 * (ap1.getX() + ap2.getX());
                var midy = 0.5 * (ap1.getY() + ap2.getY());
                var sColor = 'white';
                if(ac==0||ac==3){//Color the AHA segment boundary marker
                	sColor = 'yellowgreen';
                }
                var line = new Kinetic.Line({
                    points: pts,
                    stroke: sColor,
                    strokeWidth: 5,
                    lineCap: 'round',
                    lineJoin: 'round'
                });
                this.lineGroup.add(line);
                var midC = new Kinetic.Circle({
                    x: midx,
                    y: midy,
                    radius: 2,
                    stroke: "#666",
                    fill: "red",
                    strokeWidth: 2,
                });
                this.lineGroup.add(midC);
            }
            // Additional lines
            for (var ac = 0; ac < 4; ac++) {
                var ap1 = this.orderedAnchorPoints[ac];
                var ap1n = this.orderedAnchorPoints[ac + 1];
                var ap2 = this.orderedAnchorPoints[8 - ac];
                var ap2n = this.orderedAnchorPoints[7 - ac];
                var ap1x = 0.5 * (ap1.getX() + ap1n.getX());
                var ap1y = 0.5 * (ap1.getY() + ap1n.getY());
                var ap2x = 0.5 * (ap2.getX() + ap2n.getX());
                var ap2y = 0.5 * (ap2.getY() + ap2n.getY());
                var pts = Array(ap1x, ap1y, ap2x, ap2y);
                var midx = 0.5 * (ap1x + ap2x);
                var midy = 0.5 * (ap1y + ap2y);
                var sColor = 'white';
                if(ac==1){//Color the AHA segment boundary marker
                	sColor = 'yellowgreen';
                }
                var line = new Kinetic.Line({
                    points: pts,
                    stroke: sColor,
                    strokeWidth: 5,
                    lineCap: 'round',
                    lineJoin: 'round'
                });
                this.lineGroup.add(line);
                var midC = new Kinetic.Circle({
                    x: midx,
                    y: midy,
                    radius: 2,
                    stroke: "#666",
                    fill: "red",
                    strokeWidth: 2,
                });
                this.lineGroup.add(midC);
            }
            // Polygon
            var polyPts = new Array();
            for (var ac = 0; ac < 9; ac++) {
                var ap1 = this.orderedAnchorPoints[ac];
                polyPts.push(ap1.getX());
                polyPts.push(ap1.getY());
            }
            
            var poly = new Kinetic.Line({
                points: polyPts,
                stroke: 'yellow',
                strokeWidth: 5,
                lineCap: 'round',
                tension: 0.9,
                //dashArray: [10, 10],
                opacity: 0.5
              });
            
            this.lineGroup.add(poly);            

            // Apex base line
            {
                var ap1 = this.orderedAnchorPoints[4];
                var ap2 = this.orderedAnchorPoints[0];
                var ap3 = this.orderedAnchorPoints[8];
                var apex = ap1.getX();
                var apey = ap1.getY();

                var basex = 0.5 * (ap3.getX() + ap2.getX());
                var basey = 0.5 * (ap3.getY() + ap2.getY());
                var pts = Array(apex, apey, basex, basey);
                var line = new Kinetic.Line({
                    points: pts,
                    stroke: 'white',
                    strokeWidth: 5,
                    lineCap: 'round',
                    lineJoin: 'round'
                });
                this.lineGroup.add(line);
            }
        },

        drawShortAxisLines: function () {
            if (this.anchorPoints.length < 2)
                return;
            // Draw lines
            this.lineGroup.removeChildren();
            var pts = new Array();
            for (var ac = 0; ac < 8; ac++) {
                var ap1 = this.orderedAnchorPoints[ac];
                pts.push(ap1.getX());
                pts.push(ap1.getY());
            }

            var poly = new Kinetic.Line({
                points: pts,
                fill: 'grey',
                stroke: 'red',
                strokeWidth: 5,
                opacity: 0.4,
                closed: true
            });
            this.lineGroup.add(poly);
        }

    });
});