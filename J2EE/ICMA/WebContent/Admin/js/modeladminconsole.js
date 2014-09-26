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


function submitModelExport(modelid){
	  var param = {"modelid":modelid};
	      $.ajax({
           type: "POST",
           dataType : "json",
           url: "../REST/admin/exportModel",
           contentType: "application/json",
           data: JSON.stringify(param),
           beforeSend: function( jqXHR, settings){
        	   $( "#busyDialog" ).dialog( "open" );
           },
           success: function(data, textStatus, jqXHR ){
        	   $( "#busyDialog" ).dialog( "close" );
        	   var dialogDiv = $(document.createElement('div'));
    		   dialogDiv.html('<label for="speckletracking" id="speckletrackingHeading">SpeckleTrackingOuptut</label> <textarea rows="15" cols="50" id="speckletracking" style="resize: none; height: 400px; max-height: 400px;">'+data.msg+'</textarea>');
   			   dialogDiv.dialog({modal: true, buttons: {
	   		        Ok: function() {
	   		            $( this ).dialog( "close" );
	   		            $( this ).remove();
	   		          }
	   		        },
	   		        title:"Success",
	   		        width: 690,
	   		        height: 625
   				});
           },
           error: function( jqXHR, textStatus, errorThrown){
        	   $( "#busyDialog" ).dialog( "close" );
        	   console.debug(jqXHR);
        	   var dialogDiv = $(document.createElement('div'));
       		   dialogDiv.html("Failed <BR> "+jqXHR+" <BR>"+textStatus+"\t"+errorThrown);
   			   dialogDiv.dialog({modal: true, buttons: {
   		        Ok: function() {
   		            $( this ).dialog( "close" );
   		            $( this ).remove();
   		          }
   		        },
   		        title:"Failed",
   		        width: 400
   			   });
           }
        });
};

function submitModelDelete(modelid,ctr){
	  var param = {"modelid":modelid,"actor":"adminclient"};
      $.ajax({
       type: "POST",
       dataType : "json",
       url: "../REST/admin/removeModel",
       contentType: "application/json",
       data: JSON.stringify(param),
       beforeSend: function( jqXHR, settings){
    	   $( "#busyDialog" ).dialog( "open" );
       },
       success: function(data, textStatus, jqXHR ){
           //Delete the row
           //data.modelid
           cTable.fnDeleteRow(ctr);
    	   $( "#busyDialog" ).dialog( "close" );
    	   var dialogDiv = $(document.createElement('div'));
		   dialogDiv.html('Successfully removed the model');
			   dialogDiv.dialog({modal: true, buttons: {
   		        Ok: function() {
   		            $( this ).dialog( "close" );
   		            $( this ).remove();
   		          }
   		        },
   		        title:"Success",
   		        width: 400
				});
       },
       error: function( jqXHR, textStatus, errorThrown){
    	   $( "#busyDialog" ).dialog( "close" );
   		   dialogDiv.html("Failed to remove model server returned "+textStatus+"\t"+errorThrown);
			   dialogDiv.dialog({modal: true, buttons: {
		        Ok: function() {
		            $( this ).dialog( "close" );
		            $( this ).remove();
		          }
		        },
		        title:"Failed",
		        width: 400
			   });
       }
    });	
};