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
function writeFile(filename, contents, encoding, cb) {
	if (arguments.length==3 && typeof encoding!="string") {
		cb = encoding;
		encoding = 0;
	}
	var
		outFile = new java.io.File(filename),
		outWriter;
	if(encoding){
		outWriter = new java.io.OutputStreamWriter(new java.io.FileOutputStream(outFile), encoding);
	}else{
		outWriter = new java.io.OutputStreamWriter(new java.io.FileOutputStream(outFile));
	}

	var os = new java.io.BufferedWriter(outWriter);
	try{
		os.write(contents);
	}finally{
		os.close();
	}
	if (cb) {
		cb(0);
	};
}

var built = "//>>built\n";

function sscompile(src, dest, optimizeSwitch, copyright){
	// decode the optimize switch
	var
		options = optimizeSwitch.split("."),
		comments = 0,
		keepLines = 0,
		strip = null;
	while(options.length){
		switch(options.pop()){
			case "normal":
				strip = "normal";
				break;
			case "warn":
				strip = "warn";
				break;
			case "all":
				strip = "all";
				break;
			case "keeplines":
				keepLines = 1;
				break;
			case "comments":
				comments = 1;
				break;
		}
	}

	//Use rhino to help do minifying/compressing.
	var context = Packages.org.mozilla.javascript.Context.enter();
	try{
		// Use the interpreter for interactive input (copied this from Main rhino class).
		context.setOptimizationLevel(-1);

		var text = readFile(src, "utf-8");
		if(comments){
			//Strip comments
			var script = context.compileString(text, dest, 1, null);
			text = new String(context.decompileScript(script, 0));

			//Replace the spaces with tabs.
			//Ideally do this in the pretty printer rhino code.
			text = text.replace(/	 /g, "\t");
		}else{
			//Apply compression using custom compression call in Dojo-modified rhino.
			text = new String(Packages.org.dojotoolkit.shrinksafe.Compressor.compressScript(text, 0, 1, strip));
			if(!keepLines){
				text = text.replace(/[\r\n]/g, "");
			}
		}
	}finally{
		Packages.org.mozilla.javascript.Context.exit();
	}
	writeFile(dest, copyright + built + text, "utf-8");
}

var JSSourceFilefromCode, closurefromCode, jscomp = 0;
function ccompile(src, dest, optimizeSwitch, copyright){
	if(!jscomp){
		// don't do this unless demanded...it may not be available
		JSSourceFilefromCode=java.lang.Class.forName('com.google.javascript.jscomp.JSSourceFile').getMethod('fromCode',[java.lang.String,java.lang.String]);
		closurefromCode = function(filename,content){
			return JSSourceFilefromCode.invoke(null,[filename,content]);
		};
		jscomp = com.google.javascript.jscomp;
	}
	//Fake extern
	var externSourceFile = closurefromCode("fakeextern.js", " ");

	//Set up source input
	var jsSourceFile = closurefromCode(String(dest), String(readFile(src, "utf-8")));

	//Set up options
	var options = new jscomp.CompilerOptions();
	options.prettyPrint = optimizeSwitch.indexOf(".keeplines") !== -1;

	var FLAG_compilation_level = jscomp.CompilationLevel.SIMPLE_OPTIMIZATIONS;
	FLAG_compilation_level.setOptionsForCompilationLevel(options);
	var FLAG_warning_level = jscomp.WarningLevel.DEFAULT;
	FLAG_warning_level.setOptionsForWarningLevel(options);

	//Run the compiler
	var compiler = new Packages.com.google.javascript.jscomp.Compiler(Packages.java.lang.System.err);
	var result = compiler.compile(externSourceFile, jsSourceFile, options);
	writeFile(dest, copyright + built + compiler.toSource(), "utf-8");
}


var
	console = new java.io.BufferedReader(new java.io.InputStreamReader(java.lang.System["in"])),
	readLine = function(){
		// the + "" convert to a Javascript string
		return console.readLine() + "";
	},
	src, dest, optimizeSwitch, copyright;

while(1){
	// the + "" convert to a Javascript string
	src = readLine();
	if(src=="."){
		break;
	}
	dest = readLine();
	optimizeSwitch = readLine();
	copyright = eval(readLine());
	print(dest + ":");
	var start = (new Date()).getTime(),
		exception = "";
	try{
		if(/closure/.test(optimizeSwitch)){
			ccompile(src, dest, optimizeSwitch, copyright);
		}else{
			sscompile(src, dest, optimizeSwitch, copyright);
		}
	}catch(e){
		exception = ". OPTIMIZER FAILED: " + e;
	}
	print("Done (compile time:" + ((new Date()).getTime()-start)/1000 + "s)" + exception);
}

