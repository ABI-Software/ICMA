<%@ page language="java" contentType="text/html; charset=UTF-8"
    pageEncoding="UTF-8"%>
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html >
<head>
<title>Integrated Cardiac Modeling and Analysis</title>

<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1, minimum-scale=1,user-scalable=no"/>

<link rel="stylesheet" href="ABI/app/ICMA/Main.css" type="text/css" media="screen" title="no title" charset="utf-8">

<script src="kinetic-v5.1.0.min.js" type="text/javascript"></script>

<script type="text/javascript" src="dojo/dojo.js" data-dojo-config="mblUserAgent: 'IPhone', async: 1, parseOnLoad: true, paths: { 'dgrid': '../dgrid', 'put-selector': '../put-selector', 'xstyle': '../xstyle', 'ABI': '../ABI'}" ></script>

<style type="text/css">
  html { height: 100% }
  body { height: 100%; margin: 0; padding: 0; overflow: hidden}
</style>

<script>
<%
	response.setHeader("Cache-Control","no-cache"); 
	response.setHeader("Pragma","no-cache"); 
	response.setDateHeader ("Expires", -1);
%>
    var username = '<%=request.getRemoteUser()%>'; 
	require([
			"dojo/ready",
			"ABI/app/ICMA/Main"
		],function(ready, Main){ 
			ready(function(){
				ABI.app.ICMA.MainSingleton.startup();
			});
	  	}
	); 
</script>

</head>

<body class="claro">
</body>

</html>