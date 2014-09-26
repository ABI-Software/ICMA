<%@page import="java.util.Set"%>
<%@page import="org.json.simple.JSONObject"%>
<%@page import="nz.ac.auckland.abi.administration.ICMAAccessManager"%>
<%@page import="java.util.List"%>
<%@page import="javax.naming.InitialContext"%>
<%@ page language="java" contentType="text/html; charset=UTF-8"
	pageEncoding="UTF-8"%>
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<link type="text/css" href="./css/cupertino/jquery-ui-1.7.2.custom.css"
	rel="stylesheet" />
<link type="text/css" href="./css/demo_page.css" rel="stylesheet" />
<link type="text/css" href="./css/demo_table.css" rel="stylesheet" />
<link type="text/css" href="./css/jquery.dataTables.css"
	rel="stylesheet" />
<script src="./js/jquery-1.9.1.min.js" type="text/javascript"></script>
<script src="./js/jquery.dataTables.min.js" type="text/javascript"></script>
<script src="./js/jquery-ui-1.10.1.custom.min.js" type="text/javascript"></script>

<%
	try {
		InitialContext ic = new InitialContext();
		ICMAAccessManager accessManager = (ICMAAccessManager) ic
				.lookup("java:global/ICMAJboss/ICMAEJB/ICMAAccessManager!nz.ac.auckland.abi.administration.ICMAAccessManager");
		JSONObject result = accessManager.getUsers();
		String username = request.getRemoteUser();
%>
<script type="text/javascript">
	var selectedUser;
	var selectedRow;
	var cTable;
	var aData;
	$(document).ready(function() {
		cTable = $("#users").dataTable({
			"bPaginate" : true,
			"bLengthChange" : true,
			"bFilter" : true,
			"bSort" : true,
			"bAutoWidth" : false,
			"aoColumns" : [ {
				"sClass" : "control center"
			}, {
				"sClass" : "control center"
			}, ],
		});

		$(".mb").button();
		$( "#busyDialog" ).dialog();
		$( "#busyDialog" ).dialog( "close" );
		
		cTable.on('click', 'tr', function() {
			if ($(this).hasClass('selected')) {
				$(this).removeClass('selected');
			} else {
				cTable.$('tr.selected').removeClass('selected');
				$(this).addClass('selected');
				selectedRow = $(this).index();
				aData = cTable.fnGetData(this); // get datarow
				if (null != aData) // null if we clicked on title row
				{
					//now aData[0] - 1st column(count_id), aData[1] -2nd, etc.
					selectedUser = aData[0];
					$("#username").text(selectedUser);
					$("#controls").show();
				}
			}
		});

		
		$("#logout").button();
		
		$("#logout").click(function(){
			window.open("./logout.jsp",'_self','resizable,location,menubar,toolbar,scrollbars,status');
		});
	});
</script>
<script src="./js/useradminconsole.js" type="text/javascript"></script>
<title>ICMA Users administration</title>
</head>
<body class="ui-widget">
<div class="ui-widget-header">
<h1>Integrated Cardiac Modelling and Analysis</h1>
<div class="ui-widget-content">
<input type="button" value="Logout" id="logout" style="float:right"/>
</div>
<h2>ICMA User administration console</h2>
</div>
	<%
		if (result != null) {
	%>
	<div class="ui-widget-content">
		<table id="users" class="display" cellpadding="0" cellspacing="1">
			<thead>
				<tr>
					<th>User name</th>
					<th>Access Level</th>
				</tr>
			</thead>
			<%
				int ctr = 0;
						Set<String> keys = result.keySet();
						for (String user : keys) {
							if (user.equalsIgnoreCase(username))
								continue;
							String level = (String) result.get(user);
			%>
			<tr>
				<td><%=user%></td>
				<td><%=level.substring(4)%></td>
			</tr>
			<%
				ctr++;
						}
			%>
		</table>
	</div>
	<div class="ui-widget-content" id="controls" style="display: none">
		<div>Change&nbsp;</div>
		<div id="username"></div>
		<input type="button" value="To User" class=".mb"
			onclick="submitRemoveAdmin()" /> <input type="button" class=".mb"
			value="To Admin" onclick="submitAddAdmin()" />
		<div>&nbsp;OR&nbsp;</div>
		<input type="button" value="Remove" class=".mb"
			onclick="submitUserRemove()" />
	</div>
	<div class="ui-widget-content" style="margin-left: auto;margin-right: auto;">
		Create a new User <input type="button" value="Create"
			onclick="createNewUser()" class=".mb" />
	</div>
	<%
		} else {
	%>
	<h3>No other users exist</h3>
	<%
		}
	%>
	<div id="newuser" caption="Login" style="display: none">
		<div>
			<table>
				<tr>
					<td>Username:</td>
					<td><input style="width: 150px;" type="text" name="user"
						id="newusername" /></td>
				</tr>
				<tr>
					<td>Password:</td>
					<td><input style="width: 150px;" type="password"
						id="newuserpass" name="password" /></td>
				</tr>
				<tr>
					<td colspan="2" align="right" valign="bottom"><input
						class=".mb" type="button" id="udetails" value="Create" /></td>
				</tr>
			</table>
		</div>
	</div>
	<div id="busyDialog" title="Server operation launched"
		style="display: none">
		<p>Submitting request to server, please wait</p>
		<img src="./images/ajax-loader.gif"
			style="display: block; margin-left: auto; margin-right: auto;">
	</div>
</body>
<%
	} catch (Exception exx) {
%>
<title>ICMA Users list</title>
</head>
<body>
	<h2>
		Server error:
		<%=exx.toString()%></h2>
</body>
<%
	}
%>
</html>