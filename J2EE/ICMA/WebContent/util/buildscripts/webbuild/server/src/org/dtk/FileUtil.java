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

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.util.zip.GZIPOutputStream;

public class FileUtil {

    public static String readFromFile(String path, String encoding) throws IOException {
        // summary: reads a file and returns a string
        if (encoding == null) {
            encoding = "utf-8";
        }
        File file = new File(path);
        String lineSeparator = System.getProperty("line.separator");
        BufferedReader input = new java.io.BufferedReader(
                new InputStreamReader(new FileInputStream(file), encoding));
        try {
            StringBuffer stringBuffer = new StringBuffer();
            String line = input.readLine();
        
            // Byte Order Mark (BOM) - The Unicode Standard, version 3.0, page
            // 324
            // http://www.unicode.org/faq/utf_bom.html
        
            // Note that when we use utf-8, the BOM should appear as "EF BB BF",
            // but it doesn't due to this bug in the JDK:
            // http://bugs.sun.com/bugdatabase/view_bug.do?bug_id=4508058
            if (line != null && line.length() > 0 && line.charAt(0) == 0xfeff) {
                // Eat the BOM, since we've already found the encoding on this
                // file,
                // and we plan to concatenating this buffer with others; the BOM
                // should
                // only appear at the top of a file.
                line = line.substring(1);
            }
            while (line != null) {
                stringBuffer.append(line);
                stringBuffer.append(lineSeparator);
                line = input.readLine();
            }
            // Make sure we return a JavaScript string and not a Java string.
            return stringBuffer.toString(); // String
        } finally {
            input.close();
        }
    }

    public static void writeToFile(String path, String contents, String encoding, boolean useGzip) throws IOException {
        // summary: writes a file
        if (encoding == null) {
            encoding = "utf-8";
        }
        File file = new File(path);
        
        //Make sure destination dir exists.
        File parentDir = file.getParentFile();
        if(!parentDir.exists()){
            if(!parentDir.mkdirs()){
                throw new IOException("Could not create directory: " + parentDir.getAbsolutePath());
            }
        }

        OutputStream outStream = new FileOutputStream(file);
        if (useGzip) {
            outStream = new GZIPOutputStream(outStream);
        } else {
            
        }
 
        BufferedWriter output = new java.io.BufferedWriter(new OutputStreamWriter(outStream, encoding));
        try {
            output.append(contents);
        } finally {
            output.close();
        }            

    }
}
