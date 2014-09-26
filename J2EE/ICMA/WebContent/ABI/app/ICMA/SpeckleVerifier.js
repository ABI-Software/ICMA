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
 "dojox/uuid/Uuid",
 "dojox/uuid/generateRandomUuid"], 
function (declare, lang, topic, domConstruct,Uuid, generateRandomUuid) {

    // module:
    //		ABI/app/ICMA/SpeckleVerifier

    return declare("ABI.app.ICMA.SpeckleVerifier", null, {
        // summary:
        //		A widget that enables the user to modify speckles
	

        element: null,
        graphElement: null, //Element where strain graph should be displayed
        xdim: null,
        ydim: null,
		xscalefactor: 1,
		yscalefactor: 1,
        urls: null,
        markerArray: null,
        type: null,
        viewPlane: null,
        rendering: false,
        drawingSpline: false,
        nodeSheet: null,
        modifiedNodeCoord: null,
        framevector: null,
        framevectorArray: null,
        parentuuid: "",
        uuid: "",
        constructor: function (params) {
            dojo.mixin(this, params);
            this.uuid = new Uuid(dojox.uuid.generateRandomUuid()).toString();
            
            this.kineticStage = new Kinetic.Stage({
                container: this.element,
                width: this.xdim,
                height: this.ydim
            });

            this.currentStep = 0;
            this.maxSteps = this.urls.length;
            this.markerLayers = new Array();
            this.myImages = new Array(this.maxSteps);
            this.layerAPs = new Array();
            this.layerSpline = new Array();
            this.layerSplineChanged = new Array();
            this.loadedImagesCounter = this.urls.length;
            this.ready = false;
            this.nodeRelationOffset = new Array();
            this.framevectorArray = this.framevector.split(",");
            for (var i = 0; i < this.urls.length; i++) {
                this.myImages[i] = new Image();
                this.myImages[i].onload = lang.hitch(this,"onImageLoad");
                this.myImages[i].src = "."+this.urls[i];
            }
            var numMarkers = this.markerArray[0].length;
            this.nodeSheet = new Array(this.maxSteps);
            for(var c=0;c<this.maxSteps;c++){
            	this.nodeSheet[c] = new Array(numMarkers);
            }
            
            topic.subscribe(this.uuid+"/drawSpline",lang.hitch(this,this.drawSpline,arguments));
            
            //To handle refresh behavior on chrome
            setInterval(lang.hitch(this,"show"), 500);
    },//End of constructor

    
    show: function(){
    	//console.debug("Show called with "+this.ready);
        if (this.ready) {
            this.kineticStage.draw();
        }
    },
    
    startup: function () {

    },

    buildRendering: function () {

    },

    postCreate: function () {

    },

    refreshCurrentAnchorLayer: function () {
        try {
        	if(this.layerSplineChanged[this.currentStep]){
        		this.drawSpline();
        	}
            this.markerLayers[this.currentStep].show();
            this.markerLayers[this.currentStep].draw();
        } catch (e) {
        	console.debug(this.currentStep);
        	console.debug(this.maxSteps);
            console.debug(e);
        }
    },

    createAnchorPoints: function (mx, my,timeInterval,nodeInterval) {
    	var puid = this.uuid;
        var anchor = new Kinetic.Circle({
        	parentuuid: puid,
            x: mx,
            y: my,
            radius: 4,
            stroke: "#666",
            fill: "#ddd",
            strokeWidth: 2,
            draggable: true,
            ox: mx,
            oy: my,
            cx: mx,
            cy: my,
            deleted: false,
            rectx: timeInterval,
            recty: nodeInterval
        });
        this.nodeSheet[timeInterval][nodeInterval] = anchor;
        // add hover styling
        anchor.on("mouseover", function () {
            document.body.style.cursor = "pointer";
            this.setStrokeWidth(6);
        });
        anchor.on("mouseout", function () {
            document.body.style.cursor = "default";
            this.setStrokeWidth(2);
        });
        anchor.on("dragend", function () {
            var nx = Math.round(this.getX());
            var ny = Math.round(this.getY());
            if(nx!=this.ox||ny!=this.oy){
            	this.ox = nx;
            	this.oy = ny;
            	topic.publish(this.attrs.parentuuid+"/drawSpline",this.attrs.rectx,this.attrs.recty);
            }
        });

        return anchor;
    },

    resetAnchors : function(){
    	console.debug("Reset anchors of "+this.currentStep);
    	if(!this.reset){
    		this.reset = true;
    		var aps = this.layerAPs[this.currentStep]; //Get the aps
    		var numMarkers = aps.length;
	
    		if(this.type!='sax'){
	    		//Induce changes to neighboring nodes in space and time
	    		//Note that the cycle is almost periodic
		         var extent = 4;
		         //Modify the affected nodes, resetting all of them to tracked values
	             var ax = this.currentStep;
		         for(var yc=0;yc<numMarkers;yc++){
		             var ay = yc;
		             var anc = this.nodeSheet[ax][ay];
		             for(var cc=1;cc<=extent;cc++){
		            	 var leftx = ax - cc;
		            	 if(leftx<0)
		            		 leftx+=this.maxSteps;
		            	 var ancl =  this.nodeSheet[leftx][ay];
		            	 ancl.setX(ancl.attrs.ox);
		            	 ancl.setY(ancl.attrs.oy);
		            	 this.layerSplineChanged[leftx] = true;
		            	 leftx = ax + cc;
		            	 if(leftx>=this.maxSteps)
		            		 leftx-=this.maxSteps;
		            	 ancl =  this.nodeSheet[leftx][ay];
		            	 ancl.setX(ancl.attrs.ox);
		            	 ancl.setY(ancl.attrs.oy);
		            	 this.layerSplineChanged[leftx] = true;
		             }
		             anc.setX(anc.attrs.ox);
		             anc.setY(anc.attrs.oy);
		         }

		        topic.publish(this.parentuuid+"/RefreshStrainGraph",this.viewPlane);
	            
	    		
	    		//Create a new spline with the points
	    		var mks = new Array();
	    		for (var j = 0; j < numMarkers; j++) {
	                var xval = aps[j].getX();
	                var yval = aps[j].getY();
	                //mks.push({x:xval,y:yval});
	                mks.push(xval);
	                mks.push(yval);
	            }
	
	            var nbSpline = new Kinetic.Line({
	                points: mks,
	                stroke: 'yellow',
	                strokeWidth: 5,
	                lineCap: 'round',
	                tension: 0.9,
	                dashArray: [10, 10],
	                opacity: 0.5
	              });
	    		
	    		var bspline = this.layerSpline[this.currentStep];
	    		var slayer = bspline.getLayer();
	    		var zindex = bspline.getZIndex();
	    		bspline.destroy();
	    		slayer.add(nbSpline);
	    		nbSpline.setZIndex(zindex);
	    		slayer.draw();
	    		this.layerSpline[this.currentStep] = nbSpline;
	    		this.layerSplineChanged[this.currentStep] = false;
	    	}else{
	    		//Induce changes to neighboring nodes in space and time
	    			for(var c =0; c<this.maxSteps;c++){
    					var bspline = this.layerSpline[c];
			    		bspline.setX(this.splineInitOffset.x);
			    		bspline.setY(this.splineInitOffset.y);
			    		var slayer = bspline.getLayer();
			    		slayer.draw();
			    		this.layerSpline[c] = bspline;
	    			} 
	    			this.splineOffset = this.splineInitOffset;
	    	}

    		this.reset = false;
    	}
    },

    createMarkerLayer: function (markers, myImage,timeInterval) {
        var mlayer = new Kinetic.Layer();
        var kImage = new Kinetic.Image({
            image: myImage,
            width: this.xdim,
            height: this.ydim
        });
        mlayer.add(kImage);
        //Add Reset button

        var resetOption = new Kinetic.Text({
            x: 5,
            y: 5,
            text: 'Reset to tracked results',
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

        var rbo = new Kinetic.Group();
        rbo.add(rbRect);
        rbo.add(resetOption);
        rbo.on('click', lang.hitch(this, "resetAnchors"));
        rbo.on('mouseover', function(){
        	rbRect.setStrokeWidth(4);
        });
        rbo.on('mouseout', function(){
        	rbRect.setStrokeWidth(2);
        });
                
        mlayer.add(rbo);
        
        var layerAp = new Array(); //Record the aps
        if(this.type!='sax'){
	        var mks = new Array();
	        for (var c = 0; c < markers.length; c++) {
	            var coord = markers[c];
	            var xval = coord[0]*this.xscalefactor;
	            var yval = coord[1]*this.yscalefactor;
	            //mks.push({x:xval,y:yval});
	            mks.push(xval);
	            mks.push(yval);
	        }
	
	        var bSpline = new Kinetic.Line({
	            points: mks,
	            stroke: 'yellow',
	            strokeWidth: 5,
	            lineCap: 'round',
	            tension: 0.9,
	            dashArray: [10, 10],
	            opacity: 0.5
	          });
	        mlayer.add(bSpline);
	        this.layerSpline.push(bSpline);
	        this.layerSplineChanged.push(false);
	        for (var c = 0; c < mks.length; c+=2){ 
	            var ap = this.createAnchorPoints(mks[c], mks[c+1],timeInterval,Math.floor(c/2));
	            layerAp.push(ap);
	            mlayer.add(ap);
	        }
        }else{
	        var mks = new Array();
	        var mx = 0;
	        var my = 0;
	        var numMarkers = markers.length;
	        for (var c = 0; c < numMarkers; c++) {
	            var coord = markers[c];
	            var xval = coord[0]*this.xscalefactor;
	            var yval = coord[1]*this.yscalefactor;
	            //mks.push({x:xval,y:yval});
	            mks.push(xval);
	            mks.push(yval);
	            mx += xval;
	            my += yval;
	        }
	        mx /= numMarkers;
	        my /= numMarkers;
	        {
	        	var coord = markers[0];
	            var xval = coord[0]*this.xscalefactor;
	            var yval = coord[1]*this.yscalefactor;
	            //mks.push({x:xval,y:yval});
	            mks.push(xval);
	            mks.push(yval);
	        }
	        this.centroid = {x:mx,y:my};
	        
	        var bSpline = new Kinetic.Line({
	        	parentuuid: this.uuid,
	            points: mks,
	            stroke: 'yellow',
	            strokeWidth: 5,
                draggable: true,
	            lineCap: 'round',
	            tension: 0.9,
	            dashArray: [10, 10],
	            opacity: 0.5
	          });
	        
	        
	        bSpline.on("dragend", function () {
	            var nx = Math.round(this.getX());
	            var ny = Math.round(this.getY());
            	topic.publish(this.attrs.parentuuid+"/drawSpline",nx,ny);
	        });
	        if(this.splineInitOffset===undefined){
	        	this.splineInitOffset = {x:bSpline.getX(),y:bSpline.getY()};
	        	this.splineOffset = {x:bSpline.getX(),y:bSpline.getY()};
	        }
	        	
	        mlayer.add(bSpline);
	        this.layerSpline.push(bSpline);
	        this.layerSplineChanged.push(false);
        }
        this.layerAPs.push(layerAp);
        mlayer.hide(); //by default the layers are hidden
        this.kineticStage.add(mlayer);
        return mlayer;
    },


    createMarkerLayers: function () {
        for (var c = 0; c < this.markerArray.length; c++) {
            this.markerLayers[c] = this.createMarkerLayer(this.markerArray[c], this.myImages[c],c);
        }
        //Enable the first frame's markers
        this.currentStep = 0;
        this.refreshCurrentAnchorLayer();
        this.ready = true;
    },

    onImageLoad: function () {
        this.loadedImagesCounter--;
        if (this.loadedImagesCounter <= 0) {
            this.createMarkerLayers();
        }
    },

    setStep: function (step) {
    	var rstep = Math.round(step);
    	if(rstep<0 || rstep >= this.maxSteps){
    		return;
    	}
    	
    	if(!this.rendering){
    		this.rendering = true;
	        if (rstep != this.currentStep) {
	            this.markerLayers[this.currentStep].hide();
	            this.currentStep = rstep;
	            this.refreshCurrentAnchorLayer();
	        }
	        this.kineticStage.draw();
	        this.rendering = false;
    	}
    },

    incTime: function () {
        if (this.ready) {
            this.markerLayers[currentStep].hide();
            this.currentStep++;
            if (this.currentStep >= this.maxSteps) {
                this.currentStep = 0;
            }
            this.refreshCurrentAnchorLayer();
        }
    },

    decTime: function () {
        if (this.ready) {
            this.markerLayers[currentStep].hide();
            this.currentStep--;
            if (this.currentStep < 0) {
                this.currentStep = this.maxSteps - 1;
            }
            this.refreshCurrentAnchorLayer();
        }
    },

    getMarkers: function () {//The markers are in LB to RB order
        if (this.ready) {
            var marr = new Array(this.maxSteps);
            if(this.type=='sax'){
            	var xoff = (this.splineOffset.x - this.splineInitOffset.x)/this.xscalefactor;
            	var yoff = (this.splineOffset.y - this.splineInitOffset.y)/this.yscalefactor;
	            for (var i = 0; i < this.markerArray.length; i++) {
	                var coords = new Array();
	                var aps = this.markerArray[i];
	                for (var j = 0; j < aps.length; j++) {
	                    var x = parseFloat(aps[j][0])+ xoff;
	                    var y = parseFloat(aps[j][1])+ yoff;
	                    coords.push([""+x, ""+y]); //Ensure that string is sent
	                }
	                marr[i] = coords;
	            }
            }else{
	            for (var i = 0; i < this.layerAPs.length; i++) {
	                var coords = new Array();
	                var aps = this.layerAPs[i];
	                for (var j = 0; j < aps.length; j++) {
	                    var x = aps[j].getX()/this.xscalefactor;
	                    var y = aps[j].getY()/this.yscalefactor;
	                    coords.push([""+x, ""+y]); //Ensure that string is sent
	                }
	                marr[i] = coords;
	            }
            }
            return marr;
        }
        return this.markerArray;
    },
    
    drawSpline: function() {
    	if(!this.drawingSpline){
    		this.drawingSpline = true;
    		var aps = this.layerAPs[this.currentStep]; //Get the aps
    		var numMarkers = aps.length;
    		if(this.type!='sax'){
	    		//Induce changes to neighboring nodes in space and time
	    		//Note that the cycle is almost periodic
	            if(arguments[1]!=undefined||arguments[2]!=undefined){
		             var extent = 4;
		             var yextent = 3;
		             //Modify the affected nodes
		             var ax = arguments[1];
		             var ay = arguments[2];
		             var anc = this.nodeSheet[ax][ay];
		             var xchange = anc.getX() - anc.attrs.ox;
		             var ychange = anc.getY() - anc.attrs.oy;
		             var xper    = xchange/anc.getX();
		             var yper    = ychange/anc.getY();
		             //for(var cc=1;cc<=extent;cc++){
		            //	 var leftx = ax - cc;
		            //	 if(leftx<0)
		            //		 leftx+=this.maxSteps;
		            //	 var ancl =  this.nodeSheet[leftx][ay];
		            //	 var weight = 0.5*(extent - cc + 1)/extent;
		            //	 ancl.attrs.cx = ancl.getX();
		            //	 ancl.attrs.cy = ancl.getY();
		            //	 ancl.setX(ancl.attrs.cx + xchange*weight);
		            //	 ancl.setY(ancl.attrs.cy + ychange*weight);
		            //	 this.layerSplineChanged[leftx] = true;
		            //	 leftx = ax + cc;
		            //	 if(leftx>=this.maxSteps)
		            //		 leftx-=this.maxSteps;
		            //	 ancl =  this.nodeSheet[leftx][ay];
		            //	 ancl.attrs.cx = ancl.getX();
		            //	 ancl.attrs.cy = ancl.getY();
		            //	 ancl.setX(ancl.attrs.cx + xchange*weight);
		            //	 ancl.setY(ancl.attrs.cy + ychange*weight);
		            //	 this.layerSplineChanged[leftx] = true;
		             //}
	
		            // for(var cc=1;cc<=yextent;cc++){
		            //	 var topy = ay - cc;
		            //	 var weight = 0.25*(yextent - cc + 1)/yextent;
		            //	 if(topy>0){
			        //    	 var ancl =  this.nodeSheet[ax][topy];
			        //   	 ancl.attrs.cx = ancl.getX();
			        //  	 ancl.attrs.cy = ancl.getY();
			        //    	 ancl.setX(ancl.attrs.cx + xchange*weight);
			        //    	 ancl.setY(ancl.attrs.cy + ychange*weight);
		            //	 }
		            //	 topy = ay + cc;
		            //	 if(topy<numMarkers){
			        //    	 var ancl =  this.nodeSheet[ax][topy];
			        //    	 ancl.attrs.cx = ancl.getX();
			        //    	 ancl.attrs.cy = ancl.getY();
			        //    	 ancl.setX(ancl.attrs.cx + xchange*weight);
			        //    	 ancl.setY(ancl.attrs.cy + ychange*weight);
		            //	 }
		            // }
		             for(var cc=1;cc<=extent;cc++){
		            	 var leftx = ax - cc;
		            	 if(leftx<0)
		            		 leftx+=this.maxSteps;
		            	 var ancl =  this.nodeSheet[leftx][ay];
		            	 var weight = 0.5*(extent - cc + 1)/extent;
		            	 ancl.attrs.cx = ancl.getX();
		            	 ancl.attrs.cy = ancl.getY();
		            	 xchange = ancl.attrs.cx*xper;
		            	 ychange = ancl.attrs.cy*yper;
		            	 ancl.setX(ancl.attrs.cx + xchange*weight);
		            	 ancl.setY(ancl.attrs.cy + ychange*weight);
		            	 this.layerSplineChanged[leftx] = true;
		            	 leftx = ax + cc;
		            	 if(leftx>=this.maxSteps)
		            		 leftx-=this.maxSteps;
		            	 ancl =  this.nodeSheet[leftx][ay];
		            	 ancl.attrs.cx = ancl.getX();
		            	 ancl.attrs.cy = ancl.getY();
		            	 xchange = ancl.attrs.cx*xper;
		            	 ychange = ancl.attrs.cy*yper;
		            	 ancl.setX(ancl.attrs.cx + xchange*weight);
		            	 ancl.setY(ancl.attrs.cy + ychange*weight);
		            	 this.layerSplineChanged[leftx] = true;
		             }
	
		             for(var cc=1;cc<=yextent;cc++){
		            	 var topy = ay - cc;
		            	 if( (ay<5 && topy<5) || (ay>5 && topy>5) || ay==5){
			            	 var weight = 0.25*(yextent - cc + 1)/yextent;
			            	 if(topy>0){
				            	 var ancl =  this.nodeSheet[ax][topy];
				            	 ancl.attrs.cx = ancl.getX();
				            	 ancl.attrs.cy = ancl.getY();
				            	 xchange = ancl.attrs.cx*xper;
				            	 ychange = ancl.attrs.cy*yper;
				            	 ancl.setX(ancl.attrs.cx + xchange*weight);
				            	 ancl.setY(ancl.attrs.cy + ychange*weight);
			            	 }
		            	 }
		            	 topy = ay + cc;
		            	 if( (ay<5 && topy<5) || (ay>5 && topy>5) || ay==5){
		            		 var weight = 0.25*(yextent - cc + 1)/yextent;
			            	 if(topy<numMarkers){
				            	 var ancl =  this.nodeSheet[ax][topy];
				            	 ancl.attrs.cx = ancl.getX();
				            	 ancl.attrs.cy = ancl.getY();
				            	 xchange = ancl.attrs.cx*xper;
				            	 ychange = ancl.attrs.cy*yper;
				            	 ancl.setX(ancl.attrs.cx + xchange*weight);
				            	 ancl.setY(ancl.attrs.cy + ychange*weight);
			            	 }
		            	 }
		             }
		             topic.publish(this.parentuuid+"/RefreshStrainGraph",this.viewPlane);
	            }
	    		
	    		//Create a new spline with the points
	    		var mks = new Array();
	    		for (var j = 0; j < numMarkers; j++) {
	                var xval = aps[j].getX();
	                var yval = aps[j].getY();
	                //mks.push({x:xval,y:yval});
	                mks.push(xval);
	                mks.push(yval);
	            }
	
	            var nbSpline = new Kinetic.Line({
	                points: mks,
	                stroke: 'yellow',
	                strokeWidth: 5,
	                lineCap: 'round',
	                tension: 0.9,
	                dashArray: [10, 10],
	                opacity: 0.5
	              });
	    		
	    		var bspline = this.layerSpline[this.currentStep];
	    		var slayer = bspline.getLayer();
	    		var zindex = bspline.getZIndex();
	    		bspline.destroy();
	    		slayer.add(nbSpline);
	    		nbSpline.setZIndex(zindex);
	    		slayer.draw();
	    		this.layerSpline[this.currentStep] = nbSpline;
	    		this.layerSplineChanged[this.currentStep] = false;
	    		this.drawingSpline = false;
	    	}else{
	    		//Induce changes to neighboring nodes in space and time
	    		 if(arguments[1]!=undefined||arguments[2]!=undefined){
/*		    		var extent = 4;
		    		for(var c = 1;c<extent;c++){
		    			var left = this.currentStep -c;
		    			if(left<0)
		    				left += this.maxSteps;
			    		var bspline = this.layerSpline[left];
			    		bspline.setX(arguments[1]);
			    		bspline.setY(arguments[2]);
			    		var slayer = bspline.getLayer();
			    		slayer.draw();
			    		this.layerSpline[left] = bspline;
			    		left = this.currentStep + c;
			    		if(left>=this.maxSteps)
			    			left -=this.maxSteps;
			    		bspline = this.layerSpline[left];
			    		bspline.setX(arguments[1]);
			    		bspline.setY(arguments[2]);
			    		slayer = bspline.getLayer();
			    		slayer.draw();
			    		this.layerSpline[left] = bspline;
		    		}
		    		this.drawingSpline = false;*/
	    			for(var c =0; c<this.maxSteps;c++){
	    				if(c!=this.currentStep){
	    					var bspline = this.layerSpline[c];
				    		bspline.setX(arguments[1]);
				    		bspline.setY(arguments[2]);
				    		var slayer = bspline.getLayer();
				    		slayer.draw();
				    		this.layerSpline[c] = bspline;
	    				}
	    			} 
	    			this.splineOffset = {x:arguments[1],y:arguments[2]};
	    		}
	    }
      }
    },
    
    computeStrains: function () {
    	var markers = this.getMarkers();
    	var strains = new Array();
    	if(this.type!='sax'){
    		var strainLengths = new Array();
    		  for (var i = 0; i < this.maxSteps; i++) {//For each time step
    			 var coords = markers[i];
    			 var slengths = new Array();
    			 var lengths = new Array();
    			 for(var j=0; j<coords.length-1; j++){
    				 var x1 = coords[j];
    				 var x2 = coords[j+1];
    				 var d0 = (x1[0]-x2[0]);
    				 var d1 = (x1[1]-x2[1]);
    				 var length = Math.sqrt(d0*d0+d1*d1);
    				 slengths.push(length);
    				 //console.debug(i+"\t"+j+"\t"+coords[j]+"\t"+coords[j+1]+"\t"+length);
    			 }
   			 
    			 
    			 lengths.push(slengths[0] + (1 / 3.0 - 1 / 4.0) * 4 * slengths[1]);
    			 lengths.push((1.0 - (1 / 3.0 - 1 / 4.0)) * 4 * slengths[1] + (2 / 3.0 - 1 / 2.0) * 4 * slengths[2]);
    			 lengths.push((1.0 - (2 / 3.0 - 1 / 2.0)) * 4 * slengths[2] + slengths[3]);
    			 lengths.push(slengths[4] + (1.0 - (2 / 3.0 - 1 / 2.0)) * 4 * slengths[5]);
    			 lengths.push((2 / 3.0 - 1 / 2.0) * 4 * slengths[5] + (1.0 - (1 / 3.0 - 1 / 4.0)) * 4 * slengths[6]);
    			 lengths.push((1 / 3.0 - 1 / 4.0) * 4 * slengths[6] + slengths[7]);
    			 
    			 strainLengths.push(lengths);
    		}
    		var initStrainLengths = strainLengths[0];
    		for (var i = 1; i < this.maxSteps; i++) {//Compute Strains
    			var curStrainLengths = strainLengths[i];
    			var curStrain = new Array();
    			for(var j=0;j<initStrainLengths.length;j++){
    				var c = 100*(curStrainLengths[j] - initStrainLengths[j])/initStrainLengths[j];
    				curStrain.push(c);
    			}
    			strains.push(curStrain);
    		}
    		var strainSeries = new Array();
    		for(var j=0;j<initStrainLengths.length;j++){
    			var sl = new Array();
    			sl.push({x: 0, y :0.0});//For init step
    			var maxstep = this.maxSteps-1;
    			var maxStrain = strains[maxstep-1][j];
    			for (var i = 0; i < maxstep; i++) {//Compute Strains with drift compensation
    				var val = strains[i][j]-(i+1)*maxStrain/maxstep; 
    				sl.push({x: this.framevectorArray[i+1],y:val});
    			}
    			strainSeries.push(sl);
    		}
    		return strainSeries;
    	}
    }
    
}); 
});