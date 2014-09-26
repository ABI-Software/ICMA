<%@page import="java.util.List"%>
<%@page import="nz.ac.auckland.abi.businesslogic.DataViewManagerRemote"%>
<%@page import="nz.ac.auckland.abi.entities.FEMModel"%>
<%@page import="javax.naming.InitialContext"%>
<%@ page language="java" contentType="text/html; charset=UTF-8"
    pageEncoding="UTF-8"%>
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
<%

String patientID = request.getParameter("patientID"); 
String divID = patientID.replaceAll("\\:", "c").replaceAll("/", "s").replaceAll("\\.", "d");
try{

	InitialContext ic = new InitialContext();
	DataViewManagerRemote viewManager = (DataViewManagerRemote) ic.lookup("java:global/ICMAJboss/ICMAEJB/DataViewManager!nz.ac.auckland.abi.businesslogic.DataViewManagerRemote");
	List<String> models = viewManager.getPatientModels(patientID);
%>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<script type="text/javascript">

cTable = $("#models<%=divID%>").dataTable({
	"bPaginate" : false,
	"bLengthChange" : true,
	"bFilter" : true,
	"bSort" : true,
	"bInfo" : false,
	"bAutoWidth" : false,
	"bJQueryUI" : true,
});
</script>
<script src="./js/modeladminconsole.js" type="text/javascript"></script>	
<title>Models view for <%=patientID%></title>
</head>
<body>
<% 
	if(models!=null){
		
%>
	<table id="models<%=divID%>" >
	<thead>	
		<tr>
			<th>Model name</th>
			<th>Action</th>
		</tr>
	</thead>
<%
	int ctr = 0;
	for(String model : models){
		String toks[] = model.split(":");
%>	
	<tr>
		<td><%=toks[1]%></td>
		<td>
		<div>
		<input type="button" value="Delete" onclick="submitModelDelete('<%=toks[0]%>',<%=ctr%>)"/>
		<input type="button" value="Export" onclick="submitModelExport('<%=toks[0]%>')"/>
		</div>
		</td>
	</tr>
<%
	ctr++;
	}
%>
	</table>
<%
	}else{
%>
	<h3>No models exist for the patient with ID <%=patientID%></h3>
<%
	}
%>
</body>
<%
	}catch(Exception exx){
%>
<title>Models view for <%=patientID%></title>
</head>
<body>
	<h2>Server error: <%=exx.toString()%></h2>
</body>
<%		
	}
%>
</html>