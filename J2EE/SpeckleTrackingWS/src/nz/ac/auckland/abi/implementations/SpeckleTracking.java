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
package nz.ac.auckland.abi.implementations;

import java.io.BufferedReader;
import java.io.File;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.io.StringWriter;

public class SpeckleTracking implements Runnable{
	private ProcessBuilder mypb;
	private Process sfProcess;
	private String sfInputFile;
	private String sfWorkingDir;
	private String sfExec;

	private boolean completed = false;
	private boolean failed = false;
	private String output = null;
	private double workload = 0.0;
	private double max = 1.0;
	private int viewCounter = 0; 

	/* Initialize the class with details for fitting a CAP model
	 * @param xmlFile Name of the XML file passed to SF client for fitting. The name should be fully qualified and the directory containing the xmlfile should also contain the images
	 * @param sfExecutable fully qualified path to the SF Client executable that will be invoked
	 * @param sfWorkDir fully qualified path of the SF Client's working directory
	 */
	public SpeckleTracking(String xmlFile, String executable, String workDir) throws Exception
	{
		sfInputFile = xmlFile;
		sfExec = executable;
		sfWorkingDir = workDir;

		//Client expects fully qualified xmlInputfilename 
		mypb = new ProcessBuilder(sfExec,sfInputFile);
		mypb.directory(new File(sfWorkingDir));
		sfProcess = mypb.redirectErrorStream(true).start();

	}

	/* Wait for the process initiated to be completed and return the output stream of the process as a String
	 * @return the output stream of the process
	 * 
	 */

	public void run(){
		StringBuffer buf = new StringBuffer("");
		BufferedReader in = null;
		workload  = 0.0;
		try{
			try
			{
				in = new BufferedReader(new InputStreamReader(sfProcess.getInputStream()));
				while (true)
				{
					String line = in.readLine();
					if (line == null)
						break;
					buf.append(line);
					//Do processing
					if(line.indexOf("WORKLOAD:")>=0){
						workload += Double.parseDouble(line.substring(line.indexOf(':')+1));
						max = workload;
						viewCounter++;
					}else if(line.indexOf(":completed")>0){
						viewCounter--;
					}else if(line.startsWith("MET")||line.startsWith("RMET")){
						workload -= 1.0;
					}
				}
			}
			finally
			{
				in.close();
			}
			int retval = sfProcess.waitFor();

			if(retval!=0&&retval!=139){ //Note that the exec throws segmentation fault post creation and could be ignored, all others indicate failure
				failed = true;
			}
			output = retval+"\n"+buf.toString();
			completed = true;
		}catch(Exception exx){
			exx.printStackTrace();
			StringWriter sw = new StringWriter();
			exx.printStackTrace(new PrintWriter(sw));
			output = sw.toString();
			completed = true;
			failed = true;
		}
	}

	public double getProgress(){
		double result = 1.0 - workload/max;
		if(!completed){ //Ensure that 1.0 is returned only on completion
			return (result==1.0)? 0.0 : result;
		}else{
			return 1.0;
		}
	}
	
	public int getActiveViews(){
		return viewCounter;
	}
	
	public String getOutput(){
		return output;
	}
	
	public boolean hasCompleted(){
		return completed;
	}
	
	public boolean hasFailed(){
		return failed;
	}
	
	public static void main(String args[]) throws Exception
	{
		String xmlFile = "/home/rjag008/clevelandclinic/SpeckleTracking/build/Testing/1View_ST.xml";
		String capExec = "/home/rjag008/clevelandclinic/SpeckleTracking/build/Speckle";
		String capDir =  "/home/rjag008/clevelandclinic/SpeckleTracking/build/Testing";
		SpeckleTracking inter = new SpeckleTracking(xmlFile, capExec, capDir);
		inter.run();
		System.out.println(inter.getOutput());
	}
}
