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
var name;

function loadStyleSheet(sheet, callback, reload, remaining) {
    var sheetName = name.slice(0, name.lastIndexOf('/') + 1) + sheet.href;
    var input = readFile(sheetName);
    var parser = new less.Parser({
        paths: [sheet.href.replace(/[\w\.-]+$/, '')]
    });
    parser.parse(input, function (e, root) {
        if (e) {
            print("Error: " + e);
            quit(1);
        }
        callback(root, sheet, { local: false, lastModified: 0, remaining: remaining });
    });

    // callback({}, sheet, { local: true, remaining: remaining });
}

function writeFile(filename, content) {
    var fstream = new java.io.FileWriter(filename);
    var out = new java.io.BufferedWriter(fstream);
    out.write(content);
    out.close();
}

// Command line integration via Rhino
(function (args) {
    name = args[0];
    var output = args[1];

    if (!name) {
        print('No files present in the fileset; Check your pattern match in build.xml');
        quit(1);
    }
    path = name.split("/");path.pop();path=path.join("/")

    var input = readFile(name);

    if (!input) {
        print('lesscss: couldn\'t open file ' + name);
        quit(1);
    }

    var result;
    var parser = new less.Parser();
    parser.parse(input, function (e, root) {
        if (e) {
            quit(1);
        } else {
            result = root.toCSS();
            if (output) {
                writeFile(output, result);
                print("Written to " + output);
            } else {
                print(result);
            }
            quit(0);
        }
    });
    print("done");
}(arguments));
