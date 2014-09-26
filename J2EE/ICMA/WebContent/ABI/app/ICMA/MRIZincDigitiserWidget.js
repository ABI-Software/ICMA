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
        "dojo/Deferred",
        "dojo/dom",
        "dojo/dom-class",
        "dojo/dom-construct",
        "dojo/dom-style",
        "dojo/topic",
        "dojo/on",
        "dojo/query",
        "dojox/mobile/Carousel",
        "dojox/mobile/CarouselItem",
        "dijit/form/Button",
        "dojox/mobile/SwapView",
        "dojox/mobile/TextBox",
        "dojox/mobile/Video",
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
        "ABI/lib/Widgets/Alert"
], function (declare, array, event, lang, win, has, aspect, locale, Deferred, dom, domClass, domConstruct, domStyle, topic, on, query, Carousel, CarouselItem, Button, SwapView,
    TextBox, Video, TableContainer, Dialog, TitlePane,
    DijitTextBox, BorderContainer, ContentPane, TabContainer, Contained, Container, WidgetBase, Alert) {

    // module:
    //		ABI/app/ICMA/MRIZincDigitiserWidget

    return declare("ABI.app.ICMA.MRIZincDigistiserWidget", [ContentPane], {
        // summary:
        //		A zinc widget 
        // description:
        //		

        time: 0,
        maxTime: 0, //Maximum time that can be set
        parentPlugin: null, //handle to parent zinc plugin
        viewParameters: null, //Set lookup parameters for rendering
        viewAngle: 0.471,
        meta: null, //Any meta data that the end user might wish to store
        debug: false,
        timeoutHandle: null,
        open: true,
        showMarkers: false,
        previousTime: -1,
        initialisedSharedInstance: false,
        timeFactor : 18,
        frameTimeVector: null,
        edbasepoints : null, // Array containing cmiss nodes of two points
        edframe      : 0,
        esbasepoints : null, // Array containing cmiss nodes of two points
        esframe      : 0,
        apex		 : null, // Coordinate of Apex 
        endoLandMarks: null, // Array of array of nodes of endocardial landmarks
        epiLandMarks : null, // Array of array of nodes of epicardial landmarks

        
        constructor: function (params) {
            dojo.mixin(this, params);

            this.regionNames = new Array(); //Names of regions 
            this.regionHasProjectedCoordinates = new Object(); //If a region's rendering uses projected coordinates - the entry is true
            this.regions = new Object(); //Handles to regions
            this.landmarkRegions = new Array();
            this.visibleRegion = new Array(); // initialized in showAllRegions
            this.regionMaterials = new Object();
            this.mesh_loaded = new Object();
            this.images_loaded = new Object();
            this.edbasepoints = new Array(); // Array containing cmiss nodes of two points
            this.esbasepoints = new Array(); // Array containing cmiss nodes of two points
            this.endoLandMarks= new Array(); // Array (for each frame) of array of cmiss nodes of endocardial landmarks
            this.epiLandMarks = new Array(); // Array (for each frame) of array of cmiss nodes of epicardial landmarks
            for(var i=0;i<this.timeFactor;i++){
            	this.endoLandMarks.push(new Array());
            	this.epiLandMarks.push(new Array());
            }
        },

        startup: function () {
            if (this._started) {
                return;
            }
            if(this.debug)
            	console.log("zinc widget startup: " + this.zincId);

            topic.subscribe("/onzincready/" + this.zincId, lang.hitch(this, this.attachSharedInstance));

            // This needs to be included to provide restart stub calls after re-attachment
            //          topic.subscribe("/onsceneviewerready/" + this.zincId, lang.hitch(this, this.restartSharedInstance));

            topic.subscribe("/onsceneviewerreadyNotSharedInstance/" + this.zincId, lang.hitch(this, this.ready));

            topic.subscribe("/attachSharedInstance/" + this.zincId, lang.hitch(this, this.attachSharedInstance));
            
            this.deferred = new Deferred();

            this._resizeCalled = true; // This forces the _onShow to be called from inherited startup()
            this.inherited(arguments);

            return this.deferred.promise;
        },


        buildRendering: function () {
            this.inherited(arguments);
        },

        postCreate: function () {
            this.inherited(arguments);
        },

        getLookAtParameters: function () {
            var parm = new Array();
            for (var i = 0; i < 9; i++) {
                parm.push(0.0);
            }
            var sceneViewer2 = this.zincHandle.getSceneViewerFromInteger(this.zincHandle.sceneViewerIntLow, this.zincHandle.sceneViewerIntHigh);
            sceneViewer2.getLookAtParameters(parm);
            return parm;
        },


        setViewParameters: function (vParam) {
            this.viewParameters = new Array();
            var parm = vParam.split(" ");
            for (var i = 0; i < parm.length; i++) {
                this.viewParameters.push(parm[i] * 1.0);
            }
        },


        setTumbleRate: function (rate) {
            var sceneViewer2 = this.zincHandle.getSceneViewerFromInteger(
                this.zincHandle.sceneViewerIntLow,
                this.zincHandle.sceneViewerIntHigh);
            sceneViewer2.tumbleRate = rate;
        },


        viewAll: function () {
            var sceneViewer2 = this.zincHandle.getSceneViewerFromInteger(
                this.zincHandle.sceneViewerIntLow,
                this.zincHandle.sceneViewerIntHigh);
            sceneViewer2.viewAll();

            if (this.viewParameters != null) {
	            var parm = this.viewParameters;
	            sceneViewer2.setLookAtParametersNonSkew(parm[0], parm[1], parm[2], parm[3], parm[4], parm[5], parm[6], parm[7], parm[8]);
            }
            if (this.viewAngle)
                sceneViewer2.viewAngle = this.viewAngle;// * 3.14 / 180;

        },

        computeModelViewParameters: function () {
            if (this.viewParameters == undefined)
                this.setViewParameters('1125 1247 -1718 400 300 -6 0.47 -0.83 -0.26');
            if (this.viewAngle == undefined)
                this.viewAngle = 0.471;
        },


        clearDisplay: function () {
            var graphicsModule = this.zincHandle.context.getDefaultGraphicsModule();
            var new_scene = graphicsModule.createScene();
            new_scene.setName('BlankScene');
            var rootRegion = this.zincHandle.context.getDefaultRegion();
            new_scene.setRegion(rootRegion); //Set the region to root
            new_scene.setFilter(graphicsModule.createFilterGraphicName("BLANK"));
            var sceneViewer2 = plugin.zincHandle.getSceneViewerFromInteger(
                splugin.zincHandle.sceneViewerIntLow,
                splugin.zincHandle.sceneViewerIntHigh);
            sceneViewer2.setScene(new_scene);
        },

        clear: function (regionName) {
            var graphicsModule = this.zincHandle.context.getDefaultGraphicsModule();
            //Get the current view parameters
            var sceneViewer2 = this.zincHandle.getSceneViewerFromInteger(
                    this.zincHandle.sceneViewerIntLow,
                    this.zincHandle.sceneViewerIntHigh);
            var currentViewAngle = sceneViewer2.viewAngle;
            var parm = new Array();
            for (var i = 0; i < 9; i++) {
            	parm.push(0.0);
            }
            sceneViewer2.getLookAtParameters(parm);
            var new_scene = graphicsModule.createScene();
            var rootRegion = this.zincHandle.context.getDefaultRegion();
            new_scene.setName('SceneWithout'+regionName);
            new_scene.setRegion(rootRegion); //Set the region to root
            this.removeVisibleRegion(regionName);
            if(this.visibleRegion.length>0){
	            /* Create filter region */
	            var sregions = new Array();
	            for (var i = 0; i <  this.visibleRegion.length; i++) {
	                    var gelem = this.visibleRegion[i];
	                   	if (this.debug)
	                   		console.debug("Enabling region " + gelem);
	                   	sregions.push(graphicsModule.createFilterGraphicName("lines"+gelem));
	                   	sregions.push(graphicsModule.createFilterGraphicName("surface"+gelem));
	            }
	            var graphicFilterOr = graphicsModule.createFilterOperatorOr();
	            graphicFilterOr.appendOperand(sregions[0]); // The or operator is binary so chain it
	            for (var i = 1; i < sregions.length; i++) {
	               graphicFilterOr.appendOperand(sregions[i]);
	               if ((i + 1) < sregions.length) {
	            	   var tgraphicFilterOr = graphicsModule.createFilterOperatorOr();
	            	   tgraphicFilterOr.appendOperand(graphicFilterOr);
	                   graphicFilterOr = tgraphicFilterOr;
	               }
	            }
	            
	            this.activegraphicFilterOr = graphicFilterOr;
	            this.viewParameters = parm;
	            this.viewAngle = currentViewAngle;
	            //Add one for the current time
	     	    var tgraphicFilterOr = graphicsModule.createFilterOperatorOr();
	    	    tgraphicFilterOr.appendOperand(graphicFilterOr);
	    	    tgraphicFilterOr.appendOperand(graphicsModule.createFilterGraphicName("Time"+Math.floor(this.time)));
	            
	            new_scene.setFilter(tgraphicFilterOr);
            }else{
                 new_scene.setFilter(graphicsModule.createFilterGraphicName("BLANK"));
            }
            sceneViewer2.setScene(new_scene);
            sceneViewer2.viewAll();
            sceneViewer2.setLookAtParametersNonSkew(parm[0], parm[1], parm[2], parm[3], parm[4], parm[5], parm[6], parm[7], parm[8]);
            sceneViewer2.viewAngle = currentViewAngle;
        },
        
        showAllRegions: function () {
            var graphicsModule = this.zincHandle.context.getDefaultGraphicsModule();

            this.visibleRegion = new Array();
            for (item in this.regions) {
            	var regionName = item;
            	this.visibleRegion.push(regionName);
            	var sliceRendition = graphicsModule.getRendition(this.regions[item]);
                sliceRendition.beginChange();
                sliceRendition.executeCommand("general clear");
                sliceRendition.executeCommand("lines coordinate coordinates select_on material default as lines"+regionName); 
                sliceRendition.executeCommand("surfaces coordinate coordinates face xi3_0 select_on material " + regionName + " texture_coordinates ltexture_coordinates selected_material " + regionName + " as surface" + regionName);
                sliceRendition.endChange();
            }
            //Create scene filter for downstream calls
            var sregions = new Array();
            for (var i = 0; i <  this.visibleRegion.length; i++) {
                    var gelem = this.visibleRegion[i];
                   	if (this.debug)
                   		console.debug("Enabling region " + gelem);
                   	sregions.push(graphicsModule.createFilterGraphicName("lines"+gelem));
                   	sregions.push(graphicsModule.createFilterGraphicName("surface"+gelem));
            }
            var graphicFilterOr = graphicsModule.createFilterOperatorOr();
            graphicFilterOr.appendOperand(sregions[0]); // The or operator is binary so chain it
            for (var i = 1; i < sregions.length; i++) {
               graphicFilterOr.appendOperand(sregions[i]);
               if ((i + 1) < sregions.length) {
            	   var tgraphicFilterOr = graphicsModule.createFilterOperatorOr();
            	   tgraphicFilterOr.appendOperand(graphicFilterOr);
                   graphicFilterOr = tgraphicFilterOr;
               }
            }
            this.activegraphicFilterOr = graphicFilterOr;
            this.setTime(this.time);
            
        },
        
        setupInteractionTool: function(){
/*        	var rootRegion = this.zincHandle.context.getDefaultRegion();
        	var graphicsModule = this.zincHandle.context.getDefaultGraphicsModule();
            var fieldModule = rootRegion.getFieldModule();
            fieldModule.defineField("coordinates", "finite_element num 3 coordinate comp x y z");
            {
            	var sliceRendition = graphicsModule.getRendition(rootRegion);
                sliceRendition.beginChange();
                sliceRendition.executeCommand("node_points select_on material default glyph sphere general size 0.1 coordinate coordinates as nodesofroot"); 
                sliceRendition.endChange();
            }
            this.zincHandle.sceneViewer.setInteractiveToolByName('node_tool');
        	var interactiveTool = this.zincHandle.sceneViewer.getCurrentInteractiveTool();
        	interactiveTool.executeCommand('group "/" coordinate_field coordinates');
        	interactiveTool.executeCommand('create define edit select');*/
        },
        
        showRegion: function (regionName) {
            var graphicsModule = this.zincHandle.context.getDefaultGraphicsModule();
          
            //Get the current view parameters
            var sceneViewer2 = this.zincHandle.getSceneViewerFromInteger(
                    this.zincHandle.sceneViewerIntLow,
                    this.zincHandle.sceneViewerIntHigh);
            var currentViewAngle = sceneViewer2.viewAngle;
            var parm = new Array();
            for (var i = 0; i < 9; i++) {
            	parm.push(0.0);
            }
            sceneViewer2.getLookAtParameters(parm);
            
            this.addVisibleRegion(regionName);
            
            var new_scene = graphicsModule.createScene();
            new_scene.setName('SceneWith'+regionName);
            var rootRegion = this.zincHandle.context.getDefaultRegion();
            new_scene.setRegion(rootRegion); //Set the region to root
            /* Create filter region */
            var sregions = new Array();
            for (var i = 0; i <  this.visibleRegion.length; i++) {
                    var gelem = this.visibleRegion[i];
                   	if (this.debug)
                   		console.debug("Enabling region " + gelem);
                   	sregions.push(graphicsModule.createFilterGraphicName("lines"+gelem));
                   	sregions.push(graphicsModule.createFilterGraphicName("surface"+gelem));
            }
            var graphicFilterOr = graphicsModule.createFilterOperatorOr();
            graphicFilterOr.appendOperand(sregions[0]); // The or operator is binary so chain it
            for (var i = 1; i < sregions.length; i++) {
               graphicFilterOr.appendOperand(sregions[i]);
               if ((i + 1) < sregions.length) {
            	   var tgraphicFilterOr = graphicsModule.createFilterOperatorOr();
            	   tgraphicFilterOr.appendOperand(graphicFilterOr);
                   graphicFilterOr = tgraphicFilterOr;
               }
            }

            
            this.activegraphicFilterOr = graphicFilterOr;
            this.viewParameters = parm;
            this.viewAngle = currentViewAngle;
            //Add one for the current time
     	    var tgraphicFilterOr = graphicsModule.createFilterOperatorOr();
    	    tgraphicFilterOr.appendOperand(graphicFilterOr);
    	    tgraphicFilterOr.appendOperand(graphicsModule.createFilterGraphicName("Time"+Math.floor(this.time)));
    	    
            new_scene.setFilter(tgraphicFilterOr);
            sceneViewer2.setScene(new_scene);
            sceneViewer2.viewAll();
            sceneViewer2.setLookAtParametersNonSkew(parm[0], parm[1], parm[2], parm[3], parm[4], parm[5], parm[6], parm[7], parm[8]);
            sceneViewer2.viewAngle = currentViewAngle;
            
        },


        addRegion: function (regionName) {
            var rootRegion = this.zincHandle.context.getDefaultRegion();
            if (regionName != "") {
                if (this.regions[regionName] == undefined) {
                    var myRegion = rootRegion.findChildByName(regionName);
                    if (myRegion == null) {
                        myRegion = rootRegion.createChild(regionName);
                        this.regionNames.push(regionName);
                        this.regions[regionName] = myRegion;
                    } else {
                        this.regions[regionName] = myRegion;
                    }
                    return myRegion;
                } else {
                    return this.regions[regionName];
                }
            }
            return rootRegion;
        },

        downloadsReady: function (plugin, regionName, downloadItems, callback) {
            var rootRegion = this.zincHandle.context.getDefaultRegion();
            var callingObject = this;
            return function () {
                if (rootRegion) {
                    var myRegion = rootRegion;
                    if (regionName != "") {
                        myRegion = rootRegion.findChildByName(regionName);
                        if (myRegion == null) {
                            myRegion = rootRegion.createChild(regionName);
                            callingObject.regions[regionName] = myRegion;
                            callingObject.regionNames.push(regionName);
                        }
                    }
                    if(this.frameTimeVector==null){
                    	var numMeshes = downloadItems.length;
                    	var step = 1.0/(numMeshes -1.0);
                    	this.frameTimeVector = "0.0";
                    	for(var i=1;i<numMeshes;i++){
                    		this.frameTimeVector = this.frameTimeVector + "," + i*step;
                    	}
                    	if(this.debug)
                    		console.debug("Setting frametime vector to be "+this.frameTimeVector);
                    }
                    var times = this.frameTimeVector.split(",");
                    var streamInformation = myRegion.createStreamInformation();
                    this.timeFactor = downloadItems.length -1;
                    for (var i = 0; i < downloadItems.length; i++) {
                        var sHandle = streamInformation.createResourceDownloadItem(downloadItems[i]);
                        streamInformation.setResourceAttributeReal(sHandle, streamInformation.ATTRIBUTE.TIME, times[i]*this.timeFactor);
                    }
                    myRegion.read(streamInformation);
                    callingObject.mesh_loaded[regionName] = true;
                }
                callback(callingObject);
            };

        },

        imagesReady: function (regionName, downloadItems, exRegion, type, planecoordinates, callback) {
            var rootRegion = this.zincHandle.context.getDefaultRegion();
            var graphicsModule = this.zincHandle.context.getDefaultGraphicsModule();
            var callingObject = this;
            //if(this.viewParameters==null) 
            {
            	this.viewParameters = new Array(9);
                this.viewAngle = 27.0;
            	var newcoords = planecoordinates[0].split(",");
            	var x1 = parseFloat(newcoords[0]);
                var y1 = parseFloat(newcoords[1]);
                var z1 = parseFloat(newcoords[2]);
                newcoords = planecoordinates[1].split(",");
            	var x2 = parseFloat(newcoords[0]);
                var y2 = parseFloat(newcoords[1]);
                var z2 = parseFloat(newcoords[2]);
                newcoords = planecoordinates[2].split(",");
            	var x3 = parseFloat(newcoords[0]);
                var y3 = parseFloat(newcoords[1]);
                var z3 = parseFloat(newcoords[2]);
                newcoords = planecoordinates[3].split(",");
            	var x4 = parseFloat(newcoords[0]);
                var y4 = parseFloat(newcoords[1]);
                var z4 = parseFloat(newcoords[2]);
                var A = y1 * (z2 - z3) + y2 * (z3 - z1) + y3 * (z1 - z2);
                var B = z1 * (x2 - x3) + z2 * (x3 - x1) + z3 * (x1 - x2);
                var C = x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2);


                //Get the lookat point
                surfaceLookAtPoint = new Array(3);
                surfaceLookAtPoint[0] = (x1 + x2 + x3 + x4) / 4.0;
                surfaceLookAtPoint[1] = (y1 + y2 + y3 + y4) / 4.0;
                surfaceLookAtPoint[2] = (z1 + z2 + z3 + z4) / 4.0;
                //Get the upVector
                upVector = new Array(3);
                var ux = (x3 - x1);
                var uy = (y3 - y1);
                var uz = (z3 - z1);

                var uLength = Math.sqrt(ux * ux + uy * uy + uz * uz);
                //var uLength = 1.0;
                upVector[0] = ux / uLength;
                upVector[1] = uy / uLength;
                upVector[2] = uz / uLength;

                //Get eyePoint
                var snl = Math.sqrt(A * A + B * B + C * C);
                var ex = surfaceLookAtPoint[0] + A * 1000 / snl;
                var ey = surfaceLookAtPoint[1] + B * 1000 / snl;
                var ez = surfaceLookAtPoint[2] + C * 1000 / snl;
                eyePoint = [ex, ey, ez];

                //Set view parameters to the imaging plane
                this.viewParameters[0] = eyePoint[0];
                this.viewParameters[1] = eyePoint[1];
                this.viewParameters[2] = eyePoint[2];
                this.viewParameters[3] = surfaceLookAtPoint[0];
                this.viewParameters[4] = surfaceLookAtPoint[1];
                this.viewParameters[5] = surfaceLookAtPoint[2];
                this.viewParameters[6] = upVector[0];
                this.viewParameters[7] = upVector[1];
                this.viewParameters[8] = upVector[2];
            }
            
            //Determine the bounding box for all image planes loaded up to this point
            if(this.boundingbox){
            	for(var nc=0;nc<4;nc++){
                	var newcoords = planecoordinates[nc].split(",");
                    x = parseFloat(newcoords[0]);
                    y = parseFloat(newcoords[1]);
                    z = parseFloat(newcoords[2]);
                    this.boundingbox.maxx = Math.max(this.boundingbox.maxx,x);
                    this.boundingbox.minx = Math.min(this.boundingbox.minx,x);
                    this.boundingbox.maxy = Math.max(this.boundingbox.maxy,y);
                    this.boundingbox.miny = Math.min(this.boundingbox.miny,y);
                    this.boundingbox.maxz = Math.max(this.boundingbox.maxz,z);
                    this.boundingbox.minz = Math.min(this.boundingbox.minz,z);
            	}
            }else{
            	this.boundingbox = new Object();
            	var newcoords = planecoordinates[0].split(",");
                x = parseFloat(newcoords[0]);
                y = parseFloat(newcoords[1]);
                z = parseFloat(newcoords[2]);
                this.boundingbox.maxx = x;
                this.boundingbox.minx = x;
                this.boundingbox.maxy = y;
                this.boundingbox.miny = y;
                this.boundingbox.maxz = z;
                this.boundingbox.minz = z;

            	for(var nc=1;nc<4;nc++){
                	var newcoords = planecoordinates[nc].split(",");
                    x = parseFloat(newcoords[0]);
                    y = parseFloat(newcoords[1]);
                    z = parseFloat(newcoords[2]);
                    this.boundingbox.maxx = Math.max(this.boundingbox.maxx,x);
                    this.boundingbox.minx = Math.min(this.boundingbox.minx,x);
                    this.boundingbox.maxy = Math.max(this.boundingbox.maxy,y);
                    this.boundingbox.miny = Math.min(this.boundingbox.miny,y);
                    this.boundingbox.maxz = Math.max(this.boundingbox.maxz,z);
                    this.boundingbox.minz = Math.min(this.boundingbox.minz,z);
            	}          	
            }
            
            
            
            return function () {
                var myRegion = rootRegion.findChildByName(regionName);
                if (myRegion == null) {
                    myRegion = rootRegion.createChild(regionName);
                    callingObject.regions[regionName] = myRegion;
                    callingObject.regionNames.push(regionName);
                }

                var streamInformation = myRegion.createStreamInformation();
                streamInformation.createResourceDownloadItem(exRegion);
                myRegion.read(streamInformation);

                var fieldModule = myRegion.getFieldModule();
                var xiField = fieldModule.findFieldByName("xi");
                if (xiField == null) {
                    // Create coordinate field
                    fieldModule.defineField("coordinates", "finite_element num 3 coordinate comp x y z");

                    // create xi-coordinates field
                    fieldModule.defineField("xi", "xi_coordinates");
                    xiField = fieldModule.findFieldByName("xi");
                }
                //Setup the image by the plane coordinates
                if(planecoordinates.length==4){
                	var coordinatesField = fieldModule.findFieldByName("coordinates");
                    var fieldCache = fieldModule.createCache();
                    var nodeIterator = fieldModule.findNodesetByName("cmiss_nodes").createNodeIterator(); // cmiss_nodes
                    var node = nodeIterator.next();
                    var nodeMap = new Array(4);
                    if (node != undefined) {
                        while (node) {
                        	nodeMap[node.getIdentifier()-1] = node; 
                            node = nodeIterator.next();
                        }
                    }
                    var temp_array = new Array(3);
                    
                    for(var nc=0;nc<4;nc++){
                    	var newcoords = planecoordinates[nc].split(",");
                        temp_array[0] = parseFloat(newcoords[0]);
                        temp_array[1] = parseFloat(newcoords[1]);
                        temp_array[2] = parseFloat(newcoords[2]);
                        fieldCache.setNode(nodeMap[nc]);
                        coordinatesField.assignReal(fieldCache, 3, temp_array);
                    }
                    
                }
                var imageField = fieldModule.createImage(xiField);
                imageField.setName(regionName);
                slice_material = graphicsModule.createMaterial();
                slice_material.setName(regionName);
                if(this.frameTimeVector==null){
                	var numMeshes = downloadItems.length;
                	var step = 1.0/(numMeshes -1.0);
                	this.frameTimeVector = "0.0";
                	for(var i=1;i<numMeshes;i++){
                		this.frameTimeVector = this.frameTimeVector + "," + i*step;
                	}
                	if(this.debug)
                		console.debug("Setting frametime vector to be "+this.frameTimeVector);
                }
                var times = this.frameTimeVector.split(",");
                var imgStreamInformation = imageField.createStreamInformation();
                this.timeFactor = downloadItems.length -1;
                for (var imgCnt = 0; imgCnt < downloadItems.length; imgCnt++) {
                	var sHandle = imgStreamInformation.createResourceDownloadItem(downloadItems[imgCnt]);
                	streamInformation.setResourceAttributeReal(sHandle, streamInformation.ATTRIBUTE.TIME, times[imgCnt]*this.timeFactor);
                }

                if (type == "JPG")
                    imgStreamInformation.setFileFormat(imgStreamInformation.FILE_FORMAT.JPG);
                else if (type == "DCM")
                    imgStreamInformation.setFileFormat(imgStreamInformation.FILE_FORMAT.DICOM);

                imageField.read(imgStreamInformation);

                imageField.setImageAttributeReal(imageField.IMAGE_ATTRIBUTE.PHYSICAL_WIDTH_PIXELS, 1.0);
                imageField.setImageAttributeReal(imageField.IMAGE_ATTRIBUTE.PHYSICAL_HEIGHT_PIXELS, 1.0);
                imageField.setImageAttributeReal(imageField.IMAGE_ATTRIBUTE.PHYSICAL_DEPTH_PIXELS, (downloadItems.length - 1)); //19 images 0 - 18
                callingObject.maxTime = downloadItems.length - 1;
                fieldModule.defineField("time", "constant 0.0");
                fieldModule.defineField("ltexture_coordinates", "composite xi.1 xi.2 time");
                slice_material.setImageField(1, imageField);

                callingObject.regionMaterials[regionName] = slice_material;
                callingObject.images_loaded[regionName] = true;
                callback(callingObject);
            	//console.debug("Images ready for "+regionName+"\t"+callingObject);
            };
        },

        createHeartMaterials: function () {
            var graphicsModule = this.zincHandle.context.getDefaultGraphicsModule();
            var trans_brown = graphicsModule.createMaterial();
            trans_brown.setName("trans_brown");
            trans_brown.setAttributeReal3(trans_brown.ATTRIBUTE.DIFFUSE, [0.6, 0.3, 0.2]);
            trans_brown.setAttributeReal3(trans_brown.ATTRIBUTE.AMBIENT, [0.6, 0.3, 0.2]);
            trans_brown.setAttributeReal3(trans_brown.ATTRIBUTE.EMISSION, [0.0, 0.0, 0.0]);
            trans_brown.setAttributeReal3(trans_brown.ATTRIBUTE.SPECULAR, [0.0, 0.0, 0.0]);
            trans_brown.setAttributeReal(trans_brown.ATTRIBUTE.ALPHA, 0.0);
            trans_brown.setAttributeReal(trans_brown.ATTRIBUTE.SHININESS, 0.0);

            var purple = graphicsModule.createMaterial();
            purple.setName("trans_purple");
            purple.setAttributeReal3(purple.ATTRIBUTE.DIFFUSE, [0.4, 0.0, 0.9]);
            purple.setAttributeReal3(purple.ATTRIBUTE.AMBIENT, [0.4, 0.0, 0.9]);
            purple.setAttributeReal(purple.ATTRIBUTE.ALPHA, 0.3);

            var silver = graphicsModule.createMaterial();
            silver.setName("silver"); //Red
            silver.setAttributeReal3(silver.ATTRIBUTE.DIFFUSE, [1.0, 0.0, 0.0]);
            silver.setAttributeReal3(silver.ATTRIBUTE.AMBIENT, [0.5, 0.0, 0.0]);
            silver.setAttributeReal3(silver.ATTRIBUTE.SPECULAR, [0.2, 0.2, 0.2]);
            silver.setAttributeReal(silver.ATTRIBUTE.ALPHA, 1.0);
            silver.setAttributeReal(silver.ATTRIBUTE.SHININESS, 0.1);
            
            
            var red = graphicsModule.createMaterial();
            red.setName("red"); //Red
            red.setAttributeReal3(red.ATTRIBUTE.DIFFUSE, [1.0, 0.0, 0.0]);
            red.setAttributeReal3(red.ATTRIBUTE.AMBIENT, [0.5, 0.0, 0.0]);
            red.setAttributeReal3(red.ATTRIBUTE.SPECULAR, [0.2, 0.2, 0.2]);
            red.setAttributeReal(red.ATTRIBUTE.ALPHA, 1.0);
            red.setAttributeReal(red.ATTRIBUTE.SHININESS, 0.2);

        },

        loadImages: function (regionName, imageurls, exRegion, type, planecoordinates, callback) {
            var downloadManager = this.zincHandle.createDownloadManager();

            var downloadItems = new Array();
            if (typeof exregionArray == 'string') {
                downloadItems.push(downloadManager.addURI(exregionArray));
            } else {
                for (var i = 0; i < imageurls.length; i++) {
                    downloadItems.push(downloadManager.addURI(imageurls[i]));
                }
            }
            var exRegionHandle = downloadManager.addURI(exRegion);
            this.images_loaded[regionName] = false;
            //console.dir(imageurls);
            downloadManager.addCompletionCallback(this.imagesReady(regionName, downloadItems, exRegionHandle, type, planecoordinates, callback));
        },

        removeVisibleRegion: function (regionName) {
            var idx = -1;
            for (var i = 0; i < this.visibleRegion.length; i++) {
                //console.debug(regionName+"\t"+this.visibleRegion[i]);
                if (this.visibleRegion[i] == regionName) {
                    idx = i;
                    break;
                }
            }
            if (idx > -1) {
                this.visibleRegion.splice(idx, 1);
            }
        },

        loadMesh: function (regionName, exregionArray, callback) {
            var downloadManager = this.zincHandle.createDownloadManager();

            var downloadItems = new Array();
            if (typeof exregionArray == 'string') {
                downloadItems.push(downloadManager.addURI(exregionArray));
            } else {
                for (var i = 0; i < exregionArray.length; i++) {
                    downloadItems.push(downloadManager.addURI(exregionArray[i]));
                }
            }
            this.mesh_loaded[regionName] = false;
            var plugin = this.zincHandle;
            //console.debug("load mesh called");
            //console.dir(exregionArray);
            downloadManager.addCompletionCallback(this.downloadsReady(plugin, regionName, downloadItems, callback));
            //Create regions for each time point to capture landmarks
            var rootRegion = this.zincHandle.context.getDefaultRegion();
            for(var i=0;i<this.timeFactor;i++){
            	var regionName  = "Time"+i;
                var myRegion = rootRegion.createChild(regionName);
                var fieldModule = myRegion.getFieldModule();
                fieldModule.defineField("coordinates", "finite_element num 3 coordinate comp x y z");
                //Create the node points layer for nodes to be visible when created
                var sliceRendition = graphicsModule.getRendition(myRegion);
                sliceRendition.beginChange();
                sliceRendition.executeCommand("general clear");
                sliceRendition.executeCommand("node_points coordinate coordinates select_on material default glyph sphere general size 0.1 as "+regionName); 
                sliceRendition.endChange();
                this.landmarkRegions.push(myRegion);
            }
        },

        setFrameTimeVector : function (ftv){
        	this.frameTimeVector = ftv;
        },
        
        setTime: function (timex) {
        	var graphicsModule = this.zincHandle.context.getDefaultGraphicsModule();
        	var sceneViewer2 = this.zincHandle.getSceneViewerFromInteger(
                    this.zincHandle.sceneViewerIntLow,
                    this.zincHandle.sceneViewerIntHigh);
        	if(!this.rendering){
        		this.rendering = true;
	        	var time = timex*this.timeFactor;
	        	if(!timex){
	        		time = 0;
	        	}
	        	this.time = time;
	            for (item in this.regions) {
	                var blockRegion = this.regions[item];
	                if (blockRegion) {
	                    var fieldModule = blockRegion.getFieldModule();
	                    if (fieldModule) {
	                        //Change the texture coordinates
	                        fieldModule.defineField("time", "constant " + time);
	                        if (this.debug)
	                        	console.debug(item+" time constant " + time);
	                        //this.time = inc;
	                    }
	                    var floorTime = Math.floor(time);
	                    if(this.previousTime!=floorTime){//To avoid multiple calls during a play sequence
	                    	//Get the look at parameters
	                    	if(this.previousTime>-1){//Ignore the first call as the scene is not set yet
	                    		this.viewAngle = sceneViewer2.viewAngle;
	                    		sceneViewer2.getLookAtParameters(this.viewParameters);
	                    	}
	                    	console.debug("Filter change at time "+time);
	                		var new_scene = graphicsModule.createScene();
	                        new_scene.setName('SceneAtTime'+time);
	                        var rootRegion = this.zincHandle.context.getDefaultRegion();
	                        new_scene.setRegion(rootRegion); //Set the region to root
		                    //Hide/Select nodes by setting appropriate filter
	                        var graphicFilterOr = graphicsModule.createFilterOperatorOr();
	                        graphicFilterOr.appendOperand(this.activegraphicFilterOr); // The or operator is binary so chain it
	                        graphicFilterOr.appendOperand(graphicsModule.createFilterGraphicName("Time"+floorTime));
	                        new_scene.setFilter(graphicFilterOr);
	                        sceneViewer2.setScene(new_scene);
	                        sceneViewer2.viewAll();
	                        var parm = this.viewParameters;
	                        sceneViewer2.setLookAtParametersNonSkew(parm[0], parm[1], parm[2], parm[3], parm[4], parm[5], parm[6], parm[7], parm[8]);
	                        sceneViewer2.viewAngle = this.viewAngle;
	                        this.previousTime = floorTime;
	                    }
	                }
	            }
	
	            //Change the mesh coordiantes
	            var timeKeeper = this.zincHandle.context.getDefaultTimeKeeper();
	            if (timeKeeper != null)
	                timeKeeper.setAttributeReal(timeKeeper.ATTRIBUTE.TIME, time);
	            
	            this.rendering = false;
        	}
        },

        addVisibleRegion: function (regionName) {
            this.visibleRegion.push(regionName);
        },

        setViewAngle: function (vAngle) {
            this.viewAngle = vAngle;
        },

        getZincHandle: function () {
            return this.zincHandle;
        },

        getCurrentTime: function () {
            return this.time;
        },

        getMaximumTime: function () {
            return this.maxTime;
        },

        getRegionHandle: function (regionName) {
            return this.regions[regionName];
        },

        getRootRegionHandle: function () {
            return this.zincHandle.context.getDefaultRegion();
        },

        setDebug: function (state) {
            this.debug = state;
        },

        ready: function () {
            this.deferred.resolve("zincHandle created");
        },

        onShow: function () {
            if (this.debug)
            	console.log("ZinWidget onShow() " + this.zincId);
        },

        _onShow: function () {
            // summary:
            //      Called when the ContentPane is made visible
            // description:
            //      For a plain ContentPane, this is called on initialization, from startup().
            //      If the ContentPane is a hidden pane of a TabContainer etc., then it's
            //      called whenever the pane is made visible.
            //
            //      Does necessary processing, including href download and layout/resize of
            //      child widget(s)
            if (this.debug){
            	console.log("MRIZinWidget _onShow() " + this.zincId);
            }
            this.inherited(arguments);
            if (this.debug){
            	console.log("MRIZinWidget _onShow() sharedinstanceid " + this.sharedinstanceid);
            }

            // We need to wait until the zinc plugins are ready before we do anything to them, including call initialiseCmguiSharedInstance() for shared instances
            // However the npruntime version of zinc only accepts strings as arguments to onsceneviewerready and onzincready, making getting feedback of
            // those events at the right time practically impossible! It is possible to provide a string to execute but the scope of the execution of that string
            // seems to vary between Firefox and Chrome. On Chrome we seem to be able to access the scope dojo sits but not in Firefox - it shows 
            // out of native scope for most anything. 
            // Therefore in chrome we can achieve the desired result by publishing a dojo event to which we can subscribe to be notified of the onsceneviewerready or onzincready events.
            // On Firefox this trickery is not possible so we just use the zinc straight away and hope for the best, it seems to largely work work on Windows
            // the results on Linux are less predictable...
            if (this.sharedinstanceid) {
                var onsceneviewerready = "";
                var onzincready = "";
                if (dojo.isMozilla) {
                    onsceneviewerready = "";
                    onzincready = "";
                } else {
                    if(!this.debug){
                    	onsceneviewerready = "dojo.publish('/onsceneviewerready/" + this.zincId + "', ['" + this.zincId + "']);";
                    	onzincready = "dojo.publish('/onzincready/" + this.zincId + "', ['" + this.zincId + "']);";
                    }else{
                    	onsceneviewerready = "console.log('onsceneviewerready: sharedinstance'); dojo.publish('/onsceneviewerready/" + this.zincId + "', ['" + this.zincId + "']);";
                    	onzincready = "console.log('onzincready: sharedinstance'); dojo.publish('/onzincready/" + this.zincId + "', ['" + this.zincId + "']);";
                    }
                }

                this.zincHandle = domConstruct.create("object", {
                    id: this.zincId,
                    type: "application/x-zinc-plugin",
                    sharedinstanceid: this.sharedinstanceid,
                    onsceneviewerready: onsceneviewerready,
                    onzincready: onzincready,
                    style: "width: 100%; height: 100%; background-color:blue; padding: 0px",
                }, this.containerNode);
                if (dojo.isMozilla) {
                	if(this.debug){
                		console.debug("************** Calling initialiseCmguiSharedInstance **********");
                	}
                    this.zincHandle.initialiseCmguiSharedInstance();
                    this.ready();
                }

            } else {
                var onsceneviewerready = "";
                var onzincready = "";
                if (dojo.isMozilla) {
                    onsceneviewerready = ""; // "console.log('onsceneviewerready: not sharedinstance');";
                    onzincready = ""; // "console.log('onzincready: not sharedinstance');"
                } else {
                	if(!this.debug){
                		onsceneviewerready = "dojo.publish('/onsceneviewerreadyNotSharedInstance/" + this.zincId + "', ['" + this.zincId + "']);";
                    	onzincready = "dojo.publish('/onsceneviewerreadyNotSharedInstance/" + this.zincId + "', ['" + this.zincId + "']);";
                	}else{
                        onsceneviewerready = "console.log('onsceneviewerready: not sharedinstance'); dojo.publish('/onsceneviewerreadyNotSharedInstance/" + this.zincId + "', ['" + this.zincId + "']);";
                        onzincready = "console.log('onzincready: not sharedinstance'); dojo.publish('/onsceneviewerreadyNotSharedInstance/" + this.zincId + "', ['" + this.zincId + "']);";              		
                	}
                }
                this.zincHandle = domConstruct.create("object", {
                    id: this.zincId,
                    type: "application/x-zinc-plugin",
                    //onsceneviewerready: onsceneviewerready,
                    onzincready: onzincready,
                    style: "width: 100%; height: 100%; background-color:blue; padding: 0px",
                }, this.containerNode);
                if (dojo.isMozilla) {
                    this.ready();
                }
            }

        },
        

        attachSharedInstance: function (params) {
        	if (this.zincHandle) {
        		if (this.debug){
		            console.debug("Calling attach for " + params);
		            console.debug("Handle found for " + params);
        		}
	            //initialiseCmguiSharedInstance seems to cause the zinc plugin to crash
	            this.zincHandle.initialiseCmguiSharedInstance();
	            if (this.debug)
	            	console.debug("Handle initialized for " + params);
	            this.initialisedSharedInstance = true;
	            this.restartSharedInstance(params);
                if (this.debug)
                	console.log("attachSharedInstance success: ");
	            this.ready();
        	}else{
                if (this.debug)
                	console.debug(this.zincId+" context is undefined");
               	var param = this.zincId;
               	setTimeout(function(){
               			if (this.debug)
               				console.debug("Follow through for "+param);
               			topic.publish("/attachSharedInstance/" + param,params);
               	}, 10000);
            }
       },
        
        restartSharedInstance: function (params) {
            // In Chrome once a shared plugin is hidden it is destroyed, so here we
            // setup again when recreated upon display again
            if (this.initialisedSharedInstance) {
                if (this.reattachCallback) {
                    if (this.debug)
                    	console.debug("Doing reattachCallback for " + params);
                    this.reattachCallback(this);
                }
            }
        },

        resize: function () {
            if (this.zincHandle.context !== undefined) {
                if (this.debug)
                	console.log("MRIZincWidget resize");
                this.inherited(arguments);
            }else{
                if (this.debug)
                	console.log("MRIZincWidget resize deffered");
            }
        }

    });
});