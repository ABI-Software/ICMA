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
	"../buildControl",
	"../version",
	"../fileUtils"
], function(bc, version, fileUtils) {
	var dir = bc.buildReportDir || ".",
		filename = bc.buildReportFilename || "build-report.txt";

	return function(resource, callback) {
		resource.reports.push({
			dir:dir,
			filename:filename,
			content: function(){
				var result= "";

				result+= "Build started: " + bc.startTimestamp + "\n";
				result+= "Build application version: " + version + "\n";

				result+= "Messages:\n" + bc.getAllNonreportMessages();

				result+= "Layer Contents:\n";
				for(var p in bc.resources){
					resource= bc.resources[p];
					if(resource.moduleSet){
						result+= resource.mid + ":\n";
						var moduleSet= resource.moduleSet;
						for(var q in moduleSet){
							result+= "\t" + moduleSet[q].mid + "\n";
						}
						result+= "\n";
					}
				}

				var optimizerOutput = bc.getOptimizerOutput();
				if(optimizerOutput.length){
					result+= "Optimizer Messages:\n" + optimizerOutput;
				}

				result+= bc.getAllReportMessages();

				bc.log("pacify", "Report written to " + fileUtils.computePath(fileUtils.catPath(dir, filename), bc.destBasePath));

				result+= "\n\nProcess finished normally\n";
				result+= "\terrors: " + bc.getErrorCount() + "\n\twarnings: " + bc.getWarnCount() + "\n\tbuild time: " + ((new Date()).getTime() - bc.startTimestamp.getTime()) / 1000 + " seconds";
				return result;
			}
		});
		return 0;
	};
});
