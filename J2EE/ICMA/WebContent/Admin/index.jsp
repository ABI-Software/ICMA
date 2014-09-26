<%@page import="javax.naming.InitialContext"%>
<%@ page language="java" contentType="text/html; charset=UTF-8"  pageEncoding="UTF-8"%>
<!DOCTYPE html>
<html>
<%
boolean allowLocalUpload = false;
try{
	InitialContext ic = new InitialContext();
	allowLocalUpload = ((Boolean) ic.lookup("java:global/ICMAALLOWLOCALDICOMLOADING")).booleanValue();
}catch(Exception exx){
	
}
%>
<head>
<meta charset="UTF-8">
<link type="text/css" href="./css/cupertino/jquery-ui-1.7.2.custom.css"
	rel="stylesheet" />
<link type="text/css" href="./css/demo_page.css" rel="stylesheet" />
<link type="text/css" href="./css/demo_table.css" rel="stylesheet" />
<link type="text/css" href="./css/uploadfile.min.css" rel="stylesheet" />
<style type="text/css">
</style>
<script src="./js/jquery-1.9.1.min.js" type="text/javascript"></script>
<script src="./js/jquery.dataTables.min.js" type="text/javascript"></script>
<script src="./js/jquery-ui-1.10.1.custom.min.js" type="text/javascript"></script>
<script src="./js/jquery.uploadfile.min.js" type="text/javascript"></script>
<!-- <script src="./js/jquery.MultiFile.pack.js" type="text/javascript" language="javascript"></script>  -->
<script src="./js/adminconsole.js" type="text/javascript"></script>
<title>ICMA Admin Console</title>
</head>
<body class="ui-widget">
<div class="ui-widget-header">
<h1>Integrated Cardiac Modelling and Analysis</h1>
<div class="ui-widget-content">
<input type="button" value="Logout" id="logout" style="float:right"/>
</div>
<h2>Patient Record administration console</h2>
</div>
<div class="ui-widget-content">
<table id="patients_table" class="display" cellpadding="0" cellspacing="1">
  <thead>
    <tr>
      <th title="Patient ID">Patient ID</th>
      <th title="Patient Name">Name</th>
      <th title="Data Location" >Location</th>
      <th title="Action">Action</th>
      <th title="Batch Action">Group</th>
      </tr>
  </thead>
  <tbody>
		<tr>
			<td colspan="5" class="dataTables_empty">Loading data from server</td>
		</tr>
  </tbody>
</table>
</div>
<div class="ui-widget-content" style="float:right;">
<input type="button" id="batchAdd" value="Sync with ICMA" />
<input type="button" id="batchRemove" value="Remove from ICMA"/ >
</div>
<%
if(allowLocalUpload){
%>
<div class="ui-widget-content" >
<div style="margin-top:30px">
<p>Upload dicom files from local storage to ICMA PACS. If there are multiple files, drag and drop them in the file area.</p>
<button id="Upload">Upload selected files</button>
<div id="fileupload"/>
<!-- <form action="../loadDicomFiles" method="post"
enctype="multipart/form-data">
<label for="file">Folder</label>
<input type="file" class="multi">
</form> -->
</div>
</div>
<%
}
%>

<div id="busyDialog" title="Server operation launched">
  <p>Submitting request to server, please wait</p>
  <img src="./images/ajax-loader.gif" style="display: block; margin-left: auto; margin-right: auto;">
  </div>
</body>
</html>