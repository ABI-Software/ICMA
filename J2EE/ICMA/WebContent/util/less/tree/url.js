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
(function (tree) {

tree.URL = function (val, paths) {
    if (val.data) {
        this.attrs = val;
    } else {
        // Add the base path if the URL is relative and we are in the browser
        if (typeof(window) !== 'undefined' && !/^(?:https?:\/\/|file:\/\/|data:|\/)/.test(val.value) && paths.length > 0) {
            val.value = paths[0] + (val.value.charAt(0) === '/' ? val.value.slice(1) : val.value);
        }
        this.value = val;
        this.paths = paths;
    }
};
tree.URL.prototype = {
    toCSS: function () {
        return "url(" + (this.attrs ? 'data:' + this.attrs.mime + this.attrs.charset + this.attrs.base64 + this.attrs.data
                                    : this.value.toCSS()) + ")";
    },
    eval: function (ctx) {
        return this.attrs ? this : new(tree.URL)(this.value.eval(ctx), this.paths);
    }
};

})(require('../tree'));
