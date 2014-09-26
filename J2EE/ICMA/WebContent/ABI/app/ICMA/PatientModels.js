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
	"dojo/_base/lang",
	"dojo/aspect",
	"dojo/topic",
	"dojo/dom-construct",
	"dijit/layout/ContentPane",
	"dijit/layout/TabContainer",
	"ABI/app/ICMA/Model"
], function(declare, lang, aspect, topic, domConstruct, ContentPane, TabContainer, Model){

	// module:
	//		ABI/app/ICMA/PatientModels

	return declare("ABI.app.ICMA.PatientModels", [ContentPane],{
		// summary:
		//		A widget that manages views of Models
		// description:
		//		
		patientManager: null,
		server: null,
		setupModel: null,
		needsReloaded: false,
		hasShown: false,
		NoModelsMsg: null,
		debug: false,
		constructor: function(params){
			dojo.mixin(this, params);
			
			// Need to clear and restart for patient updates - no test services provided yet for this work...
//			aspect.after(this.patientManager, "onPatientUpdate", lang.hitch(this, this.onShow));
            aspect.after(this.patientManager, "onPatientUpdate", lang.hitch(this, this.removeModels));
            topic.subscribe("/NewModel",lang.hitch(this, this.addModel));
            topic.subscribe("/SetupModel",lang.hitch(this, this.setSetupModel));
            topic.subscribe("/DiscardModel",lang.hitch(this, this.removeSetupModel));
		},
		
		startup: function(){
			if(this._started){ return; }
			this.inherited(arguments);
		},
		
		buildRendering: function(){
			this.inherited(arguments);

			this.tcModels = new TabContainer({
	            style: "height: 100%; width: 100%;"
	        });
	        
	        this.tcModels.startup();
	        
	        this.tcModels.watch("selectedChildWidget", function(name,oval,nval){
	        	//console.debug("Calling from selectedChildWidget ",nval);
	        	topic.publish("/loadFirstModelView",nval);
	        });
	        
	        this.domNode.appendChild( this.tcModels.domNode);
		},
		
		onShow: function(){
			//Gets called each time needsReloaded is true (when a model is discarded)
			if(this.debug)
				console.log("Models tab onShow");
			var patient = this.patientManager.getCurrentPatient();
			for(item in patient.Models){
				//Check if the model exists
				var model = patient.Models[item];
				this.addModel(model);				
			}

			//Models can be in the db and created during the session (in which case they will not be counted)
			if(this.tcModels.getChildren().length ==0){
/*				this.NoModelsMsg = new ContentPane({
				      content:"<h2>There are no models in the database</h2>",
				    });
				this.tcModels.addChild(this.NoModelsMsg);*/
				topic.publish("ABI/lib/Widgets/modelstabhide");
			}
				
			this.needsReloaded = false;
			this.hasShown = true;
		},
		
		addModel: function(model){
			//Multiple calls to add the same model may arrive
			//the function has to check if the model exists prior to adding it
			var exists = false;
			var children = this.tcModels.getChildren();
			if(model.modelid!==undefined){
				for(var i = 0; i < children.length; i++){
	            	if(children[i].model!==undefined){
		            	if(children[i].model.modelid!==undefined){
		            		if(children[i].model.modelid == model.modelid){
		            			exists = true;
		            			break;
		            		}
		            	}else{
		        			//For models from the same target view, model id is always undefined
		            		if(children[i].model.studyUid == model.studyUid){
		            			exists = true;
		            			break;
		            		}
		            	}
	            	}
				}
			}else{
				for(var i = 0; i < children.length; i++){
	            	if(children[i].model!==undefined){
		            	if(children[i].model.modelid===undefined){
		        			//For models from the same target view, model id is always undefined
		            		if(children[i].model.studyUid == model.studyUid){
		            			exists = true;
		            			break;
		            		}
		            	}
	            	}
				}
			}

			if(exists==false){
				if(this.debug)
					console.debug("Adding model "+model.modelid);
				var patient = this.patientManager.getCurrentPatient();
		        var modelView = new Model({
		        	server: this.server,
		        	model: model,
		            title:  model.name,
		            sxmUpdate: this.server + "/" + patient.SXMUPDATE
		        });
	
		        this.tcModels.addChild(modelView);
			}
	        topic.publish("ABI/lib/Widgets/modelstabalive");
		},

		
		resize: function(){
			this.inherited(arguments);
		},
		
		setSetupModel: function(model){
			this.setupModel = model;
		},
		
		removeModel: function(model){
			var children = this.tcModels.getChildren();
            if(children.length > 0){
                for(var i = 0; i < children.length; i++){
                	try{
                		if(children[i].model!=undefined){
		                	if(children[i].model.workflowId!=undefined){
		                		if(children[i].model.studyUid==model.studyUid){
		                			this.tcModels.removeChild(children[i]);
		                			children[i].domNode.style.display = 'none';
		                			children[i].destroyRecursive();
		                		}
		                	}
                		}
                	}catch(e){
                		console.debug(e);
                	}
                }
            }
            if(this.tcModels.getChildren().length==0){
/*            	this.NoModelsMsg = new ContentPane({
				      content:"<h2>There are no models in the database</h2>",
				    });
				this.tcModels.addChild(this.NoModelsMsg);*/
            	topic.publish("ABI/lib/Widgets/modelstabhide");
            }
            this.needsReloaded = true;
		},
		
		
		removeSetupModel: function(){
			this.removeModel(this.setupModel);
			this.setupModel = null;
		},
		
		removeModels: function(){
			var children = this.tcModels.getChildren();
            if(children.length > 0){
                for(var i = 0; i < children.length; i++){
                    this.tcModels.removeChild(children[i]);
                }
            }
            this.onShow();//Load the current patients models
            //topic.publish("ABI/lib/Widgets/modelstabhide");
            //this.needsReloaded = true;
		}
	});
});
