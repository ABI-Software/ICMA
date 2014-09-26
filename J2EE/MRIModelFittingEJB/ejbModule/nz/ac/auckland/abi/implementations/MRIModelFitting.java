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
import java.util.logging.Level;
import java.util.logging.Logger;

public class MRIModelFitting implements Runnable {
	private ProcessBuilder fittingProcessBuilder;
	private ProcessBuilder measureProcessBuilder;
	private Process fittingProcess;
	private String xmlInputFile;
	private String fittingOutputDir;
	private String fittingWorkingDir;
	private String fittingExec;
	private Process measuringProcess;
	private String measuringWorkingDir;
	private String measuringExec;
	private String errorLogDirectory;

	private boolean completed = false;
	private boolean failed = false;
	private String output = null;
	private double workload = 0.0;
	private double max = 1.0;
	Logger log;

	/*
	 * Initialize the class with details for fitting a CMISS model
	 * 
	 * @param xmlFile Name of the XML file passed to model fitting client for
	 * fitting. The name should be fully qualified and the directory containing
	 * the xmlfile should also contain the images
	 * 
	 * @param outputDir Name of the directory where the model files will be
	 * stored
	 * 
	 * @param fittingExecutable fully qualified path to the Client executable
	 * that will be invoked
	 * 
	 * @param fitWorkDir fully qualified path of the Client's working directory
	 */
	public MRIModelFitting(String xmlFile, String outputDir, String fitExecutable, String fitWorkDir, String measureExecutable, String measureWorkDir) throws Exception {
		xmlInputFile = xmlFile;
		fittingOutputDir = outputDir;
		fittingExec = fitExecutable;
		fittingWorkingDir = fitWorkDir;
		errorLogDirectory = ".";
		// Client expects fully qualified xmlfilename, outputDir
		fittingProcessBuilder = new ProcessBuilder(fittingExec, xmlInputFile, fittingOutputDir);
		fittingProcessBuilder.directory(new File(fittingWorkingDir));

		fittingProcess = fittingProcessBuilder.redirectErrorStream(true).start();

		measuringExec = measureExecutable;
		measuringWorkingDir = measureWorkDir;
		log = Logger.getLogger(this.getClass().getSimpleName());
		

	}

	public void setErrorLogDirectory(String errordir) {
		errorLogDirectory = errordir;
	}

	/*
	 * Wait for the process initiated to be completed and return the output
	 * stream of the process as a String
	 * 
	 * @return the output stream of the process
	 */

	public void run() {
		try {
			workload = 2;
			max = 2;
			log.log(Level.FINE,"MRI FIT started with xmlinput "+xmlInputFile);
			{

				BufferedReader in = null;
				try
				{
					in = new BufferedReader(new InputStreamReader(fittingProcess.getInputStream()));
					while (true)
					{
						String line = in.readLine();
						if (line == null)
							break;
						log.log(Level.INFO,line);
					}
				}
				finally
				{
					in.close();
				}

			}
			int retval = fittingProcess.waitFor();
			log.log(Level.FINE,"MRI FIT completed with "+retval);
			if (retval != 0) {
				failed = true;
			} else {
				workload = 1.0;
				output = retval + "\t";
				// Create measuring process
				File xmlFile = new File(fittingOutputDir, "MRIFIT.xml");
				File dcmFile = new File(fittingOutputDir, "MRIFIT.dcm");
				if (xmlFile.exists() && dcmFile.exists()) {
					log.log(Level.FINE,"MRI measure being created woth "+xmlFile.getAbsolutePath()+"\n"+dcmFile.getAbsolutePath());
					measureProcessBuilder = new ProcessBuilder(measuringExec, xmlFile.getAbsolutePath(), dcmFile.getAbsolutePath(), fittingOutputDir);
					measureProcessBuilder.directory(new File(measuringWorkingDir));

					measuringProcess = measureProcessBuilder.redirectErrorStream(true).start();
					log.log(Level.FINE,"MRI measure started");
					StringBuffer buf = new StringBuffer();
					BufferedReader in = null;
					try
					{
						in = new BufferedReader(new InputStreamReader(measuringProcess.getInputStream()));
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
					
					retval = measuringProcess.waitFor();
					log.log(Level.FINE,"MRI measure completed with "+retval+"\t"+buf.toString());
					if (retval != 0) {
						failed = true;
					}
					workload = 0.0;
				}else{
					failed = true;
				}
			}
			output = output + retval;
			completed = true;
			log.log(Level.FINE,"MRI exited with "+output+"\t failed = "+failed);
		} catch (Exception exx) {
			exx.printStackTrace();
			StringWriter sw = new StringWriter();
			exx.printStackTrace(new PrintWriter(sw));
			output = sw.toString();
			completed = true;
			failed = true;
			log.log(Level.INFO,"MRI exited with "+exx);
		}
	}

	public double getProgress() {
		double result = 1.0 - workload / max;
		if(result<0.1)
			result = 0.25;
		if (!completed && !failed) { // Ensure that 1.0 is returned only on
										// completion
			return (result == 1.0) ? 0.0 : result;
		} else {
			return 1.0;
		}
	}

	public String getOutput() {
		return output;
	}

	public boolean hasCompleted() {
		return completed;
	}

	public boolean hasFailed() {
		return failed;
	}
}
