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
package org.dtk;

import org.mozilla.javascript.Context;
import org.mozilla.javascript.ContextAction;
import org.mozilla.javascript.Function;
import org.mozilla.javascript.Script;
import org.mozilla.javascript.Scriptable;
import org.mozilla.javascript.ScriptableObject;

public class BuilderContextAction implements ContextAction {
    private String builderPath;
    private String version;
    private String cdn;
    private String dependencies;
    private String optimize;

    private Exception exception;
    private Scriptable topScope;
    private Context context;
    private String result;

    public BuilderContextAction(String builderPath, String version, String cdn, String dependencies, String optimize) {
        this.builderPath = builderPath;
        this.version = version;
        this.cdn = cdn;
        this.dependencies = dependencies;
        this.optimize = optimize;

        this.exception = null;
        this.context = null;
        this.topScope = null;
    }

    public Object run(Context newContext) {
        context = newContext;
        context.setOptimizationLevel(-1);

        // Set up standard scripts
        topScope = context.initStandardObjects();

        try {
            String fileName = builderPath + "build.js";
            String fileContent = FileUtil.readFromFile(fileName, null);
            Script script = context.compileString(fileContent, fileName, 1, null);
            
            // Expose top level scope as the "global" scope.
            //TODO: only need this for the load function, maybe there is a built in way
            //to get this.
            ScriptableObject.putProperty(topScope, "global", Context.javaToJS(topScope, topScope));

            // Exec the build script.
            script.exec(context, topScope);

            // Call build.make(builderPath)
            Scriptable build = Context.toObject(topScope.get("build", topScope), topScope);
            Object args[] = {
                    builderPath,
                    version,
                    cdn,
                    dependencies,
                    optimize
            };
            Function make = (Function) build.get("make", topScope);
            Object resultObj = make.call(context, topScope, build, args);
            result = (String) Context.jsToJava(resultObj, String.class); 
        } catch (Exception e) {
            this.exception = e;

        }
        return null;
    }

    public String getResult() {
        return result;
    }

    public Exception getException() {
        return exception;
    }
}
