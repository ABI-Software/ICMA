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
package nz.ac.auckland.abi.dicomprocessing;

import java.io.BufferedReader;
import java.io.File;
import java.io.InputStreamReader;

import javax.naming.Context;
import javax.naming.InitialContext;

public class DICOMTOMOVIEInterface {
	private ProcessBuilder mypb;
	private Process mProcess;
	private String inputFile;
	private String workingDir;
	private String exec;
	private String ffmpeg = "ffmpeg";


	/* Initialize the class with details for fitting a CAP model
	 * @param xmlFile Name of the XML file passed to DICOMTOMOVIE client 
	 * @param executable fully qualified path to the DICOMTOMOVIE Client executable that will be invoked
	 * @param workDir fully qualified path of the DICOMTOMOVIE Client's working directory
	 */
	public DICOMTOMOVIEInterface(String xmlFile, String executable, String workDir) throws Exception
	{
		inputFile = xmlFile;
		exec = executable;
		workingDir = workDir;
		
		Context context = new InitialContext();
		try{
			ffmpeg = (String) context.lookup("java:global/FFMEPGEXEC");
		}catch(Exception exx){
			ffmpeg = "ffmpeg";		
		}

		//Client expects fully qualified xmlInputfilename 
		mypb = new ProcessBuilder(exec,inputFile,ffmpeg,workDir);
		mypb.directory(new File(workingDir));

		mProcess = mypb.redirectErrorStream(true).start();

	}

	/* Wait for the process initiated to be completed and return the output stream of the process as a String
	 * @return the output stream of the process
	 * 
	 */

	public String checkCompletion() throws Exception{
		StringBuffer buf = new StringBuffer("");
		
		BufferedReader in = null;
		try
		{
			in = new BufferedReader(new InputStreamReader(mProcess.getInputStream()));
			while (true)
			{
				String line = in.readLine();
				if (line == null)
					break;
				buf.append(line);
			}
		}
		finally
		{
			in.close();
		}
		int retval = mProcess.waitFor();
		
		return retval+"\n"+buf.toString();
	}

	// Deletes all files and subdirectories under dir.
	// Returns true if all deletions were successful.
	// If a deletion fails, the method stops attempting to delete and returns false.
	public static boolean deleteDir(File dir) {
		if (dir.isDirectory()) {
			String[] children = dir.list();
			for (int i=0; i<children.length; i++) {
				boolean success = deleteDir(new File(dir, children[i]));
				if (!success) {
					return false;
				}
			}
		}

		// The directory is now empty so delete it
		return dir.delete();
	}

	public static void main(String args[]) throws Exception
	{
		String xmlFile = "/home/rjag008/workspace/DICOMtoMOVIE/src/build/test.xml";
		String Exec = "/home/rjag008/workspace/DICOMtoMOVIE/src/build/movie";
		String Dir =  "/home/rjag008/workspace/DICOMtoMOVIE/src/build";
		DICOMTOMOVIEInterface inter = new DICOMTOMOVIEInterface(xmlFile, Exec, Dir);
		System.out.println(inter.checkCompletion());
		//inter.checkCompletion();
	}
}
