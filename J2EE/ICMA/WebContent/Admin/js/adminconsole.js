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
var oTable;  // kept as handy global in window namespace
var cTable; //Child table
var gaiSelected =  [];
var refreshIntervalId; //The handle to table refresh

//To perform fnRedraw without resetting pagination
$.fn.dataTableExt.oApi.fnStandingRedraw = function(oSettings) {
	document.body.style.cursor='wait';
    if(oSettings.oFeatures.bServerSide === false){
        var before = oSettings._iDisplayStart;
 
        oSettings.oApi._fnReDraw(oSettings);
 
        // iDisplayStart has been reset to zero - so lets change it back
        oSettings._iDisplayStart = before;
        oSettings.oApi._fnCalculateEnd(oSettings);
    }
      
    // draw the 'current' page
    oSettings.oApi._fnDraw(oSettings);
    document.body.style.cursor='default';
};

//To rest the row selection
$.fn.dataTableExt.oApi.fnToggleRowSelection = function ( oSettings, sSearch, iColumn ){
		var
	    i,iLen, j, jLen,
	    aOut = [], aData,
	    nTr, id;
	for ( i=0, iLen=oSettings.aoData.length ; i<iLen ; i++ )
	{
	    aData = oSettings.aoData[i]._aData;
	    nTr = oSettings.aoData[i].nTr;
    	id = aData[0].replace(/\./,'d').replace(/\:/g,'').replace(/\//g,'s');	      
	    if ( typeof iColumn == 'undefined' )
	    {
	        for ( j=0, jLen=aData.length ; j<jLen ; j++ )
	        {
	            if ( aData[j] == sSearch )
	            {
	            	$(nTr).toggleClass('row_selected');
	            	$('#cbx'+id).prop('checked',false);
	                aOut.push( i );
	            }
	        }
	    }
	    else if ( aData[iColumn] == sSearch )
	    {
        	$(nTr).toggleClass('row_selected');
        	$('#cbx'+id).prop('checked',false);
	        aOut.push( i );
	    }
	}
	  
	return aOut;
};

//To find the row index for a given key
$.fn.dataTableExt.oApi.fnFindCellRowIndexes = function ( oSettings, sSearch, iColumn )
{
    var
        i,iLen, j, jLen,
        aOut = [], aData;
    for ( i=0, iLen=oSettings.aoData.length ; i<iLen ; i++ )
    {
        aData = oSettings.aoData[i]._aData;
          
        if ( typeof iColumn == 'undefined' )
        {
            for ( j=0, jLen=aData.length ; j<jLen ; j++ )
            {
                if ( aData[j] == sSearch )
                {
                    aOut.push( i );
                }
            }
        }
        else if ( aData[iColumn] == sSearch )
        {
            aOut.push( i );
        }
    }
      
    return aOut;
};


var oCache = {
	    iCacheLower: -1
};
	 
	function fnSetKey( aoData, sKey, mValue )
	{
	    for ( var i=0, iLen=aoData.length ; i<iLen ; i++ )
	    {
	        if ( aoData[i].name == sKey )
	        {
	            aoData[i].value = mValue;
	        }
	    }
	}
	 
	function fnGetKey( aoData, sKey )
	{
	    for ( var i=0, iLen=aoData.length ; i<iLen ; i++ )
	    {
	        if ( aoData[i].name == sKey )
	        {
	            return aoData[i].value;
	        }
	    }
	    return null;
	}
	 
	function fnDataTablesPipeline ( sSource, aoData, fnCallback ) {
	    var iPipe = 5; /* Adjust the pipe size */
	     
	    var bNeedServer = false;
	    var sEcho = fnGetKey(aoData, "sEcho");
	    var iRequestStart = fnGetKey(aoData, "iDisplayStart");
	    var iRequestLength = fnGetKey(aoData, "iDisplayLength");
	    var iRequestEnd = iRequestStart + iRequestLength;
	    oCache.iDisplayStart = iRequestStart;
	     
	    /* outside pipeline? */
	    if ( oCache.iCacheLower < 0 || iRequestStart < oCache.iCacheLower || iRequestEnd > oCache.iCacheUpper )
	    {
	        bNeedServer = true;
	    }
	     
	    /* sorting etc changed? */
	    if ( oCache.lastRequest && !bNeedServer )
	    {
	        for( var i=0, iLen=aoData.length ; i<iLen ; i++ )
	        {
	            if ( aoData[i].name != "iDisplayStart" && aoData[i].name != "iDisplayLength" && aoData[i].name != "sEcho" )
	            {
	                if ( aoData[i].value != oCache.lastRequest[i].value )
	                {
	                    bNeedServer = true;
	                    break;
	                }
	            }
	        }
	    }
	     
	    /* Store the request for checking next time around */
	    oCache.lastRequest = aoData.slice();
	     
	    if ( bNeedServer )
	    {
	        if ( iRequestStart < oCache.iCacheLower )
	        {
	            iRequestStart = iRequestStart - (iRequestLength*(iPipe-1));
	            if ( iRequestStart < 0 )
	            {
	                iRequestStart = 0;
	            }
	        }
	         
	        oCache.iCacheLower = iRequestStart;
	        oCache.iCacheUpper = iRequestStart + (iRequestLength * iPipe);
	        oCache.iDisplayLength = fnGetKey( aoData, "iDisplayLength" );
	        fnSetKey( aoData, "iDisplayStart", iRequestStart );
	        fnSetKey( aoData, "iDisplayLength", iRequestLength*iPipe );
	         
	        $.getJSON( sSource, aoData, function (json) {
	            /* Callback processing */
	            oCache.lastJson = jQuery.extend(true, {}, json);
	             
	            if ( oCache.iCacheLower != oCache.iDisplayStart )
	            {
	                json.aaData.splice( 0, oCache.iDisplayStart-oCache.iCacheLower );
	            }
	            json.aaData.splice( oCache.iDisplayLength, json.aaData.length );
	             
	            fnCallback(json);
	        } );
	    }
	    else
	    {
	        json = jQuery.extend(true, {}, oCache.lastJson);
	        json.sEcho = sEcho; /* Update the echo for each response */
	        json.aaData.splice( 0, iRequestStart-oCache.iCacheLower );
	        json.aaData.splice( iRequestLength, json.aaData.length );
	        fnCallback(json);
	        return;
	    }
	}
	
	function refreshServerData(){
		oTable.fnStandingRedraw();
	}
	
	function submitAction(buttonID){
		var query = "#"+buttonID;
		var button = $(query);
		var bid = button.attr('bid');
		var action = button.attr('action');
		var confirmed = true;
		//confirm once again
		if(action == 'delete'){
			confirmed = confirm("Are you sure you wish to remove the patient record from ICMA!!");	
		}
		
		if(confirmed){
			//Call server to delete data and if successful remove the row (call fnDraw to refresh)
			var param = {"patientid":bid,"action":action};
	        $.ajax({
	           type: "POST",
	           dataType : "json",
	           url: "../REST/admin/associate",
	           contentType: "application/json",
	           data: JSON.stringify(param),
	           beforeSend: function( jqXHR, settings){
	        	   $( "#busyDialog" ).dialog( "open" );
	           },
	           success: function(data, textStatus, jqXHR ){
	        	   $( "#busyDialog" ).dialog( "close" );
	        	   var dialogDiv = $(document.createElement('div'));
	        	   if(action=="sync")
	        		   dialogDiv.html('Successfully submitted the request to the server<BR> Refresh table to follow record status changes');
	        	   else
	        		   dialogDiv.html('Successfully removed patient from ICMA records <BR> Refresh table to follow record status changes');
	   			   dialogDiv.dialog({modal: true, buttons: {
	   		        Ok: function() {
	   		            $( this ).dialog( "close" );
	   		            $( this ).remove();
	   		          }
	   		        },
	   		        title:"Success",
	   		        width: 400
	   			});
	   			refreshServerData();
	   			refreshIntervalId = setInterval(refreshServerData, 120000);//Every two minutes
	           },
	           error: function( jqXHR, textStatus, errorThrown){
	        	   $( "#busyDialog" ).dialog( "close" );
	       		   dialogDiv.html("Failed to perfom action "+action+" on Patient "+bid+" server returned "+textStatus+"\t"+errorThrown);
	   			   dialogDiv.dialog({modal: true, buttons: {
	   		        Ok: function() {
	   		            $( this ).dialog( "close" );
	   		            $( this ).remove();
	   		          }
	   		        },
	   		        title:"Failed",
	   		        width: 400
	   			   });
	   			   refreshServerData();
	               oTable.fnToggleRowSelection(bid,0);
	           }
	        });
		}
	}

	function showModels(patientID){
		var iId = patientID;
    	var dialogDiv = $(document.createElement('div')); 
    	dialogDiv.html('<div id="models">Loading data</div>');
		dialogDiv.dialog({
			closeOnEscape: false,
			open: function(event,ui){
				//If the user closes by escape or title bar, then opne in subsequent calls does not perform as expected
				//so remove it
				$(".ui-dialog-titlebar-close").hide(); 
				$('#models').load('modelAdminConsole.jsp?patientID='+iId);
			},
			modal: true, 
			buttons: {
				Close: function() {
					$( this ).dialog( "close" );
					$( this ).remove();
	            }
	        },
	        title:"Models",
	        width: 800,
	        height: 400
		});
	}
	
	
	$(document).ready(function() {

		$( "#busyDialog" ).dialog({
		      autoOpen: false,
		      position: 'top',
		      show: {
		        effect: "blind",
		        duration: 100
		      },
		      hide: {
		        effect: "explode",
		        duration: 100
		      }
	    });

		
		$("batchAdd").button();
		$("batchRemove").button();
		$("batchCompute").button();
		
	    oTable = $('#patients_table').dataTable( {
	        "bProcessing": true,
	        "bServerSide": true,
	        "sAjaxSource": "../PatientDBInterface",
	        "fnServerData": fnDataTablesPipeline,
	        "bJQueryUI": true,
	        "sPaginationType" : "full_numbers",
	        "bPaginate" : true,
	        "bLengthChange" : true,
	        "bAutoWidth" : false,
	        "aoColumns" : [
	                       {"sClass": "control center"},
	                       {"sClass": "control left"},
	                       {"sClass": "control center"},
	                       {"sClass": "control center"},
	                       {"sClass": "control center"}
	                      ],

	        "fnRowCallback": function( nRow, aData, iDisplayIndex ) {
				if ( jQuery.inArray(aData[0], gaiSelected) != -1 )
				{
					$(nRow).addClass('row_selected');
				}
				return nRow;
			},
	        "aoColumnDefs": [
	                         {
	                             "mRender": function ( data, type, full ) {
		                             //Ensure that special characters are removed from the id string, else jquery bombs
	                            	 var id = full[0].replace(/\./,'d').replace(/\:/g,'').replace(/\//g,'s');
	                            	 if(full[2]!="operating"){
	                            		if ( jQuery.inArray(data, gaiSelected) == -1 )
	                     				{ 
	                            			return '<div class=\"datatableTextCenterAlign\" ><input type=\"checkbox\" id=\"cbx'+id+'\" value="'+ data +'"></div>';
	                     				}else{
	                     					return '<div class=\"datatableTextCenterAlign\" ><input type=\"checkbox\" id=\"cbx'+id+'\" value="'+ data +'" checked></div>';
	                     				}
	                            	 }else{
	                            		 return '<div class=\"datatableTextCenterAlign\" ><input type=\"checkbox\" disabled=\"disabled\" id=\"cbx'+id+'\" value="'+ data +'"></div>';
		                             }
	                             },
	                             "aTargets": [ 4 ]
	                         },
	                         {
	                             "mRender": function ( data, type, full ) {
		                             var location = full[2];
		                             var action = 'delete';
		                           	 //Ensure that special characters are removed from the id string, else jquery bombs
		                             var id = data.replace(/\./,'d').replace(/\:/g,'').replace(/\//g,'s');
		                             var bLabel = "Remove from ICMA";
		                             if(location=="PACS Only"){
		                            	 bLabel = "Sync with ICMA";
		                            	 action = 'sync';
		                             } 
		                             if(location!="operating"){
		                            	 if(bLabel != "Remove from ICMA"){
		                            		 return '<div class=\"datatableTextCenterAlign\" ><input type=\"button\" class=\"dbUpdate\" bid=\"'+data+'\" id=\"'+id+'\" action=\"'+action+'\" value="'+ bLabel +'\" onclick=\"submitAction(\''+id+'\')\"></div>';
		                            	 }else{
		                            		 return '<div class=\"datatableTextCenterAlign\" ><input type=\"button\" bid=\"'+data+'\" id=\"m'+id+'\" action=\"Show Models\" value="Show Models\" onclick=\"showModels(\''+data+'\')\">'+
		                            		 		                                    '&nbsp<input type=\"button\" class=\"dbUpdate\" bid=\"'+data+'\" id=\"'+id+'\" action=\"'+action+'\" value="'+ bLabel +'\" onclick=\"submitAction(\''+id+'\')\"></div>';
		                            	 }
		                             }else{
			                            return  '<div class=\"datatableTextCenterAlign\" >Processing</div>';
			                         }
	                             },
	                             "aTargets": [ 3 ]
	                         }
	              ]
	    } );


	    $('#patients_table tbody').on('dblclick', "tr", function (event) {
	    	var aData = oTable.fnGetData( this );
			var iId = aData[0];
	    	var dialogDiv = $(document.createElement('div')); 
	    	dialogDiv.html('<div id="models">Loading data</div>');
			dialogDiv.dialog({
				closeOnEscape: false,
				open: function(event,ui){
					//If the user closes by escape or title bar, then opne in subsequent calls does not perform as expected
					//so remove it
					$(".ui-dialog-titlebar-close").hide(); 
					$('#models').load('modelAdminConsole.jsp?patientID='+iId);
				},
				modal: true, 
				buttons: {
					Close: function() {
						$( this ).dialog( "close" );
						$( this ).remove();
		            }
		        },
		        title:"Models",
		        width: 800,
		        height: 400
			});
	    });
	    
	    $('#patients_table tbody').on('click', "tr", function (event) {
	    	if(event.target.nodeName=='INPUT' && event.target.type!='checkbox') //Do not select if buttons are clicked
	    		return;
			var aData = oTable.fnGetData( this );
			var iId = aData[0];
			var id = aData[0].replace(/\./,'d').replace(/\:/g,'').replace(/\//g,'s');
			if(aData[2]!="operating"){
				if ( jQuery.inArray(iId, gaiSelected) == -1 )
				{
					gaiSelected[gaiSelected.length++] = iId;
					$('#cbx'+id).prop('checked',true);
				}
				else
				{
					//remove the id
					gaiSelected = jQuery.grep(gaiSelected, function(value) {
						return value != iId;
					} );
					$('#cbx'+id).prop('checked',false);
				}
				$(this).toggleClass('row_selected');
			}
		} );


		$('#batchAdd').click(function(){
//			alert("Your current server configuration does not allow for batch additions, try one patient at a time.")
			//Keys are is available in gaiSelected, create a json using that
			var json = {"action":"sync", "patientids":gaiSelected};
			//console.debug(JSON.stringify(json));
	        $.ajax({
	            type: "POST",
	            dataType : "json",
	            url: "../REST/admin/batchAssociate",
	            contentType: "application/json",
	            data: JSON.stringify(json),
	            success: function(data, textStatus, jqXHR ){
	    			//Reset the selections
	    			for(var i=0;i<gaiSelected.length;i++){
	    				oTable.fnToggleRowSelection(gaiSelected[i],0);
	    			}
	    			gaiSelected = [];
	    			var dialogDiv = $(document.createElement('div')); 
	    			dialogDiv.html('Successfully submitted the request to the server<BR> Refresh table to follow record status changes');
	    			dialogDiv.dialog({modal: true, buttons: {
	    		        Ok: function() {
	    		            $( this ).dialog( "close" );
	    		            $( this ).remove();
	    		          }
	    		        },
	    		        title:"Success",
	    		        width: 400
	    			});
	    			refreshServerData();
	    			refreshIntervalId = setInterval(refreshServerData, 120000);//Every two minutes
	            },
	            error: function( jqXHR, textStatus, errorThrown){
	    			var dialogDiv = $(document.createElement('div')); 
	    			dialogDiv.html('Failed to process the request, server returned '+textStatus+'\t'+errorThrown);
	    			dialogDiv.dialog({modal: true, buttons: {
	    		        Ok: function() {
	    		            $( this ).dialog( "close" );
	    		            $( this ).remove();
	    		          }
	    		        },
	    		        title:"Failed",
	    		        width: 400
	    			});
	    			//Reset the selections
	    			for(var i=0;i<gaiSelected.length;i++){
	    				oTable.fnToggleRowSelection(gaiSelected[i],0);
	    			}
	    			gaiSelected = [];
	            }
	         });

		});	    

		$('#batchRemove').click(function(){
			//Keys are is available in gaiSelected, create a json using that
			var json = {"action":"delete", "patientids":gaiSelected};
	        $.ajax({
	            type: "POST",
	            dataType : "json",
	            url: "../REST/admin/batchAssociate",
	            contentType: "application/json",
	            data: JSON.stringify(json),
	            success: function(data, textStatus, jqXHR ){
	    			//Reset the selections
	    			for(var i=0;i<gaiSelected.length;i++){
	    				oTable.fnToggleRowSelection(gaiSelected[i],0);
	    			}
	    			gaiSelected = [];
	    			var dialogDiv = $(document.createElement('div')); 
	    			dialogDiv.html('Successfully submitted the request to the server<BR> Refresh table to follow record status changes');
	    			dialogDiv.dialog({modal: true, buttons: {
	    		        Ok: function() {
	    		            $( this ).dialog( "close" );
	    		            $( this ).remove();
	    		          }
	    		        },
	    		        title:"Success",
	    		        width: 400
	    			});
	    			refreshServerData();
	    			refreshIntervalId = setInterval(refreshServerData, 120000);//Every two minutes
	            },
	            error: function( jqXHR, textStatus, errorThrown){
	    			var dialogDiv = $(document.createElement('div')); 
	    			dialogDiv.html('Failed to process the request, server returned '+textStatus+'\t'+errorThrown);
	    			dialogDiv.dialog({modal: true, buttons: {
	    		        Ok: function() {
	    		            $( this ).dialog( "close" );
	    		            $( this ).remove();
	    		          }
	    		        },
	    		        title:"Failed",
	    		        width: 400
	    			});

	    			//Reset the selections
	    			for(var i=0;i<gaiSelected.length;i++){
	    				oTable.fnToggleRowSelection(gaiSelected[i],0);
	    			}
	    			gaiSelected = [];
	            }
	         });
		});	  
		
		var uploadObj = $('#fileupload').uploadFile({
            dataType: 'json',
            multiple:true,
            autoSubmit:false,
            showStatusAfterSuccess: false,
            url: '../loadDicomFiles',
            onSuccess:function(files,data,xhr){
            	oTable.fnStandingRedraw();
            	console.debug(data);
            },
            onError: function(files,data,xhr){
            	console.debug(data);
            },
            afterUploadAll:function()
            {
            	//Start a refresh
            	$.ajax({
            		  type: "GET",
            		  url: '../loadDicomFiles?refreshPatientMap=true',
            		});
            	alert("Selected files have been uploaded, failed uploads if any are listed below");           	
            }
        });
		
		$("#Upload").click(function(){
			uploadObj.startUpload();
		});
		
		$("#logout").button();
		
		$("#logout").click(function(){
			window.open("./logout.jsp",'_self','resizable,location,menubar,toolbar,scrollbars,status');
		});
		
	} );
