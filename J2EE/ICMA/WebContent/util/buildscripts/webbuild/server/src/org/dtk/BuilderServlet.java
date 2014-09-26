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

import java.io.BufferedInputStream;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.io.PrintWriter;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

import javax.servlet.ServletContext;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import org.mozilla.javascript.ContextFactory;

import sun.misc.BASE64Encoder;

public class BuilderServlet extends HttpServlet {
    private String builderPath;
    private String cachePath;
 
    // *****************************************************
    public void init() throws ServletException {
        super.init();

        // Read in the builderPath.
        // - context-param
        // - Java system property
        ServletContext sc = getServletContext();
        builderPath = sc.getInitParameter("builderPath");
        cachePath = sc.getInitParameter("cachePath");
       if (builderPath == null || builderPath.length() == 0) {
            builderPath = System.getProperty("builderPath");
            cachePath = System.getProperty("cachePath");
       }
       if (cachePath == null || cachePath.length() == 0) {
           cachePath = System.getProperty("cachePath");
       }
    }

    // *****************************************************
    public void doPost(HttpServletRequest req, HttpServletResponse res)
            throws ServletException, IOException {
        doGet(req, res);
    }

    // *****************************************************
    public void doGet(HttpServletRequest req, HttpServletResponse res)
            throws ServletException, IOException {

        //Pull out the parameters.
        String version = req.getParameter("version");
        String cdn = req.getParameter("cdn");
        String dependencies = req.getParameter("dependencies");
        String optimize = req.getParameter("optimize");
        
        String cacheFile = null;
        String result = null;
        boolean isCached = false;

        Boolean isError = true;

        //Validate parameters
        if(!version.equals("1.3.2")) {
            result = "invalid version: " + version;
        }
        if(!cdn.equals("google") && !cdn.equals("aol")) {
            result = "invalide CDN type: " + cdn;
        }
        if(!optimize.equals("comments") && !optimize.equals("shrinksafe")
            && !optimize.equals("none") && !optimize.equals("shrinksafe.keepLines")) {
            result = "invalid optimize type: " + optimize;
        }
        if(!dependencies.matches("^[\\w\\-\\,\\s\\.]+$")) {
            result = "invalid dependency list: " + dependencies;
        }

        try {
            //See if we already did the work.
            MessageDigest md = null;
            try {
                md = MessageDigest.getInstance("SHA");
            } catch (NoSuchAlgorithmException e) {
                result = e.getMessage();
            }
            if (result == null) {
                md.update(dependencies.getBytes());
                String digest = (new BASE64Encoder()).encode(md.digest()).replace('+', '~').replace('/', '_').replace('=', '_');
                cacheFile = cachePath + "/" + version + "/" + cdn + "/" + digest + "/" + optimize + ".js";
                
                File file = new File(cacheFile);
                if (file.exists()) {
                    isCached = true;
                    isError = false;
                }
            }
    
            //Generate the build.
            if (result == null && !isCached) {
                BuilderContextAction contextAction = new BuilderContextAction(builderPath, version, cdn, dependencies, optimize);
                ContextFactory.getGlobal().call(contextAction);
                Exception exception = contextAction.getException();
    
                if (exception != null) {
                    result = exception.getMessage();
                } else {
                    result = contextAction.getResult();
                    FileUtil.writeToFile(cacheFile, result, null, true);
                    isError = false;
                }
            }
        } catch (Exception e) {
            result = e.getMessage();
        }

        //Write out response.
        res.setCharacterEncoding("utf-8");
        if (isError) {
            result = result.replaceAll("\\\"", "\\\"");
            result = "<html><head><script type=\"text/javascript\">alert(\"" + result + "\");</script></head><body></body></html>";
            PrintWriter writer = res.getWriter();
            writer.append(result);
        } else {
            res.setHeader("Content-Type", "application/x-javascript");
            res.setHeader("Content-disposition", "attachment; filename=dojo.js");  
            res.setHeader("Content-Encoding", "gzip");

            //Read in the gzipped bytes of the cached file.
            File file = new File(cacheFile);

            BufferedInputStream in = new java.io.BufferedInputStream(
                    new DataInputStream(new FileInputStream(file)));
            OutputStream out = res.getOutputStream();
            byte[] bytes = new byte[64000];
            int bytesRead = 0;
            while (bytesRead != -1) {
                bytesRead = in.read(bytes);
                if (bytesRead != -1) {
                    out.write(bytes, 0, bytesRead);
                }
            }
        }
    }
}
