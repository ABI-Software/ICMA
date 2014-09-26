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
/*
    Handles gradients
*/
define([],function(vendor){
	var colorString = /#(\w{6})/;
	var createGradient = {
		"-webkit-": function(type, position, from, to){
			return "background-image: -webkit-gradient(" + type.substring(0, 6) + ", left top, left bottom, from(" + from + "), to(" + to + "))";
		},
		"-moz-": function(type, position, from, to){
			return "background-image: -moz-" + type + "(" + position + "," + from + "," + to + ")";
		},
		"-o-": function(type, position, from, to){
			return "background-image: -o-" + type + "(" + position + "," + from + "," + to + ")";
		},
		"-ms-": function(type, position, from, to){
			
			from = from.match(colorString);
			to = to.match(colorString);
			if(from && to){ 
				// must disable border radius for IE
				return "border-radius: 0px; filter: progid:DXImageTransform.Microsoft.gradient(startColorstr=#FF" + from[1] + ",endColorstr=#FF" + to[1] +",gradientType=" + (position=="left" ? 1 : 0) + ");";
			}
		}
	}[vendor.prefix];
	return {
		onIdentifier: function(name, value, rule){
			var parts = value.match(/(\w+-gradient)\(([^\)]*)\)/);
			var type = parts[1];
			var args = parts[2].split(/,\s*/);
			var position = args[0];
			var start = args[1];
			var end = args[2];
			return createGradient(type, position, start, end);
		}
	};
});

