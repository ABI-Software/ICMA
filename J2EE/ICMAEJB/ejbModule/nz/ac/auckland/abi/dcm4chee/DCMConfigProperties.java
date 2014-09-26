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
package nz.ac.auckland.abi.dcm4chee;


 
 import java.io.File;
 import java.io.IOException;
 import java.io.InputStream;
 import java.net.MalformedURLException;
 import java.net.URI;
 import java.net.URISyntaxException;
 import java.net.URL;
 import java.util.LinkedList;
 import java.util.List;
 import java.util.Properties;
 import java.util.StringTokenizer;
 
 
 public class DCMConfigProperties extends Properties
 {
	private static final long serialVersionUID = 25062011L;
	
   private static String replace(String val, String from, String to)
   {
     return ((from.equals(val)) ? to : val);
   }
   
   /**
    * Create an ConfigurationProperty object (extension of Propreties) without 
    * any Properties included.
    */
   public DCMConfigProperties()
   {
   }

   /**
    * Create an ConfigurationProperty object (extension of Propreties) and load 
    * Properties from given URL.
    *
    * @param url the URL containing the Properties.
    */
   public DCMConfigProperties(URL url)
     throws IOException
   {
     InputStream in = null;
     try {
       load(in = url.openStream());
     } catch (Exception e) {
     }
     finally {
       if (in != null) try {
           in.close();
         }
         catch (IOException ignore)
         {
         }
     }
   }
 
   public String getProperty(String key, String defaultValue, String replace, String to) {
     return replace(getProperty(key, defaultValue), replace, to);
   }

   
   public List<String> tokenize(String s, List<String> result)
   {
     StringTokenizer stk = new StringTokenizer(s, ", ");
     while (stk.hasMoreTokens()) {
       String tk = stk.nextToken();
       if (tk.startsWith("$"))
         tokenize(getProperty(tk.substring(1), ""), result);
       else {
         result.add(tk);
       }
     }
     return result;
   }

   
   public String[] tokenize(String s)
   {
     if (s == null) {
       return null;
     }
     List<String> l = tokenize(s, new LinkedList<String>());
     return ((String[])(String[])l.toArray(new String[l.size()]));
   }

   /**
    * Create a File from an URI.
    * <span style="font-style: italic;">file-uri</span>
    * <p>See the API-Doc of the URI class. For Windows-OS the absolute URI
    * "file:/c:/user/abi/foo.txt" describes the file
    * "C:\\user\\abi\\foo.txt". Relative URI's, e.g. without the "file:"
    * schema-prefix, are relative to the user-directory, given by the system
    * property user.dir.
    * <p>For example: If the user.dir is "C:\\user\\tom\\"
    * and the relative URI is "/abc/foo.txt" the referenced file is
    * "C:\\user\\abi\\abc\\foo.txt". The abbreviations "." for the current
    * and ".." for the upper directory are valid to form a relative URI.
    *
    * @param uriString The string-description of an absolute or relative URI.
    * @return the file which is described by the uriString. Returns null, if
    *         uriString is null or "". Returns null also, if a conversion error occurs.
    */  
   
   public static File uriToFile(String uriString)
   {
     if (uriString == null) {
       return null;
     }
 
     if (uriString.equals("")) {
       return null;
     }
     try
     {
       URI uri = new URI(uriString);
 
       if (!(uri.isAbsolute()))
       {
         URI baseURI = new File(System.getProperty("user.dir")).toURI();
         uri = baseURI.resolve(uri);
       }
 
       return new File(uri); } catch (Exception e) {
     }
     return null;
   }
 
   /**
    * Returns a URL of a reference to a file. If the file reference is a valid
    * absolute URI, this URI is converted directly to a URL. If the file reference
    * is a relative URI this is resolved relative to a given base URL.
    * <p>Example: For a class org.abi.ac.nz.dcm4chee.DCMAccessManager the method call
    * fileRefToURL(CDimseService.class.getResource(""), "resources/identity.p12")
    * results to the URL "file:/D:/DcmServices/build/classes/org/abi/ac/nz/dcm4chee/resources/identity.p12"
    *
    * @param baseURL the base URL to which relative file references are resolved.
    *                May be null, if the fileRef is a absolute reference.
    * @param fileRef the reference to file file. May be an absolute reference
    *                (file:/C:/a/b/c.cfg) or relative reference (b/c.cfg).
    * @return the URL of a file reference. The String representation is of the form "file:/a/b/c.cfg".
    * @throws URISyntaxException if the fileRef is not formed as a URI.
    * @throws MalformedURLException if the fileRef is not a reference to a file or
    *                               baseURL is null for relative file references.
    */   
   public static URL fileRefToURL(URL baseURL, String fileRef)
     throws URISyntaxException, MalformedURLException
   {
     URL resultURL = null;
 
     URI fileRefURI = new URI(fileRef);
 
     if (fileRefURI.isAbsolute())
     {
       resultURL = fileRefURI.toURL();
     }
     else
     {
       resultURL = new URL(baseURL, fileRef);
     }
 
     return resultURL;
   }
 }

