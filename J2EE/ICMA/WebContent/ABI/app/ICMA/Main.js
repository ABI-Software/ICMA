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
	"dojo/_base/json",
	"dojo/_base/lang",
	"dojo/aspect",
	"dojo/io-query",
	"dojo/topic",
	"dojo/ready",
	"dojo/on",
	"dojo/store/Memory",
	"dojo/store/Observable",
	"dojox/mobile",          	// (Required) This is a mobile app.
	"dojox/mobile/compat",   	// (Optional) This mobile app supports running on desktop browsers
	"dojox/mobile/deviceTheme", // (Optional) Provides device specific css
	"ABI/app/ICMA/MultiImageFit",
	"ABI/app/ICMA/PatientManager",
	"ABI/app/ICMA/PatientView"
], function(declare, json, lang, aspect, ioQuery, topic, ready, on,  MemoryStore, ObservableStore, dm, compat, deviceTheme, MultiImageFit,
		PatientManager, PatientView ){
  
	// module:
	//		ABI/app/ICMA/Main
	// summary:
	//		ICMA Main

	var Main = declare("ABI.app.ICMA.Main", null,{

		queryObject: null, // so far we have isMobile=true 
		isMobile: false,

		server: "",	// Setting up to also support phonegap based installation, but not actually building out installables for the moment...
		
		startup: function(){
			//console.log("ICMA App Startup");
			
			// If running as mobile app we need to set the server path explicitly
			if(window.location.protocol.substring(0, 4) != "file"){
				this.server = window.location.protocol + "//" + window.location.host;

				var pathname = window.location.pathname.split("/");
				this.server = this.server + "/" + pathname[1];
				//console.log("Server path for services and data: " + this.server);
			}

			// Extract and query string parameters
			if(window.location.href.indexOf('?') !== -1){
				var url = window.location.href.substring(window.location.href.indexOf("?") + 1, window.location.href.length);
				this.queryObject = ioQuery.queryToObject(url);
				if(lang.exists("isMobile", this.queryObject)){
					this.isMobile = dojo.fromJson(this.queryObject.isMobile);
				}
			}
			
			this._multiImageFit = new MultiImageFit({
				isMobile: this.isMobile,
			});
			
			this._patientView = new PatientView({
				server: this.server,
				isMobile: this.isMobile
			});		
			
			this._patientView.startup();			
		}
		
	});
	
	ABI.app.ICMA._main = null;

    Main.getSingleton = ABI.app.ICMA.main = function () {
        if (!ABI.app.ICMA._main) {
        	ABI.app.ICMA._main = new ABI.app.ICMA.Main();
        }
        return ABI.app.ICMA._main; // Object
    };

    ABI.app.ICMA.MainSingleton = Main.getSingleton();

    return Main;
});
