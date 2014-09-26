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
var assert = require("assert"),
	put = require("../put");
exports.testSimple = function() {
	assert.equal(put('div span.test<').toString(), '\n<div>\n  <span class="test"></span>\n</div>');
	assert.equal(put('div', ['header', 'section']).toString(), '\n<div>\n  <header></header>\n  <section></section>\n</div>');
};
exports.testPage = function() {
	put.indentation = false;
	var page = put('html');
	put(page, 'head script[src=test.js]+link[href=test.css]+link[href=test2.css]');
	var content = put(page, 'body div.header $\
			+div.content', 'Hello World');
	put(content, 'div.left', 'Left');
	put(content, 'div.right', {innerHTML: 'Right <b>text</b>'});
	assert.equal(page.toString(), '<!DOCTYPE html>\n<html><head><script src=\"test.js\"></script><link href=\"test.css\"><link href=\"test2.css\"></head><body><div class=\"header\">Hello World</div><div class=\"content\"><div class=\"left\">Left</div><div class=\"right\">Right <b>text</b></div></div></body></html>');
};
exports.testStream = function() {
	//put.indentation = '  ';
	var output = '';
	var stream = {
		write: function(str){
			output += str;
		},
		end: function(str){
			output += str;
		}
	}
	var page = put('html').sendTo(stream);
	put(page, 'head script[src=test.js]+link[href=test.css]+link[href=test2.css]');
	var content = put(page, 'body div.header $\
			+div.content', 'Hello World');
	content.put('div.left', 'Left');
	content.put('div.right', {innerHTML: 'Right <b>text</b>'});
	page.end();
	assert.equal(output, '<!DOCTYPE html>\n<html><head><script src=\"test.js\"></script><link href=\"test.css\"><link href=\"test2.css\"></head><body><div class=\"header\">Hello World</div><div class=\"content\"><div class=\"left\">Left</div><div class=\"right\">Right <b>text</b></div></div></body>\n</html>');
};
if (require.main === module)
    require("patr/runner").run(exports);