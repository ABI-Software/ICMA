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
jsdoc = {nodes: {}};

dojo.addOnLoad(function(){
	dojo.query("#jsdoc-manage table").forEach(function(table){
		dojo.connect(dojo.byId("jsdoc-manage"), "onsubmit", function(e){
			var valid = true;
			dojo.query("select", table).forEach(function(select){
				if(select.options.length > 1 && select.selectedIndex == 0){
					valid = false;
				}
			});
			if(!valid){
				alert("All variables must either be marked as new, or used in a rename.");
				dojo.stopEvent(e);
			}
		});

		var available = {};

		dojo.query("input", table).forEach(function(checkbox){
			checkbox.checked = true;
			var parts = checkbox.value.split("|");
			var node = {
				project: parts[0],
				resource: parts[1],
				title: parts[2],
				nid: parts[3],
				vid: parts[4]
			}
			jsdoc.nodes[node.nid + "_" + node.vid] = node;
			dojo.connect(checkbox, "onchange", function(e){
				var checked = e.target.checked;

				if(!available[node.project]){
					e.target.checked = true;
				}
				if(available[node.project] || checked){
					dojo.publish("/jsdoc/onchange", [checkbox.checked, node.nid + "_" + node.vid]);
				}

				if(!checked && available[node.project]){
					--available[node.project];
				}else if(checked) {
					++available[node.project];
				}
			});
		});

		dojo.query("select", table).forEach(function(select){
			var project = select.name.slice(9, select.name.indexOf("]"));
			available[project] = (available[project] || 0) + 1;

			dojo.connect(select, "onchange", function(){
				if(select.selectedIndex == 0){
					if(select.last){
						dojo.publish("/jsdoc/onchange", [false, select.last, select]);
						select.last = 0;
					}
				}else if(select.selectedIndex > 0){
					if(select.last){
						dojo.publish("/jsdoc/onchange", [false, select.last, select]);
					}
					var option = select.options[select.selectedIndex];
					select.last = option.value;
					dojo.publish("/jsdoc/onchange", [true, option.value, select]);
				}
			});

			dojo.subscribe("/jsdoc/onchange", null, function(checked, id, current){
				if(current === select){
					return;
				}

				var node = jsdoc.nodes[id];

				if(!checked){
					if(select.name.indexOf("modified[" + node.project + "]") == 0){
						var i = select.options.length++;
						select.options[i].value = id;
						select.options[i].text = node.title + " in " + node.resource;
					}
				}else{
					dojo.query("option[value=" + id + "]", select).orphan();
					if(!select.options.length){
						select.selectedIndex = 0;
					}
				}
			});
		});
	});
});