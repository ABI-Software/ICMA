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
define("doh/plugins/remoteRobot", ["doh/runner"], function(runner){

/*=====
return {
	// summary:
	//		Plugin that bridges the doh.robot and WebDriver APIs.
};
=====*/
	
	// read in the test and port parameters from the URL
	var remoteRobotURL = "";
	var value = "";
	var paths = "";
	var qstr = window.location.search.substr(1);
	if(qstr.length){	
	var qparts = qstr.split("&");
		for(var x=0; x<qparts.length; x++){
			var tp = qparts[x].split("="), name = tp[0], value = tp[1].replace(/[<>"'\(\)]/g, "");	// replace() to avoid XSS attack
			//Avoid URLs that use the same protocol but on other domains, for security reasons.
			if (value.indexOf("//") === 0 || value.indexOf("\\\\") === 0) {
				throw "Insupported URL";
			}
			switch(name){
				case "remoteRobotURL":
					remoteRobotURL = value;
					break;
				case "paths":
					paths = value;
					break;
			}
		}
	}
	// override doh runner so that it appends the remote robot url to each test
	doh._registerUrl = (function(oi){
		return doh.hitch(doh, function(group, url, timeout, type, dohArgs){
			// append parameter, or specify new query string if appropriate
			if(remoteRobotURL){
				url += (/\?/.test(url)?"&":"?") + "remoteRobotURL=" + remoteRobotURL
			}
			if(paths){
				url += (/\?/.test(url)?"&":"?") + "paths=" + paths;
			}
			top.console.log(group);
			top.console.log(url);
			oi.apply(doh, [group, url, timeout, type, dohArgs]);
		});
	})(doh._registerUrl);
	return remoteRobotURL;
});