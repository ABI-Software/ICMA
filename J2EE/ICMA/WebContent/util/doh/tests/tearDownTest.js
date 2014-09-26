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
define(["doh/runner"], function(doh) {
	doh.register("doh/tearDownTest",[
		{
			name: "sync_pass_teardown",
			pass: 1,
			runTest: function(){
				doh.debug("test sync_pass_teardown called");
				doh.t(this.pass,"teardown was called out of order???");
				doh.t(1);
			},
			tearDown: function(){
				doh.debug("teardown sync_pass_teardown called");
				this.pass=0;
			}
		},
		{
			name: "sync_fail_teardown",
			pass: 1,
			runTest: function(){
				doh.debug("test sync_fail_teardown called");
				doh.t(this.pass,"teardown was called out of order???");
				doh.t(0,"SHOULD FAIL with 'teardown sync_fail_teardown called' in log");
			},
			tearDown: function(){
				doh.debug("teardown sync_fail_teardown called");
				this.pass=0;
			}
		},
		{
			name: "async_pass_teardown",
			pass: 1,
			runTest: function(){
				doh.debug("test async_pass_teardown called");
				var d = new doh.Deferred();
				var _this=this;
				setTimeout(d.getTestCallback(function(){
					doh.t(_this.pass,"teardown was called out of order???");
				}),900);
				return d;
			},
			tearDown: function(){
				this.pass=0;
				doh.debug("teardown async_pass_teardown called");
			}
		},
		{
			name: "async_fail_teardown",
			pass: 1,
			runTest: function(){
				doh.debug("test async_fail_teardown called");
				var d = new doh.Deferred();
				var _this=this;
				setTimeout(d.getTestCallback(function(){
					doh.t(_this.pass,"teardown was called out of order???");
					doh.t(0,"SHOULD FAIL with 'teardown async_fail_teardown called' in log");
				}),900);
				return d;
			},
			tearDown: function(){
				doh.debug("teardown async_fail_teardown called");
				this.pass=0;
			}
		}
	]);
});