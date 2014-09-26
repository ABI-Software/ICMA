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
var profile = (function(){
	return {
		// relative to this file
		basePath:"../../..",

		// relative to base path
		releaseDir:"./dojo-amd",

		packages:[{
			name:"dojo",
			location:"./dojo"
		},{
			name:"dijit",
			location:"./dijit"
		}],

		selectorEngine:"lite",

		defaultConfig:{
			hasCache:{
				// these are the values given above, not-built client code may test for these so they need to be available
				'dojo-built':1,
				'dojo-loader':1,
				'dom':1,
				'host-browser':1,

				// default
				"config-selectorEngine":"lite"
			},
			async:1
		},

		dojoBootText:"require.boot && require.apply(null, require.boot);",

		// since this build it intended to be utilized with properly-expressed AMD modules;
		// don't insert absolute module ids into the modules
		insertAbsMids:0,

		// these are all the has feature that affect the loader and/or the bootstrap
		// the settings below are optimized for the smallest AMD loader that is configurable
		// and include dom-ready support
		staticHasFeatures:{
			// dojo/dojo
			'config-dojo-loader-catches':0,

			// dojo/dojo
			'config-tlmSiblingOfDojo':0,

			// dojo/dojo
			'dojo-amd-factory-scan':0,

			// dojo/dojo
			'dojo-combo-api':0,

			// dojo/_base/config, dojo/dojo
			'dojo-config-api':1,

			// dojo/main
			'dojo-config-require':0,

			// dojo/_base/kernel
			'dojo-debug-messages':0,

			// dojo/dojo
			'dojo-dom-ready-api':1,

			// dojo/main
			'dojo-firebug':0,

			// dojo/_base/kernel
			'dojo-guarantee-console':1,

			// dojo/has
			'dojo-has-api':1,

			// dojo/dojo
			'dojo-inject-api':1,

			// dojo/_base/config, dojo/_base/kernel, dojo/_base/loader, dojo/ready
			'dojo-loader':1,

			// dojo/dojo
			'dojo-log-api':0,

			// dojo/_base/kernel
			'dojo-modulePaths':0,

			// dojo/_base/kernel
			'dojo-moduleUrl':0,

			// dojo/dojo
			'dojo-publish-privates':0,

			// dojo/dojo
			'dojo-requirejs-api':0,

			// dojo/dojo
			'dojo-sniff':0,

			// dojo/dojo, dojo/i18n, dojo/ready
			'dojo-sync-loader':0,

			// dojo/dojo
			'dojo-test-sniff':0,

			// dojo/dojo
			'dojo-timeout-api':0,

			// dojo/dojo
			'dojo-trace-api':0,

			// dojo/dojo
			'dojo-undef-api':0,

			// dojo/i18n
			'dojo-v1x-i18n-Api':0,

			// dojo/_base/xhr
			'dojo-xhr-factory':0,

			// dojo/_base/loader, dojo/dojo, dojo/on
			'dom':1,

			// dojo/dojo
			'host-browser':1,

			// dojo/_base/array, dojo/_base/connect, dojo/_base/kernel, dojo/_base/lang
			'extend-dojo':1
		},

		layers:{
			"dojo/dojo":{
				include:[],
				customBase:1
			},
			"dojo/main":{
				include:["dojo/selector/lite"]
			}
		}
	};
})();
