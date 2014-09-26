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
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.logging.Level;
import java.util.logging.Logger;

public class SpeckleFitting implements Runnable{
	private ProcessBuilder mypb;
	private Process speckleProcess;
	private String speckleInputFile;
	private String speckleOutputDir;
	private String speckleWorkingDir;
	private String speckleExec;
	private String errorLogDirectory;

	private boolean completed = false;
	private boolean failed = false;
	private String output = null;
	private double workload = 0.0;
	private double max = 1.0;

	/* Initialize the class with details for fitting a CMISS model
	 * @param xmlFile Name of the XML file passed to speckle fittinh client for fitting. The name should be fully qualified and the directory containing the xmlfile should also contain the images
	 * @param outputDir Name of the directory where the model files will be stored 
	 * @param speckleExecutable fully qualified path to the Client executable that will be invoked
	 * @param speckleWorkDir fully qualified path of the Client's working directory
	 */
	public SpeckleFitting(String xmlFile, String outputDir, String capExecutable, String capWorkDir) throws Exception
	{
		speckleInputFile = xmlFile;
		speckleOutputDir = outputDir;
		speckleExec = capExecutable;
		speckleWorkingDir = capWorkDir;
		errorLogDirectory = ".";
		//Client expects fully qualified xmlfilename, outputDir
		mypb = new ProcessBuilder(speckleExec,speckleInputFile,speckleOutputDir);
		mypb.directory(new File(speckleWorkingDir));

		speckleProcess = mypb.redirectErrorStream(true).start();

	}

	public void setErrorLogDirectory(String errordir){
		errorLogDirectory = errordir;
	}
	
	/* Wait for the process initiated to be completed and return the output stream of the process as a String
	 * @return the output stream of the process
	 * 
	 */

	public void run(){
		StringBuffer buf = new StringBuffer("");
		BufferedReader in = null;
		try{
			try
			{
				in = new BufferedReader(new InputStreamReader(speckleProcess.getInputStream()));
				while (true)
				{
					String line = in.readLine();
					if (line == null)
						break;
					buf.append(line+"\n");
					//Do processing
					if(line.indexOf("WORKLOAD:")>=0){
						workload += Double.parseDouble(line.substring(line.indexOf(':')+1));
						max = workload;
					}else if(line.indexOf(":completed")>0){
						workload = 0.0;
					}else if(line.startsWith("STEP")){
						workload -= 1.0;
					}
				}
			}
			finally
			{
				in.close();
			}
			int retval = speckleProcess.waitFor();

			if(retval!=0){ 
				failed = true;
			}else{
				//Append output to a log file 
				try {
					File edir = new File(errorLogDirectory);
					if(!edir.exists())
						edir.mkdirs();
					File efile = new File(edir,"FittingResultsLog0.txt");
					int ctr = 1;
					while(efile.exists()){
						efile = new File(edir,"FittingResultsLog"+ctr+".txt");
						ctr++;
					}
				    PrintWriter out = new PrintWriter(new BufferedWriter(new FileWriter(efile, true)));
				    out.println(buf.toString());
				    out.close();
				} catch (IOException e) {
				    Logger log = Logger.getLogger(this.getClass().getSimpleName());
				    log.log(Level.INFO,"Error occured while writing to fitting log file\n"+e.getMessage());
				}
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
	
	public String getOutput(){
		return output;
	}
	
	public boolean hasCompleted(){
		return completed;
	}
	
	public boolean hasFailed(){
		return failed;
	}
}
