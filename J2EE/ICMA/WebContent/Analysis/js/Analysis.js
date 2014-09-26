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
var oTable = null; // kept as handy global in window namespace
var gaiUnSelected = [];
var anOpen = [];
var queryString;
var listOfModels = [];
var variablesTable;
var existingReports;
var selectedRow = 1;
var rowData = 1;
var selectedReportRow = 1;
var GeneratingDialog;
var notesDialog;

// To perform fnRedraw without resetting pagination
$.fn.dataTableExt.oApi.fnStandingRedraw = function(oSettings) {
	if (oSettings.oFeatures.bServerSide === false) {
		var before = oSettings._iDisplayStart;

		oSettings.oApi._fnReDraw(oSettings);

		// iDisplayStart has been reset to zero - so lets change it back
		oSettings._iDisplayStart = before;
		oSettings.oApi._fnCalculateEnd(oSettings);
	}

	// draw the 'current' page
	oSettings.oApi._fnDraw(oSettings);
};

// To rest the row selection
$.fn.dataTableExt.oApi.fnToggleRowSelection = function(oSettings, sSearch,
		iColumn) {
	var i, iLen, j, jLen, aOut = [], aData, nTr, id;
	for (i = 0, iLen = oSettings.aoData.length; i < iLen; i++) {
		aData = oSettings.aoData[i]._aData;
		nTr = oSettings.aoData[i].nTr;
		id = aData[0].replace(/\./, 'd').replace(/\:/g, '').replace(/\//g, 's');
		if (typeof iColumn == 'undefined') {
			for (j = 0, jLen = aData.length; j < jLen; j++) {
				if (aData[j] == sSearch) {
					$(nTr).toggleClass('row_selected');
					$('#cbx' + id).prop('checked', false);
					aOut.push(i);
				}
			}
		} else if (aData[iColumn] == sSearch) {
			$(nTr).toggleClass('row_selected');
			$('#cbx' + id).prop('checked', false);
			aOut.push(i);
		}
	}

	return aOut;
};

// To find the row index for a given key
$.fn.dataTableExt.oApi.fnFindCellRowIndexes = function(oSettings, sSearch,
		iColumn) {
	var i, iLen, j, jLen, aOut = [], aData;
	for (i = 0, iLen = oSettings.aoData.length; i < iLen; i++) {
		aData = oSettings.aoData[i]._aData;

		if (typeof iColumn == 'undefined') {
			for (j = 0, jLen = aData.length; j < jLen; j++) {
				if (aData[j] == sSearch) {
					aOut.push(i);
				}
			}
		} else if (aData[iColumn] == sSearch) {
			aOut.push(i);
		}
	}

	return aOut;
};

var oCache = {
	iCacheLower : -1
};

function fnSetKey(aoData, sKey, mValue) {
	for ( var i = 0, iLen = aoData.length; i < iLen; i++) {
		if (aoData[i].name == sKey) {
			aoData[i].value = mValue;
		}
	}
}

function fnGetKey(aoData, sKey) {
	for ( var i = 0, iLen = aoData.length; i < iLen; i++) {
		if (aoData[i].name == sKey) {
			return aoData[i].value;
		}
	}
	return null;
}

function fnDataTablesPipeline(sSource, aoData, fnCallback) {
	var iPipe = 5; /* Adjust the pipe size */

	var bNeedServer = false;
	var sEcho = fnGetKey(aoData, "sEcho");
	var iRequestStart = fnGetKey(aoData, "iDisplayStart");
	var iRequestLength = fnGetKey(aoData, "iDisplayLength");
	var iRequestEnd = iRequestStart + iRequestLength;
	oCache.iDisplayStart = iRequestStart;

	/* outside pipeline? */
	if (oCache.iCacheLower < 0 || iRequestStart < oCache.iCacheLower
			|| iRequestEnd > oCache.iCacheUpper) {
		bNeedServer = true;
	}

	/* sorting etc changed? */
	if (oCache.lastRequest && !bNeedServer) {
		for ( var i = 0, iLen = aoData.length; i < iLen; i++) {
			if (aoData[i].name != "iDisplayStart"
					&& aoData[i].name != "iDisplayLength"
					&& aoData[i].name != "sEcho") {
				if (aoData[i].value != oCache.lastRequest[i].value) {
					bNeedServer = true;
					break;
				}
			}
		}
	}

	/* Store the request for checking next time around */
	oCache.lastRequest = aoData.slice();

	if (bNeedServer) {
		if (iRequestStart < oCache.iCacheLower) {
			iRequestStart = iRequestStart - (iRequestLength * (iPipe - 1));
			if (iRequestStart < 0) {
				iRequestStart = 0;
			}
		}

		oCache.iCacheLower = iRequestStart;
		oCache.iCacheUpper = iRequestStart + (iRequestLength * iPipe);
		oCache.iDisplayLength = fnGetKey(aoData, "iDisplayLength");
		fnSetKey(aoData, "iDisplayStart", iRequestStart);
		fnSetKey(aoData, "iDisplayLength", iRequestLength * iPipe);

		$.getJSON(sSource, aoData, function(json) {
			/* Callback processing */
			oCache.lastJson = jQuery.extend(true, {}, json);

			if (oCache.iCacheLower != oCache.iDisplayStart) {
				json.aaData
						.splice(0, oCache.iDisplayStart - oCache.iCacheLower);
			}
			json.aaData.splice(oCache.iDisplayLength, json.aaData.length);

			fnCallback(json);
		});
	} else {
		json = jQuery.extend(true, {}, oCache.lastJson);
		json.sEcho = sEcho; /* Update the echo for each response */
		json.aaData.splice(0, iRequestStart - oCache.iCacheLower);
		json.aaData.splice(iRequestLength, json.aaData.length);
		fnCallback(json);
		return;
	}
}

function refreshServerData() {
	oTable.fnStandingRedraw();
}

function showNotes(noteid) {
	var notes = $('#' + noteid).text();
	if (notes != '') {
		notesDialog.html(notes);
		$("#ModelNotes").dialog('open');
	} else
		alert("No notes have been made!!");

}

function toggleModelSelection(cbxid) {
	if (jQuery.inArray(cbxid, gaiUnSelected) == -1) {
		gaiUnSelected.push(cbxid);
	} else {// Remove
		// remove the id
		gaiUnSelected = jQuery.grep(gaiUnSelected, function(value) {
			return value != cbxid;
		});
	}
	// Set the hidden input Value with csv of unselected
	$('#unSelected').val(gaiUnSelected.join(", "));
}

function editExpression(index) {
	curRowPos = index;
	var oData = variablesTable.fnGetData(index);
	$('#name').val(oData[0]);
	$('#exp').val(oData[0]);
}

function downLoadReport(str) {
	$(self).attr("href", ".." + str);
	// starting download process
	$.fileDownload($(self).attr('href'), {
		// preparingMessageHtml: "Downloading the report, please wait...",
		failMessageHtml : "There was a problem generating your report!"
	});
}

function deleteReport(str) {
	var con = confirm("Are you sure you want to delete the report?");
	if (con == true) {
		var request = $.ajax({
			url : "../UserDOCSList?delete=" + str,
			type : "post",
			dataType : "text"
		});

		request.done(function(response, textStatus, jqXHR) {
			alert("Report deleted from repository");
			existingReports.fnDeleteRow(selectedReportRow);
			// window.open(".."+response);
		});

		request.fail(function(jqXHR, textStatus, errorThrown) {
			// log the error to the console
			alert("Unable to delete the report!");
			console.error("The following error occured: " + textStatus,
					errorThrown);
		});
	}
}

$(document)
		.ready(
				function() {

					$("#logout").button();
					
					$("#logout").click(function(){
						window.open("./logout.jsp",'_self','resizable,location,menubar,toolbar,scrollbars,status');
					});
					
					$("#helpButton").button();

					$("#addVariable").button();

					$("#addVariable").click(
							function() {
								$('#variables tr:last').after(
										'<tr><td>' + $("#name").val()
												+ '</td><td>' + $("#exp").val()
												+ '</td></tr>');
							});

					$("#helpButton").click(function() {
						$("#dialog-message").dialog({
							modal : true,
							width : 600,
							buttons : {
								Ok : function() {
									$(this).dialog("close");
								}
							}
						});
					});

					$("#generateReport").button();

					$("#generateReport")
							.click(
									function() {
										var reportName = $('#reportName').val();
										if ($.trim(reportName).length == 0) {
											alert("Please specify a name for the document");
											return;
										}
										GeneratingDialog = $('<div>', {
											title : "Generating report",
											text : "Please wait...",
										}).dialog({
											modal : true,
											center : true
										});

										// Get the list of selected models
										var selection = new Object();
										for ( var i = 0; i < listOfModels.length; i++) {
											selection[listOfModels[i]] = listOfModels[i];
										}
										for ( var i = 0; i < gaiUnSelected.length; i++) {
											delete selection[gaiUnSelected[i]];
										}
										var pklist = new Array();
										for (k in selection) {
											pklist.push(k);
										}
										// Get the list of analysis variables
										var explist = new Array();

										$('#variables tbody tr').each(
												function(index, item) {
													var name = $(this).find(
															'td:first').html();
													var exp = $(this).find(
															'td:last').html();
													explist.push(name + "="
															+ exp);
												});

										/*
										 * variablesTable.each(function(index,
										 * item){ var oData =
										 * variablesTable.fnGetData( index );
										 * if(oData!=null){
										 * explist.push(oData[0]+"="+oData[1]); }
										 * });
										 */

										var jsonReq = new Object();
										jsonReq["pklist"] = pklist;
										jsonReq["analysisvariables"] = explist;
										jsonReq["reportname"] = reportName;
										var self = this;
										var request = $
												.ajax({
													url : "../ReportGenerator",
													type : "post",
													data : JSON
															.stringify(jsonReq),
													contentType : "application/json; charset=utf-8",
													dataType : "text"
												});

										request
												.done(function(response,
														textStatus, jqXHR) {
													// log a message to the
													// console
													GeneratingDialog
															.dialog('close');
													$(self).attr("href",
															".." + response);
													// starting download process
													$
															.fileDownload(
																	$(self)
																			.attr(
																					'href'),
																	{
																		// preparingMessageHtml:
																		// "Preparing
																		// your
																		// report,
																		// please
																		// wait...",
																		failMessageHtml : "There was a problem generating your report!"
																	});
													// window.open(".."+response);
												});

										request.fail(function(jqXHR,
												textStatus, errorThrown) {
											// log the error to the console
											$GeneratingDialog.dialog('close');
											console.error(
													"The following error occured: "
															+ textStatus,
													errorThrown);
										});

									});

					existingReports = $("#existingreportstable")
							.dataTable(
									{
										"bDestroy" : true, // Destroy previous
															// table if any
										"bProcessing" : true,
										"bFilter" : true,
										"sAjaxSource" : "../UserDOCSList?filter=Analysis",
										"bJQueryUI" : true,
										"sPaginationType" : "full_numbers",
										"bPaginate" : true,
										"bLengthChange" : true,
										"bAutoWidth" : false,
										"aoColumns" : [
												{
													"mDataProp" : "name",
													"sClass" : "control center"
												},
												{
													"mDataProp" : "path",
													"sClass" : "control center",
													"bSortable" : false,
													"fnRender" : function(oObj) {
														return '<button class=".ui-button" type="button" onclick=downLoadReport("'
																+ oObj.aData["path"]
																+ '")> Download </button><button class=".ui-button" type="button" onclick=deleteReport("'
																+ oObj.aData["path"]
																+ '")> Delete </button>';
													}
												} ]
									});

					$('#existingreportstable tbody').on(
							'click',
							"tr",
							function(event) {
								selectedReportRow = existingReports
										.fnGetPosition(this);
							});

					var form = $("#demoForm")
							.formwizard(
									{
										formPluginEnabled : true,
										validationEnabled : true,
										focusFirstInput : true,
										formOptions : {
											success : function(data) {
												$("#status")
														.fadeTo(
																500,
																1,
																function() {
																	$(this)
																			.html(
																					"You are now registered!")
																			.fadeTo(
																					5000,
																					0);
																});
											},
											dataType : 'json',
											resetForm : true
										}
									});
					// Remove the back button
					form.find(":reset").hide();

					var remoteAjax = {};
					// empty options object

					remoteAjax['first'] = {
						url : "../ModelQueryMaker",
						dataType : 'text',
						success : function(data) {
							// Set the query input to the query
							queryString = data; // To be used in the next step
							$('#queryString').val(data);
							// Create the dataTable
							oTable = $('#models_table')
									.dataTable(
											{
												"bDestroy" : true, // Destroy
																	// previous
																	// table if
																	// any
												"bProcessing" : true,
												"bFilter" : false,
												"bServerSide" : true,
												"sAjaxSource" : "../" + data,
												"fnServerData" : fnDataTablesPipeline,
												"bJQueryUI" : true,
												"sPaginationType" : "full_numbers",
												"bPaginate" : true,
												"bLengthChange" : true,
												"bAutoWidth" : false,
												"aoColumns" : [
														{
															"mDataProp" : "id",
															"sClass" : "control center"
														},
														{
															"mDataProp" : "name"
														},
														{
															"mDataProp" : "gender",
															"sClass" : "control center"
														},
														{
															"mDataProp" : "birthdate"
														},
														{
															"mDataProp" : "counter",
															"sClass" : "control center",
															"bSortable" : false
														},
														{
															"mDataProp" : null,
															"sClass" : "control center",
															"bSortable" : false,
															"sDefaultContent" : '<img src="./images/details_open.png">'
														} ]

											});

							$('#models_table')
									.on(
											'click',
											'td.control',
											function(event) {
												var nTr = this.parentNode;
												var i = $.inArray(nTr, anOpen);

												if (i === -1) {
													$('img', this)
															.attr('src',
																	"./images/details_close.png");
													var nDetailsRow = oTable
															.fnOpen(
																	nTr,
																	fnFormatDetails(
																			oTable,
																			nTr),
																	'details');
													$('div.innerDetails',
															nDetailsRow)
															.slideDown();
													anOpen.push(nTr);
												} else {
													$('img', this)
															.attr('src',
																	"./images/details_open.png");
													$('div.innerDetails',
															$(nTr).next()[0])
															.slideUp(
																	function() {
																		oTable
																				.fnClose(nTr);
																		anOpen
																				.splice(
																						i,
																						1);
																	});
												}
											});

							function fnFormatDetails(oTable, nTr) {
								var oData = oTable.fnGetData(nTr);
								var myModels = oData.models;
								var sOut = '<div class="innerDetails" style="background-color:white">'
										+ '<table cellpadding="5" cellspacing="0" border="0" style="padding-left:50px;">'
										+ '<thead><tr><th></th><th>Name</th><th>Status</th><th>Study date</th><th>Study Type</th><th>Gravitational Conditioning</th><th>Special Classification</th><th>Notes</th></tr></thead>';
								for ( var m in myModels) {
									sOut += '<tr><td>'
											+ convertModelToHtml(myModels[m])
											+ '</td></tr>';
								}
								sOut += '</table></div>';
								return sOut;
							}

							function convertModelToHtml(model) {
								var anot = model.annotation;
								var html = '<tr><td><input type=\"checkbox\" id=\"cbx'
										+ model.pk
										+ '\" value="'
										+ model.pk
										+ '" onclick=toggleModelSelection(\''
										+ model.pk + '\') ';
								if (jQuery.inArray(model.pk, gaiUnSelected) == -1)
									html = html + 'checked></td> ';
								else
									html = html + '></td> ';

								html = html
										+ '<td>'
										+ model.name
										+ '</td>'
										+ '<td>'
										+ model.status
										+ '</td>'
										+ '<td>'
										+ model.studydate
										+ '</td>'
										+ '<td style="text-align:center;">'
										+ anot.studyType
										+ '</td>'
										+ '<td style="text-align:center;">'
										+ anot.gravityType
										+ '</td>'
										+ '<td style="text-align:center;">'
										+ anot.classification
										+ '</td>'
										+ '<td><button class=".ui-button" type="button" onclick="showNotes(\'Anot'
										+ model.pk
										+ '\')" >View</button><div id="Anot'
										+ model.pk + '"  style="display:none">'
										+ anot.notes + '</div></td>' +
										// '<td>'+anot.notes+'</td>'+
										'</tr>';
								return html;
							}

							return true;
						},
						error : function(data) {
							alert("Failed to retrieve the results " + data);
							return false;
						}
					};

					remoteAjax['modelSelection'] = { // Collect the model
														// data
						url : "dummy.html",
						dataType : 'text',
						success : function(data) {
							$('#models_table tr').each(function(index, item) {
								var oData = oTable.fnGetData(index);
								if (oData != null) {
									var models = oData.models;
									for (key in models) {
										var obj = models[key];
										listOfModels.push(obj.pk);
									}
								}
							});
							// console.debug(listOfModels);
							// console.debug(gaiUnSelected);
							variablesTable = $("#variables").dataTable({
								"sDom" : 't'
							});

							$('#variables tbody').on(
									'dblclick',
									"tr",
									function(event) {
										rowData = variablesTable
												.fnGetData(this);
										selectedRow = variablesTable
												.fnGetPosition(this);
										// Popup a populated model dialog

										$("#Modify").dialog("open");
									});

							return true;
						}
					};

					$("#Modify")
							.dialog(
									{
										autoOpen : false,
										height : 'auto',
										width : 500,
										center : true,
										modal : true,
										open : function() {
											// console.debug(rowData);
											$("#ename").val(rowData[0]);
											$("#eexp").val(rowData[1]);
										},
										buttons : {
											"Apply changes" : function() {
												variablesTable.fnUpdate($(
														"#ename").val(),
														selectedRow, 0);
												variablesTable.fnUpdate($(
														"#eexp").val(),
														selectedRow, 1);
												$(this).dialog("close");
											},
											"Remove variable" : function() {
												var rctr = 0;
												variablesTable.each(function(
														index, item) {
													rctr++;
												});
												if (rctr > 1) {
													variablesTable
															.fnDeleteRow(selectedRow);
													$(this).dialog("close");
												} else {
													alert("At least one Analysis variable should be defined");
												}
											}
										}
									});

					notesDialog = $("#ModelNotes").dialog({
						autoOpen : false,
						modal : true,
						width : 600,
						buttons : {
							Ok : function() {
								$(this).dialog("close");
							}
						}
					});

					$("#demoForm").formwizard("option", "remoteAjax",
							remoteAjax);
					// set the remoteAjax option for the wizard

				});
