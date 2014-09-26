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
        //"dojox/mobile/Button",
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
    //		ABI/app/ICMA/ZincWidget

    return declare("ABI.app.ICMA.ZincWidget", [ContentPane], {
        // summary:
        //		A zinc widget 
        // description:
        //		

        time: 0,
        maxTime: 0, //Maximum time that can be set
        parentPlugin: null, //handle to parent zinc plugin
        viewParameters: null, //Set lookup parameters for rendereing
        viewAngle: null,
        meta: null, //Any meta data that the end user might wish to store
        debug: false,
        timeoutHandle: null,
        open: true,
        showMarkers: false,
        previousTime: -1,
        initialisedSharedInstance: false,
        frameTimeVector: null,
        
        constructor: function (params) {
            dojo.mixin(this, params);

            this.regionNames = new Array(); //Names of regions 
            this.regionHasProjectedCoordinates = new Object(); //If a region's rendering uses projected coordinates - the netry is true
            this.regions = new Object(); //Handles to regions           
            this.children = new Object(); //named list of children
            this.visibleRegion = new Array(); // none = all regions
            this.regionMaterials = new Object();
            this.mesh_loaded = new Object();
            this.images_loaded = new Object();
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


        filterMyScene: function () {
            var plugin = this;
            var graphicsModule = plugin.zincHandle.context.getDefaultGraphicsModule();
            splugin = this;
            var rootRegion = this.zincHandle.context.getDefaultRegion();
            if(this.debug)
            	console.debug(splugin.visibleRegion);
            if (splugin.visibleRegion.length == 1) {
                var tokens = splugin.visibleRegion[0].split(":");
                regionName = tokens[0];
                if (regionName != null) {
                    if (plugin.regions[regionName] != null) {
                        var new_scene = graphicsModule.createScene();
                        new_scene.setName('Scene' + regionName);
                        new_scene.setRegion(rootRegion); //Set the region to root
                        /* Create filter region */
                        if (tokens.length > 1) { //Show only the requested gelements
                            var sregions = new Array();
                            for (var i = 1; i < tokens.length; i++) {
                                var gelem = tokens[i];
                                if (gelem == "projection") {
                                    gelem = "iso_scalarSurf" + tokens[0];
                                }
                                else if (gelem == "fibres")
                                    gelem = "fibres" + tokens[0];
                                else if (gelem == "surface")
                                    gelem = "surface" + tokens[0];
                                else if (gelem == "lines")
                                    gelem = "lines" + tokens[0];
                                else if (gelem == "nodes"){
                                  	this.showMarkers = true;
                                  	for(var reg=0;reg<18;reg++){
                                  		sregions.push(graphicsModule.createFilterGraphicName("Marker"+reg));
                                  	}
                                }
                                if (this.debug)
                                    console.debug("Enabling gelem " + gelem);
                                if (gelem != "nodes"){
                                	sregions.push(graphicsModule.createFilterGraphicName(gelem));
                                }
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
                            new_scene.setFilter(graphicFilterOr);
                        } else {
                            if (this.debug)
                                console.debug("Enabling region " + tokens[0]);
                            //new_scene.setRegion(plugin.regions[regionName]); //Set the region to the parent
                            //var graphicFilter1 = graphicsModule.createFilterVisibilityFlags();
                            //new_scene.setFilter(graphicFilter1);
                        }


                        var sceneViewer2 = plugin.zincHandle.getSceneViewerFromInteger(
                            splugin.zincHandle.sceneViewerIntLow,
                            splugin.zincHandle.sceneViewerIntHigh);
                        sceneViewer2.setScene(new_scene);
                        //Dont call view all as it resets view port
                    } else {
                        console.debug("Region " + regionName + " not found!");
                        console.debug(this.regionNames);
                        var new_scene = graphicsModule.createScene();
                        new_scene.setName('Scene' + regionName);
                        new_scene.setRegion(rootRegion); //Set the region to root
                        new_scene.setFilter(graphicsModule.createFilterGraphicName("BLANK"));
                        var sceneViewer2 = plugin.zincHandle.getSceneViewerFromInteger(
                            splugin.zincHandle.sceneViewerIntLow,
                            splugin.zincHandle.sceneViewerIntHigh);
                        sceneViewer2.setScene(new_scene);
                    }
                }
            } else if (splugin.visibleRegion.length > 1) {
                var sregions = new Array();
                var scenename = "Scene";
                for (var i = 0; i < splugin.visibleRegion.length; i++) {
                    var tokens = splugin.visibleRegion[i].split(":");
                    if (plugin.regions[tokens[0]] != undefined || plugin.regions[tokens[0]] != null) {
                        //Check if it is just the region or a list of graphic elements

                        if (tokens.length == 1) {
                            if (this.debug)
                                console.debug("Enabling region " + tokens[0]);
                            //sregions.push(graphicsModule.createFilterRegion(plugin.regions[tokens[0]]));
                        } else {
                            //sregions.push( graphicsModule.createFilterRegion(plugin.regions[tokens[0]]));
                            for (var ge = 1; ge < tokens.length; ge++) {
                                var gelem = tokens[ge];
                                if (gelem == "projection") {
                                    gelem = "iso_scalarSurf" + tokens[0];
                                }
                                if (gelem == "fibres")
                                    gelem = "fibres" + tokens[0];
                                if (gelem == "surface")
                                    gelem = "surface" + tokens[0];
                                if (gelem == "lines")
                                    gelem = "lines" + tokens[0];
                                if (gelem == "nodes"){
                                  	this.showMarkers = true;
                                  	for(var reg=0;reg<18;reg++){
                                  		sregions.push(graphicsModule.createFilterGraphicName("Marker"+reg));
                                  	}
                                }
                                if (this.debug)
                                    console.debug("Enabling gelem " + gelem);
                                if (gelem != "nodes"){
                                	sregions.push(graphicsModule.createFilterGraphicName(gelem));
                                }
                            }
                        }
                        scenename += tokens[0];
                    }
                }
                if (scenename == "Scene")
                    scenename += Math.random();
                var new_scene = graphicsModule.createScene();
                new_scene.setName(scenename);
                new_scene.setRegion(rootRegion); //Use the root region

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
                new_scene.setFilter(graphicFilterOr);

                var sceneViewer2 = plugin.zincHandle.getSceneViewerFromInteger(
                    splugin.zincHandle.sceneViewerIntLow,
                    splugin.zincHandle.sceneViewerIntHigh);
                sceneViewer2.setScene(new_scene);
                //Dont call view all as it resets view port
            }
        },

        filterScene: function () { //Called by parent
            var plugin = this;

            var graphicsModule = plugin.zincHandle.context.getDefaultGraphicsModule();
            var children = new Object();
            for (item in this.children) {
                children[item] = this.children[item];
            }

            children[""] = this;

            var rootRegion = this.zincHandle.context.getDefaultRegion();
            for (item in children) {
                splugin = children[item];
                if (this.debug)
                    console.debug(item + " " + splugin.visibleRegion);
                if (splugin.visibleRegion.length == 1) {
                    var tokens = splugin.visibleRegion[0].split(":");
                    regionName = tokens[0];
                    if (regionName != null) {
                        if (plugin.regions[regionName] != null) {
                            var new_scene = graphicsModule.createScene();
                            new_scene.setName('Scene' + regionName);
                            new_scene.setRegion(rootRegion); //Set the region to root
                            /* Create filter region */
                            if (tokens.length > 1) { //Show only the requested gelements
                                var sregions = new Array();
                                for (var i = 1; i < tokens.length; i++) {
                                    var gelem = tokens[i];
                                    if (gelem == "projection") {
                                        gelem = "iso_scalarSurf" + tokens[0];
                                    }
                                    if (gelem == "fibres")
                                        gelem = "fibres" + tokens[0];
                                    if (gelem == "surface")
                                        gelem = "surface" + tokens[0];
                                    if (gelem == "lines")
                                        gelem = "lines" + tokens[0];
                                    if (gelem == "nodes"){
                                      	this.showMarkers = true;
                                      	for(var reg=0;reg<18;reg++){
                                      		sregions.push(graphicsModule.createFilterGraphicName("Marker"+reg));
                                      	}
                                    }
                                    if (this.debug)
                                        console.debug("Enabling gelem " + gelem);
                                    if (gelem != "nodes"){
                                    	sregions.push(graphicsModule.createFilterGraphicName(gelem));
                                    }
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
                                new_scene.setFilter(graphicFilterOr);
                            } else {
                                if (this.debug)
                                    console.debug("Enabling region " + tokens[0]);
                                //new_scene.setRegion(plugin.regions[regionName]); //Set the region to the parent
                                //var graphicFilter1 = graphicsModule.createFilterVisibilityFlags();
                                //new_scene.setFilter(graphicFilter1);
                            }

                            var sceneViewer2 = plugin.zincHandle.getSceneViewerFromInteger(
                                splugin.zincHandle.sceneViewerIntLow,
                                splugin.zincHandle.sceneViewerIntHigh);
                            sceneViewer2.setScene(new_scene);
                            sceneViewer2.viewAll();

                            if (splugin.viewParameters != null) {
                                var parm = splugin.viewParameters;
                                sceneViewer2.setLookAtParametersNonSkew(parm[0], parm[1], parm[2], parm[3], parm[4], parm[5], parm[6], parm[7], parm[8]);
                            }
                            if (splugin.viewAngle) {
                                sceneViewer2.viewAngle = splugin.viewAngle * 3.14 / 180;
                            }
                        }
                    } else {
                        var new_scene = graphicsModule.createScene();
                        new_scene.setName('Scene' + regionName);
                        new_scene.setRegion(rootRegion); //Set the region to root
                        new_scene.setFilter(graphicsModule.createFilterGraphicName("BLANK"));
                        var sceneViewer2 = plugin.zincHandle.getSceneViewerFromInteger(
                            splugin.zincHandle.sceneViewerIntLow,
                            splugin.zincHandle.sceneViewerIntHigh);
                        sceneViewer2.setScene(new_scene);
                    }
                } else if (splugin.visibleRegion.length > 1) {
                    var sregions = new Array();
                    var scenename = "Scene";
                    for (var i = 0; i < splugin.visibleRegion.length; i++) {
                        var tokens = splugin.visibleRegion[i].split(":");
                        if (plugin.regions[tokens[0]] != undefined || plugin.regions[tokens[0]] != null) {
                            //Check if it is just the region or a list of graphic elements
                            if (tokens.length == 1) {
                                if (this.debug)
                                    console.debug("Enabling region " + tokens[0]);
                                //sregions.push(graphicsModule.createFilterRegion(plugin.regions[tokens[0]]));
                            } else {
                                //sregions.push( graphicsModule.createFilterRegion(plugin.regions[tokens[0]]));
                                for (var ge = 1; ge < tokens.length; ge++) {
                                    var gelem = tokens[ge];
                                    if (gelem == "projection") {
                                        gelem = "iso_scalarSurf" + tokens[0];
                                    }
                                    if (gelem == "fibres")
                                        gelem = "fibres" + tokens[0];
                                    if (gelem == "surface")
                                        gelem = "surface" + tokens[0];
                                    if (gelem == "lines")
                                        gelem = "lines" + tokens[0];
                                    if (gelem == "nodes"){
                                      	this.showMarkers = true;
                                      	for(var reg=0;reg<18;reg++){
                                      		sregions.push(graphicsModule.createFilterGraphicName("Marker"+reg));
                                      	}
                                    }
                                    if (this.debug)
                                        console.debug("Enabling gelem " + gelem);
                                    if (gelem != "nodes"){
                                    	sregions.push(graphicsModule.createFilterGraphicName(gelem));
                                    }
                                }
                            }
                            scenename += tokens[0];
                        }
                    }
                    if (scenename == "Scene")
                        scenename += Math.random();

                    var new_scene = graphicsModule.createScene();
                    new_scene.setName(scenename);
                    new_scene.setRegion(rootRegion); //Use the root region

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
                    new_scene.setFilter(graphicFilterOr);

                    var sceneViewer2 = plugin.zincHandle.getSceneViewerFromInteger(
                        splugin.zincHandle.sceneViewerIntLow,
                        splugin.zincHandle.sceneViewerIntHigh);
                    sceneViewer2.setScene(new_scene);
                    sceneViewer2.viewAll();
                    if (splugin.viewParameters != null) {
                        var parm = splugin.viewParameters;
                        //console.debug("Setting view Parameters "+parm);
                        sceneViewer2.setLookAtParametersNonSkew(parm[0], parm[1], parm[2], parm[3], parm[4], parm[5], parm[6], parm[7], parm[8]);
                        
        			    /*var lookup = new Array(9);
        			    sceneViewer2.getLookAtParameters(lookup);*/
                        
                    }
                    if (splugin.viewAngle) { //Should be greater than 0 and less than PI
                        sceneViewer2.viewAngle = splugin.viewAngle * 3.14 / 180;
                    }
                }
            }
        },

        prolateToRC: function (regionName, coordinate, targetName) {
            var item = this.regions[regionName];
            if (item != undefined) {
                var fieldModule = item.getFieldModule();
                var coordinateField = fieldModule.findFieldByName(targetName);
                if (!coordinateField) {
                    //Create rc_coordinate field
                    fieldModule.defineField(targetName, "coordinate_transform field " + coordinate);
                }
            }
        },

        addChild: function (childName, wHandle) {
            this.children[childName] = wHandle;
            wHandle.parentPlugin = this;
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

            if (this.viewParameters == null || this.viewParameters.length > 9) {
                this.computeModelViewParameters();
            }

            var parm = this.viewParameters;
            var res = sceneViewer2.setLookAtParametersNonSkew(parm[0], parm[1], parm[2], parm[3], parm[4], parm[5], parm[6], parm[7], parm[8]);

            if (this.viewAngle)
                sceneViewer2.viewAngle = this.viewAngle * 3.14 / 180;

            for (item in this.children) {
                if (this.images_loaded[item] != null) { //Ensure that images for the child are loaded, else would end up showing a mesh
                    var splugin = this.children[item];
                    var sceneViewerC = this.zincHandle.getSceneViewerFromInteger(
                        splugin.zincHandle.sceneViewerIntLow,
                        splugin.zincHandle.sceneViewerIntHigh);
                    sceneViewerC.viewAll();
                    if (splugin.viewParameters != null) {
                        var parm = splugin.viewParameters;
                        var res = sceneViewerC.setLookAtParametersNonSkew(parm[0], parm[1], parm[2], parm[3], parm[4], parm[5], parm[6], parm[7], parm[8]);
                    }
                    if (splugin.viewAngle)
                        sceneViewerC.viewAngle = splugin.viewAngle * 3.14 / 180;
                }
            }
        },

        computeModelViewParameters: function () {
            //Set the view parameters of the first visible child to be the view parameters of zincwidget
            for (var i = 0; i < this.visibleRegion.length; i++) {
                var rn = this.visibleRegion[i].split(":");

                if (rn[0] != "heart" && this.children[rn[0]] != null) {
                    this.viewParameters = this.children[rn[0]].viewParameters;
                    this.viewAngle = this.children[rn[0]].viewAngle;
                    if (this.debug)
                        console.debug("Setting visible region to " + rn[0] + "\t" + this.viewParameters);
                    break;
                }
            }
            if (this.viewParameters == undefined)
                this.setViewParameters('1125 1247 -1718 400 300 -6 0.47 -0.83 -0.26');
            if (this.viewAngle == undefined)
                this.viewAngle = 50.0;
        },

        initializeChildren: function () {
            for (item in this.children) {
                try {
                    var zHandle = this.children[item].zincHandle;
                    zHandle.initialiseCmguiSharedInstance();
                } catch (e) {
                    console.debug(e);
                }
            }
        },

        clearDisplay: function () {
            var graphicsModule = this.zincHandle.context.getDefaultGraphicsModule();
            for (item in this.regions) {
                var sliceRendition = graphicsModule.getRendition(item);
                sliceRendition.beginChange();
                sliceRendition.executeCommand("general clear");
                sliceRendition.endChange();
            }
        },

        clear: function (regionName) {
            var graphicsModule = this.zincHandle.context.getDefaultGraphicsModule();
            var item = this.regions[regionName];
            if (item != undefined) {
                var sliceRendition = graphicsModule.getRendition(item);
                sliceRendition.beginChange();
                sliceRendition.executeCommand("general clear");
                sliceRendition.endChange();
            }
        },

        showNodePoints: function (regionName, glyph, size, color, proj) {
            var graphicsModule = this.zincHandle.context.getDefaultGraphicsModule();
            item = this.regions[regionName];
            var coordinates = "coordinates";
            if (proj && this.regionHasProjectedCoordinates[regionName])
                coordinates = "proj_coordinates";
            if (item != undefined) {
                var sliceRendition = graphicsModule.getRendition(item);
                sliceRendition.beginChange();
                // The node points are named as regionName_np
                sliceRendition
                    .executeCommand("node_points coordinate " + coordinates + " select_on material " + regionName + " as EchoMarkers glyph " + glyph + " size " + size + " material " + color);

                sliceRendition.endChange();
            }
        },

        setupLines: function (regionName, linesize, color, proj) {
            var graphicsModule = this.zincHandle.context.getDefaultGraphicsModule();
            var size = 1;
            var col = 'default_selected';
            if (linesize != "") {
                size = linesize;
            }
            if (color != "")
                col = color;
            var coordinates = "coordinates";
            if (proj && this.regionHasProjectedCoordinates[regionName])
                coordinates = "proj_coordinates";
            item = this.regions[regionName];
            if (item != undefined) {
                var gfxcmd = "lines coordinate " + coordinates + " tessellation default LOCAL select_on invisible material default selected_material " + col + " line_width " + size + " as lines" + regionName + " ";
                if (this.debug)
                    console.debug("Enabling gelem lines" + regionName);
                if (arguments.length > 4)
                    gfxcmd = gfxcmd + " " + arguments[4];
                var sliceRendition = graphicsModule.getRendition(item);
                sliceRendition.beginChange();
                sliceRendition.executeCommand(gfxcmd);
                sliceRendition.endChange();
            }
            //Use this to show measure lines
            //Get the planes

        },

        setupSurfaces: function (regionName, proj) {
            var graphicsModule = this.zincHandle.context.getDefaultGraphicsModule();
            var coordinates = "coordinates";
            if (proj && this.regionHasProjectedCoordinates[regionName])
                coordinates = "proj_coordinates";
            item = this.regions[regionName];
            if (item != undefined) {
                if (arguments.length > 2) {
                    if (arguments[2].indexOf("longaxisstrain") != -1) {
                        var spectrum = graphicsModule.findSpectrumByName("longaxisstrain");
                        if (spectrum == undefined || spectrum == null) {
                            spectrum = graphicsModule.createSpectrum();
                            spectrum.setName("longaxisstrain");
                            //spectrum.setManaged(1);
                            spectrum.setAttributeInteger(spectrum.ATTRIBUTE.IS_MANAGED, 1.0);
                            spectrum.executeCommand("clear overwrite_colour");
                            spectrum.executeCommand("linear range -20 0 extend_above extend_below rainbow colour_range 0 1 component 1");
                            //this.setTime(this.getMaximumTime()/2.0);//This will ensure it covers the range properly
                            //spectrum.executeCommand("autorange");
                        }
                    }
                }

                var sliceRendition = graphicsModule.getRendition(item);
                sliceRendition.beginChange();
                if (arguments.length == 2) {
                    sliceRendition.executeCommand("surfaces coordinate " + coordinates + " face xi3_0 select_on invisible material " + regionName + " texture_coordinates ltexture_coordinates selected_material " + regionName + " as surface" + regionName);
                } else if (arguments.length > 2) {
                    sliceRendition.executeCommand("surfaces coordinate " + coordinates + " " + arguments[2] + " as surface" + regionName);
                }
                if (this.debug)
                    console.debug("Enabling gelem surface" + regionName);
                sliceRendition.endChange();
            }
        },

        setupFibres: function (regionName, proj) {
            var graphicsModule = this.zincHandle.context.getDefaultGraphicsModule();
            var coordinates = "coordinates";
            if (proj && this.regionHasProjectedCoordinates[regionName])
                coordinates = "proj_coordinates";
            item = this.regions[regionName];
            if (item != undefined) {
                //Create the deformed_fibre_axes field
                /*		    	var fieldModule = item.getFieldModule();
		    	var fibre_axes = fieldModule.findFieldByName("deformed_fibre_axes");
		    	if(!fibre_axes){
		    		
		    		if(!fieldModule.findFieldByName("rc_reference_coordinates"))
		    			fieldModule.defineField("rc_reference_coordinates","coordinate_transform field reference_coordinates");
		    		//gfx define field F gradient coordinate rc_reference_coordinates field rc_coordinates
		    		//gfx define field F_transpose transpose source_number_of_rows 3 field F
		    		if(!fieldModule.findFieldByName("F")){
		    			fieldModule.defineField("F","gradient coordinate rc_reference_coordinates field rc_coordinates");
		    			fieldModule.defineField("F_transpose","transpose source_number_of_rows 3 field F");
		    		}
		    		//gfx define field fibre_axes fibre_axes coordinate rc_reference_coordinates fibre reference_fibres
		    		//gfx define field deformed_fibre_axes matrix_multiply number_of_rows 3 fields fibre_axes F_transpose
		    		fieldModule.defineField("fibre_axes","fibre_axes coordinate rc_reference_coordinates fibre reference_fibres");
		    		fieldModule.defineField("deformed_fibre_axes","matrix_multiply number_of_rows 3 fields fibre_axes F_transpose");
		    		//Reference fiber field is in degrees convert it to radians
		    		//gfx def field scaled_reference_fibres scale field reference_fibres scale_factors 3.14159/180 3.14159/180 3.14159/180
		    		//fieldModule.defineField("deformed_fibre_axes","scale field reference_fibres scale_factors 3.14159/180 3.14159/180 3.14159/180");
		    		fieldModule.defineField("deformed_fibre_axes","scale field reference_fibres scale_factors 1 1 1");
		    	}
		        
		        var sliceRendition = graphicsModule.getRendition(item);
		        sliceRendition.beginChange();
		        if(arguments.length==2){
		            sliceRendition.executeCommand("streamlines coordinate "+coordinates+" discretization \"2*2*2\" tessellation NONE LOCAL cell_centres ribbon vector deformed_fibre_axes length 100 width 5 no_data select_on material silver selected_material default_selected"+" as fibres"+regionName);
		        }else if(arguments.length>2){
		            sliceRendition.executeCommand("streamlines coordinate "+coordinates+" discretization \"2*2*2\" tessellation NONE LOCAL cell_centres ribbon vector deformed_fibre_axes length 100 width 5 no_data select_on material silver selected_material default_selected"+arguments[2]+" as fibres"+regionName);
		        }
		        if(this.debug)
		            console.debug("Enabling gelem fibres"+regionName);
		        sliceRendition.endChange(); */
                var sliceRendition = graphicsModule.getRendition(item);
                sliceRendition.beginChange();
                if (arguments.length == 2) {
                    sliceRendition.executeCommand("streamlines coordinate " + coordinates + " discretization \"2*2*1\" tessellation NONE LOCAL cell_centres ribbon vector fibres length 100 width 5 no_data select_on material silver selected_material default_selected" + " as fibres" + regionName);
                } else if (arguments.length > 2) {
                    sliceRendition.executeCommand("streamlines coordinate " + coordinates + " discretization \"2*2*1\" tessellation NONE LOCAL cell_centres ribbon vector fibres length 100 width 5 no_data select_on material silver selected_material default_selected" + arguments[2] + " as fibres" + regionName);
                }
                if (this.debug)
                    console.debug("Enabling gelem fibres" + regionName);
                sliceRendition.endChange();
            }
        },


        setupViewProjection: function (regionName) {
            //Get the image plane coordinates for that mesh
            var coordinates = "coordinates";
            if (this.regionHasProjectedCoordinates[regionName])
                coordinates = "proj_coordinates";
            if (this.regions[regionName] != undefined) {
                var fieldModule = this.regions[regionName].getFieldModule();
                var fieldCache = fieldModule.createCache();
                var coordinatesField = fieldModule.findFieldByName(coordinates);
                var nodeIterator = fieldModule.findNodesetByName("cmiss_nodes").createNodeIterator(); // cmiss_nodes
                var node = nodeIterator.next();
                var nodeMap = new Array();
                if (node != undefined) {
                    var temp_array = new Array();
                    temp_array[0] = 0.0;
                    temp_array[1] = 0.0;
                    temp_array[2] = 0.0;
                    while (node) {
                        fieldCache.setNode(node);
                        coordinatesField.evaluateReal(fieldCache, 3, temp_array);
                        nodeMap.push([temp_array[0], temp_array[1], temp_array[2]]);
                        node = nodeIterator.next();
                    }
                }

                if (nodeMap.length > 2) {
                    //Determine the equation of the plane from these values
                    var x1 = nodeMap[0][0];
                    var y1 = nodeMap[0][1];
                    var z1 = nodeMap[0][2];
                    var x2 = nodeMap[1][0];
                    var y2 = nodeMap[1][1];
                    var z2 = nodeMap[1][2];
                    var x3 = nodeMap[2][0];
                    var y3 = nodeMap[2][1];
                    var z3 = nodeMap[2][2];
                    var x4 = nodeMap[3][0];
                    var y4 = nodeMap[3][1];
                    var z4 = nodeMap[3][2];
                    var A = y1 * (z2 - z3) + y2 * (z3 - z1) + y3 * (z1 - z2);
                    var B = z1 * (x2 - x3) + z2 * (x3 - x1) + z3 * (x1 - x2);
                    var C = x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2);

                    //Get the surface normal
                    surfaceNormal = [A, B, C];
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
                    if (this.children[regionName] != null) {
                        if (this.children[regionName].viewParameters == null) {
                            var viewParameters = new Array();
                            viewParameters[0] = eyePoint[0];
                            viewParameters[1] = eyePoint[1];
                            viewParameters[2] = eyePoint[2];
                            viewParameters[3] = surfaceLookAtPoint[0];
                            viewParameters[4] = surfaceLookAtPoint[1];
                            viewParameters[5] = surfaceLookAtPoint[2];
                            viewParameters[6] = upVector[0];
                            viewParameters[7] = upVector[1];
                            viewParameters[8] = upVector[2];
                            this.children[regionName].viewParameters = viewParameters;
                            //console.debug(regionName+" view projection "+this.children[regionName].viewParameters);
                        }
                        //Angle opposite to the diagonal of the largest square that forms the 
                        //view. The eyepoint and the diagonal endpoints form the triangle
                        //this.children[regionName].setViewAngle(52.0);
                        this.children[regionName].setViewAngle(50.0);
                    }
                }
            }
        },


        setupMeshProjection: function (meshregion, regionName) {
            var graphicsModule = this.zincHandle.context.getDefaultGraphicsModule();
            //Get the image plane coordinates for that mesh
            var coordinates = "coordinates";
            if (this.regionHasProjectedCoordinates[regionName])
                coordinates = "proj_coordinates";
            if (this.regions[regionName] != undefined) {
                //Check if spectrum exists, if not create it
                var spectrum = graphicsModule.findSpectrumByName("longaxisstrain");
                if (spectrum == undefined || spectrum == null) {
                    spectrum = graphicsModule.createSpectrum();
                    spectrum.setName("longaxisstrain");
                    //spectrum.setManaged(1);
                    spectrum.setAttributeInteger(spectrum.ATTRIBUTE.IS_MANAGED, 1.0);
                    spectrum.executeCommand("clear overwrite_colour");
                    spectrum.executeCommand("linear range -0.18 0.1 extend_above extend_below rainbow colour_range 0 1 component 1");
                    //this.setTime(this.getMaximumTime()/2.0);//This will ensure it covers the range properly
                    //spectrum.executeCommand("autorange");
                }

                var fieldModule = this.regions[regionName].getFieldModule();
                var fieldCache = fieldModule.createCache();

                var coordinatesField = fieldModule.findFieldByName(coordinates);
                var nodeIterator = fieldModule.findNodesetByName("cmiss_nodes").createNodeIterator(); // cmiss_nodes
                var node = nodeIterator.next();
                var nodeMap = new Array();
                if (node != undefined) {
                    var temp_array = new Array();
                    temp_array[0] = 0.0;
                    temp_array[1] = 0.0;
                    temp_array[2] = 0.0;
                    while (node) {
                        fieldCache.setNode(node);
                        coordinatesField.evaluateReal(fieldCache, 3, temp_array);
                        nodeMap.push([temp_array[0], temp_array[1], temp_array[2]]);
                        node = nodeIterator.next();
                    }
                }
                //Create the isosurface in the mesh region
                coordinates = "coordinates";
                if (this.regionHasProjectedCoordinates[meshregion])
                    coordinates = "proj_coordinates";

                if (nodeMap.length > 2 && this.regions[meshregion] != undefined) {
                    var fieldModule = this.regions[meshregion].getFieldModule();
                    //Determine the equation of the plane from these values
                    var x1 = nodeMap[0][0];
                    var y1 = nodeMap[0][1];
                    var z1 = nodeMap[0][2];
                    var x2 = nodeMap[1][0];
                    var y2 = nodeMap[1][1];
                    var z2 = nodeMap[1][2];
                    var x3 = nodeMap[2][0];
                    var y3 = nodeMap[2][1];
                    var z3 = nodeMap[2][2];
                    var A = y1 * (z2 - z3) + y2 * (z3 - z1) + y3 * (z1 - z2);
                    var B = z1 * (x2 - x3) + z2 * (x3 - x1) + z3 * (x1 - x2);
                    var C = x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2);
                    var D = -(x1 * (y2 * z3 - y3 * z2) + x2 * (y3 * z1 - y1 * z3) + x3 * (y1 * z2 - y2 * z1));

                    var delta = 1.0e+6;
                    //console.debug("iso_scalar"+regionName,"coordinate_system rectangular_cartesian dot_product fields "+coordinates+" \"[" +A+ " " +B+" "+C+"]\"");
                    fieldModule.defineField("iso_scalar" + regionName, "coordinate_system rectangular_cartesian dot_product fields " + coordinates + " \"[" + A + " " + B + " " + C + "]\"");
                    var sliceRendition = graphicsModule.getRendition(this.regions[meshregion]);
                    sliceRendition.beginChange();
                    //console.debug("iso_surface coordinate "+coordinates+" exterior face xi3_0 iso_scalar iso_scalar"+regionName+" iso_value " + D +" use_faces no_select line_width 5 as iso_scalarSurf"+regionName);
                    //sliceRendition.executeCommand("iso_surface coordinate "+coordinates+" exterior face xi3_0 iso_scalar iso_scalar"+regionName+" iso_value " + (-D) +" use_faces no_select invisible line_width 5 as iso_scalarSurf"+regionName);
                    sliceRendition.executeCommand("iso_surfaces as iso_scalarSurf" + regionName + " coordinate " + coordinates + " tessellation default LOCAL iso_scalar iso_scalar" + regionName + " range_number_of_iso_values 2 first_iso_value " + (-D - delta) + " last_iso_value " + (-D + delta) + " use_elements no_select material default data LAGRANGIAN spectrum longaxisstrain selected_material default_selected render_shaded");
                    if (this.debug)
                        console.debug("Enabling gelem iso_scalarSurf" + regionName);
                    sliceRendition.endChange();
                }
                this.setupViewProjection(regionName);
                this.viewAll();
            }

        },

        projectRegionCoordinates: function (regionName, coordinateName, transformationMatrix) {
            if (this.regions[regionName] != undefined) {
                var fieldModule = this.regions[regionName].getFieldModule();
                var coeff = 'constant ' + transformationMatrix;
                fieldModule.defineField("trans_coeff", coeff);
                fieldModule.defineField("proj_coordinates", "projection field " + coordinateName + " projection_matrix trans_coeff");
                this.regionHasProjectedCoordinates[regionName] = true;
                this.setViewParameters(transformationMatrix);
            }
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

            //var rootRegion = this.zincHandle.context.getDefaultRegion();
            //var callingObject = this;   //The object is not recognized in the callback as this so make a reference
            /*		    return lang.hitch( this, function() {
		    	console.log("Downloads Ready");
		    	var rootRegion = this.zincHandle.context.getDefaultRegion();
		        if (rootRegion) {
		            var myRegion = rootRegion;
		            if(regionName != ""){
		                myRegion = rootRegion.findChildByName(regionName);
		                if(myRegion==null){
		                    myRegion = rootRegion.createChild(regionName);
		                    callingObject.regions[regionName] = myRegion;
		                    callingObject.regionNames.push(regionName);
		                }
		            }
		            var streamInformation = myRegion.createStreamInformation();
		            for(var i=0;i<downloadItems.length;i++){
		                var sHandle = streamInformation.createResourceDownloadItem(downloadItems[i]);
		                streamInformation.setResourceAttributeReal(sHandle, streamInformation.ATTRIBUTE.TIME, i);
		            }
		            myRegion.read(streamInformation);
                    this.mesh_loaded[regionName] = true;
                    //callback(callingObject);
                    callback(this);
		        }
		    });*/

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
                    var factor = downloadItems.length -1;
                    for (var i = 0; i < downloadItems.length; i++) {
                        var sHandle = streamInformation.createResourceDownloadItem(downloadItems[i]);
                        streamInformation.setResourceAttributeReal(sHandle, streamInformation.ATTRIBUTE.TIME, times[i]*factor);
                    }
                    myRegion.read(streamInformation);
                    callingObject.mesh_loaded[regionName] = true;
                }
                callback(callingObject);
            };

        },

        imagesReady: function (regionName, downloadItems, exRegion, type, callback) {
            var rootRegion = this.zincHandle.context.getDefaultRegion();
            var graphicsModule = this.zincHandle.context.getDefaultGraphicsModule();
            var callingObject = this;
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
                var factor = downloadItems.length -1;
                for (var imgCnt = 0; imgCnt < downloadItems.length; imgCnt++) {
                	var sHandle = imgStreamInformation.createResourceDownloadItem(downloadItems[imgCnt]);
                	streamInformation.setResourceAttributeReal(sHandle, streamInformation.ATTRIBUTE.TIME, times[imgCnt]*factor);
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
            silver.setName("silver"); //Silver
            silver.setAttributeReal3(silver.ATTRIBUTE.DIFFUSE, [0.7, 0.7, 0.7]);
            silver.setAttributeReal3(silver.ATTRIBUTE.AMBIENT, [0.4, 0.4, 0.4]);
            silver.setAttributeReal3(silver.ATTRIBUTE.SPECULAR, [0.5, 0.5, 0.5]);
            silver.setAttributeReal(silver.ATTRIBUTE.ALPHA, 1.0);
            silver.setAttributeReal(silver.ATTRIBUTE.SHININESS, 0.3);
            
            
            var red = graphicsModule.createMaterial();
            red.setName("red"); //Red
            red.setAttributeReal3(red.ATTRIBUTE.DIFFUSE, [1.0, 0.0, 0.0]);
            red.setAttributeReal3(red.ATTRIBUTE.AMBIENT, [0.5, 0.0, 0.0]);
            red.setAttributeReal3(red.ATTRIBUTE.SPECULAR, [0.2, 0.2, 0.2]);
            red.setAttributeReal(red.ATTRIBUTE.ALPHA, 1.0);
            red.setAttributeReal(red.ATTRIBUTE.SHININESS, 0.2);

        },

        loadImages: function (regionName, imageurls, exRegion, type, callback) {
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
            downloadManager.addCompletionCallback(this.imagesReady(regionName, downloadItems, exRegionHandle, type, callback));
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
        },

        setFrameTimeVector : function (ftv){
        	this.frameTimeVector = ftv;
        },
        
        setTime: function (timex) {

        	var time = timex;
        	if(!timex){
        		time = 0;
        	}
            for (item in this.regions) {
                var blockRegion = this.regions[item];
                if (blockRegion) {
                    var fieldModule = blockRegion.getFieldModule();
                    if (fieldModule) {
                        //Change the texture coordinates
                        fieldModule.defineField("time", "constant " + time);
                        //this.time = inc;
                    }
                }
            }
            //If markers should be shown, select the appropriate one
            if (this.showMarkers){
                var curRegion = (Math.round(time))%18;
                if(this.previousTime!=curRegion){
                	var nodeRegion = "Marker"+curRegion;
                	var rootRegion = this.zincHandle.context.getDefaultRegion();
                	var heartRegion = rootRegion.findChildByName("heart");
                    var myRegion = heartRegion.findChildByName(nodeRegion);
                	var graphicsModule = this.zincHandle.context.getDefaultGraphicsModule();
                    if(myRegion){
                    	if(this.previousTime>-1){
                    		var prevRegionName = "Marker"+this.previousTime;
                    		var prevRegion = heartRegion.findChildByName(prevRegionName);
    		                var sliceRendition = graphicsModule.getRendition(prevRegion);	                
    		                sliceRendition.beginChange();
   		                	var graphic  = sliceRendition.findGraphicByName(prevRegionName);
   		                	sliceRendition.removeGraphic(graphic);
    		                sliceRendition.endChange();
                    	}
                    	{
			                var sliceRendition = graphicsModule.getRendition(myRegion);	                
			                sliceRendition.beginChange();
			                sliceRendition.executeCommand("node_points coordinate coordinates LOCAL glyph sphere general size \"10*10*10\" centre 0,0,0 font default select_on material red selected_material default_selected as "+nodeRegion);
			                sliceRendition.endChange();
			                this.previousTime=curRegion;
                    	}
                    }
                }
            }
            
            //Print lookup parameters
            /*		    console.debug("Model  "+this.getLookAtParameters());
		    for(item in this.children){
				//this.children[item].setTime(inc);
				console.debug(item+"  "+this.children[item].getLookAtParameters());
			}*/

            //Change the mesh coordiantes
            var timeKeeper = this.zincHandle.context.getDefaultTimeKeeper();
            if (timeKeeper != null)
                timeKeeper.setAttributeReal(timeKeeper.ATTRIBUTE.TIME, time);

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
            	console.log("ZinWidget _onShow() " + this.zincId);
            }
            this.inherited(arguments);
            if (this.debug){
            	console.log("ZinWidget _onShow() sharedinstanceid " + this.sharedinstanceid);
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
                	console.log("ZincWidget resize");
                this.inherited(arguments);
            }else{
                if (this.debug)
                	console.log("ZincWidget resize deffered");
            }
        }

    });
});