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
], function(lang,declare, domConstruct, has, Contained, WidgetBase, Button,HorizontalSlider){

	// module:
	//		ABI/lib/Widgets/VideoPlayer

	return declare("ABI.lib.Widgets.VideoPlayer", [WidgetBase, Contained],{
		// summary:
		//		A thin wrapper around the HTML5 `<video>` element.
		
		// source: Array
		//		An array of src and type,
		//		ex. [{src:"a.webm",type:"video/mpeg"},{src:"a.ogg",type:"video/ogg"},...]
		//		The src gives the path of the media resource. The type gives the
		//		type of the media resource.
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

		constructor: function(){
			// summary:
			//		Creates a new instance of the class.
			this.source = [];
		},

		buildRendering: function(){
			this.domNode = this.srcNodeRef || domConstruct.create(this._tag);
			this.videoNode = domConstruct.create("video",{
				preload: "auto",
				style: "width:inherit;",
				controls: false,
				loop: true
				},this.domNode);
			
			this.videoNode.addEventListener('loadedmetadata', lang.hitch(this, function() {
				this.duration = this.videoNode.duration*0.99;
			    if(this.playSlider){
			    	this.playSlider.set({maximum: this.duration});
			    	this.setTime(this.startpos);
			    }
			    //console.debug(this.duration);
			}));
			
			this.controlsNode = domConstruct.create("div",{style: "width:inherit;"},this.domNode);
			
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
                	this.time = value;
                    this.videoNode.currentTime = this.time;
                })
            });
            this.playSlider.placeAt(playSliderDiv);

			
		},

		setTime: function(time){
			try{
				this.time = time;
				this.playSlider.set("value",this.time,false);
				this.videoNode.currentTime = this.time;
			}catch(e){
				
			}
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
	                this.playSlider.set("value",this.time,false);
	            }), 50); // 50ms for attempted 20fps  
            }else{
/*            	console.log("Stop");
                this.playButton.set("iconClass", "playIcon");
                this.playButton.set("label", "Play");
                clearInterval(this.playing);
                this.playing = null;*/
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
