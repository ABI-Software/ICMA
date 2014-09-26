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
"use strict";

define([
	"dojo/_base/declare",
	"dojo/_base/array",
    "dojo/_base/lang",
    "dojo/request",
    "dojo/store/Memory",
    "dojo/store/Observable",
    "dojo/topic"
	], function (declare, array, lang, request, MemoryStore, Observable,topic) {

	    var PatientManager = declare("ABI.app.ICMA.PatientManager", null, {

	        store: null,
	        currentPatient: null,
	        currentPatientQuery: { patientid: "" },

	        constructor: function (params) {

	            dojo.mixin(this, params);

	            this.store = Observable(new MemoryStore({
	                idProperty: "patientid"
	            }));

	            this._currentPatientResults = this.store.query(this.currentPatientQuery);

	            this._currentPatientResults.observe(dojo.hitch(this, function (object, removedFrom, insertedInto) {
	                if (removedFrom > -1) { // existing patient object removed
	                    this.onPatientDelete();
	                }
	                if (insertedInto > -1) { // new or updated patient object inserted
	                    this.onPatientUpdate(object);
	                }
	            }), true);
	        },

	        loadPatient: function (patientid, server) {
//debugger;
	        	topic.publish("ABI/lib/Widgets/busy");
	        	this.currentPatientQuery.patientid = new RegExp(patientid, "i");	
//	        	this.currentPatientQuery.patientid = new RegExp("55437475", "i");
	        	                
	        	request(server + "/SXPDetails", {
	        		query: { patientID : patientid},
	        		preventCache: true,
	                handleAs: "json"
	            }).then(lang.hitch(this, function(data){
	            	// do something with handled data
	            	//debugger;
                    this.store.put(data);
                    //topic.publish("ABI/lib/Widgets/ready");
	            }), function(err){
	            	// handle an error condition
	            	//console.log("ABI.app.ICMA.PatientManager: failed to fetch patient data");
	            	topic.publish("ABI/lib/Widgets/ready");
	            }, function(evt){
	            	// handle a progress event
	            	topic.publish("ABI/lib/Widgets/ready");
	            });
	        },

	        getCurrentPatient: function () {
	            return this.currentPatient;
	        },

	        onPatientUpdate: function (patient) {
	            //console.info('ABI.app.ICMA.PatientManager: Patient: ' + patient + ' put/updated patient in store');
	            this.currentPatient = patient;
	        },

	        onPatientDelete: function () {
	            //console.info('ABI.app.ICMA.PatientManager: Deleted patient from store');
	        }
	        
	    });


	    ABI.app.ICMA._patientManager = null;

	    PatientManager.getSingleton = ABI.app.ICMA.patientManager = function () {
	        if (!ABI.app.ICMA._patientManager) {
	        	ABI.app.ICMA._patientManager = new ABI.app.ICMA.PatientManager();
	        }
	        return ABI.app.ICMA._patientManager;
	    };

	    ABI.app.ICMA.PatientManagerSingleton = PatientManager.getSingleton();

	    return PatientManager;
	});