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
define(['../elemental'], function(elemental){
	function parse(value, callback){
		var Class, prototype;
		if(value.eachProperty){
			var type, props = {/*cssText: value.cssText*/};
			value.eachProperty(function(name, value){
				name = name.replace(/-\w/g, function(dashed){
					return dashed.charAt(1).toUpperCase();
				});
				value = parse(value);
				if(name == "type" && callback){
					type = value;
				}else{
					props[name] = value;
				}
			});
			value = props;
			// load the class, and adjust the property types based on the class prototype  
			if(type){
				require(type.split(/[, ]+/), function(Class, Mixin){
					if(Mixin){
						// more than one, mix them together
						// TODO: This should be Class.extend(arguments.slice(1)), but dojo.declare has a bug in extend that causes it modify the original
						Class = dojo.declare([].slice.call(arguments,0)); // convert the arguments to an array of mixins
					}
					var prototype = Class.prototype;
					for(var name in props){
						var value = props[name];
						if(name in prototype){
							var type = typeof prototype[name];
							if(type == "string" || typeof value != "string"){
							}else if(type == "number"){
								props[name] = +value;
							}else{
								props[name] = eval(value);
							}
						}
					}
					callback(function(element){
						new Class(props, element);
					});
				});
			}
		}else if(value.charAt(0) == "'" || value.charAt(0) == '"'){
			value = eval(value);
		}else if(!isNaN(value)){
			value = +value;
		}
		return value;
	}
	
	function Widget(scope){
		return {
			widget: function(value, rule){
				var modules = [];
				value.replace(/require\s*\(\s*['"]([^'"]*)['"]\s*\)/g, function(t, moduleId){
					modules.push(moduleId);
				});
				require(modules);
				return function(domNode){
					require(modules, function(){
						with(scope){
							var __module = eval(value);
							var prototype = __module.prototype;
							var props = {};
							if(prototype){
								rule.eachProperty(function(t, name, value){
									if(name in prototype){
										var type = typeof prototype[name];
										if(type == "string" || typeof value != "string"){
											props[name] = value;
										}else if(type == "number"){
											props[name] = +value;
										}else{
											props[name] = eval(value);
										}
									}
								});
							}
							__module(props, domNode);
						}
					});
				};
			},
			role: "layout"
		};
	}
	var def = new Widget({});
	Widget.widget = def.widget;
	Widget.role = def.role;
	return {
		onProperty: function(name, value, rule){
			// used for a widget property:
			//	widget: {
			//		type: 'dijit/form/Button';
			//		label: 'Save';
			//	}
			return {
				then: function(callback){
					parse(value, function(renderer){
						elemental.addRenderer(name, value, rule, renderer);
						callback();
					}); 
				}
			}
		}/*,
		onFunction: function(name, propertyName, value){
			// this allows us to create a CSS widget function
			// x-property{
			// 		my-widget: widget(my/Widget);
			//	}
			//	.class{
			//		my-widget: 'settings';
			//	}
			return function(name, propertyValue){
				require([value], function(Class){
					elemental.addRenderer(rule, function(element){
						new Class(parse(propertyValue), element);
					});
				});
			};
		}*/
		
	} 
})