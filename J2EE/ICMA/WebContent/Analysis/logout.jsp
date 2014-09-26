<%@ page language="java" contentType="text/html; charset=UTF-8"
    pageEncoding="UTF-8"%>
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<meta charset="UTF-8">
<style type="text/css">

/*******************
FONTS
*******************/

@font-face {
  font-family: 'Bree Serif';
  font-style: normal;
  font-weight: 400;
  src: local('Bree Serif'), local('BreeSerif-Regular'), url(http://themes.googleusercontent.com/static/fonts/breeserif/v2/LQ7WLTaITDg4OSRuOZCpsxsxEYwM7FgeyaSgU71cLG0.woff) format('woff');
}

/*******************
BODY STYLING
*******************/

* {
	margin: 0;
	padding: 0;
	border: 0;
}

body {
	background: #dadada;
	font-family: "HelveticaNeue-Light", "Helvetica Neue Light", "Helvetica Neue", Helvetica, Arial, "Lucida Grande", sans-serif;
	font-weight:300;
	text-align: left;
	text-decoration: none;
}

#wrapper {
	/* Center wrapper perfectly */
	width: 300px;
	height: 400px;
	position: absolute;
	left: 50%;
	top: 50%;
	margin-left: -150px;
	margin-top: -200px;
}

.gradient {
	/* Center Positioning */
	width: 600px;
	height: 600px;
	position: fixed;
	left: 50%;
	top: 50%;
	margin-left: -200px;
	margin-top: -200px;

}


</style>
<meta http-equiv=refresh content="2, URL=./index.jsp">
<title>Integrated Cardiac Modeling and Analysis</title>
</head>
<body onload="javascript:window.history.forward(1);">
<%
     session.invalidate();       
%>
 
 <center><h1>Logged out successfully</h1></center>
 </body>
</html>