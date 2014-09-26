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
    Transforms to IE filters

*/
define([],function(){

	/*var filters = {
		return "filter: progid:DXImageTransform.Microsoft.DropShadow(
		
	}*/
	return {
		onProperty: function(name, value){
			var parts = value.split(/\s+/);
			if(name == "box-shadow"){
				var offX = parseFloat(parts[0]);
				var offY = parseFloat(parts[1]);
				var strength = Math.sqrt(offX*offX + offY*offY);
				var direction = (offY > 0 ? 180 : 360) - Math.atan(offX/offY) * 180 / Math.PI;
				return "filter: progid:DXImageTransform.Microsoft.Shadow(strength=" + strength + ",direction=" + direction + ",color='" + parts[3] + "');"
			}
			if(name == "transform" && value.match(/rotate/)){
				var angle = value.match(/rotate\(([-\.0-9]+)deg\)/)[1] / 180 * Math.PI;
				var cos = Math.cos(angle);
				var sin = Math.sin(angle);
				return "filter: progid:DXImageTransform.Microsoft.Matrix(" + 
                     "M11=" + cos +", M12=" + (-sin) + ",M21=" + sin + ", M22=" + cos + ", sizingMethod='auto expand');";
			}
		}
	};
});

