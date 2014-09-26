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

import java.io.File;
import java.io.FileInputStream;
import java.io.PrintWriter;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.HashMap;
import java.util.Properties;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.naming.Context;
import javax.naming.InitialContext;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;

import org.w3c.dom.Document;
import org.w3c.dom.Element;

public class DicomSeries2Movie implements Runnable {
	private boolean success;
	private boolean debug;
	private String outputDir;
	private boolean outputJpegs = false;
	private HashMap<String, ArrayList<String>> uri;
	private Logger log;

	String dcmseries2movieExec;
	String dcmseries2movieDir;

	public DicomSeries2Movie() throws Exception {
		// Use JNDI to get the deployment values as this class is a POJO
		Context context = new InitialContext();
		dcmseries2movieExec = (String) context.lookup("java:global/ICMADCMSERIES2MOVIE");
		dcmseries2movieDir = (String) context.lookup("java:global/ICMADCMSERIES2MOVIEDIR");
		uri = new HashMap<String, ArrayList<String>>();
		log = Logger.getLogger(this.getClass().getSimpleName());
		if (debug) {
			log.log(Level.WARNING,"*********************\nWarning! DicomSeries2Movie in debug mode\n*********************");
		}
	}

	public DicomSeries2Movie(String dcm2movieExec, String dcm2movieDir) {
		uri = new HashMap<String, ArrayList<String>>();
		this.dcmseries2movieExec = dcm2movieExec;
		this.dcmseries2movieDir = dcm2movieDir;
		log = Logger.getLogger(this.getClass().getSimpleName());
	}

	public boolean wasSuccessful() {
		return success;
	}

	public void addSeries(String name, ArrayList<String> files) {
		uri.put(name, files);
	}

	public void setOutputDirectory(String dir) {
		outputDir = dir;
	}

	public String getOutputDirectory() {
		return outputDir;
	}

	public void outputJpeg() {
		outputJpegs = true;
	}

	public boolean createMovie() throws Exception {
		String exec = dcmseries2movieExec;
		if (!debug) {
			// Create the XML file
			DocumentBuilderFactory docFactory = DocumentBuilderFactory.newInstance();
			DocumentBuilder docBuilder = docFactory.newDocumentBuilder();
			// Create temporary file
			int num = (int) (Math.random() * 1000);
			String xmlFilename = outputDir + "/dcmseries2movie" + num + ".xml";
			File xmlFile = null;
			ArrayList<String> keys = new ArrayList<String>(uri.keySet());
			// root element
			{
				Document doc = docBuilder.newDocument();
				Element rootElement = doc.createElement("ICMA");
				doc.appendChild(rootElement);

				if (outputDir != null) {
					Element directory = doc.createElement("OUTPUTDIRECTORY");
					if (outputJpegs) {
						directory.setAttribute("JPEGS", "true");
					}
					directory.appendChild(doc.createTextNode(outputDir));
					rootElement.appendChild(directory);
				}

				for (int i = 0; i < keys.size(); i++) {
					Element file = doc.createElement("FILE");
					Element name = doc.createElement("NAME");
					String key = keys.get(i);
					name.appendChild(doc.createTextNode(key));
					file.appendChild(name);
					ArrayList<String> myfiles = uri.get(key);
					for(String f: myfiles){
						Element url = doc.createElement("URI");
						url.appendChild(doc.createTextNode(f));
						file.appendChild(url);
					}
					rootElement.appendChild(file);
				}

				xmlFile = new File(xmlFilename);
				xmlFile.deleteOnExit();
				// write the content into xml file
				TransformerFactory transformerFactory = TransformerFactory.newInstance();

				Transformer transformer = transformerFactory.newTransformer();
				DOMSource source = new DOMSource(doc);
				PrintWriter pr = new PrintWriter(xmlFile);
				StreamResult result = new StreamResult(pr);
				transformer.transform(source, result);

				// Output to console for testing
/* 				StringWriter sr = new StringWriter();
 				StreamResult sresult = new StreamResult(sr);
				transformer.transform(source, sresult);
				StreamResult console = new StreamResult(System.out);
				transformer.transform(source, console);
				log.log(Level.INFO,sr.toString());*/
				//System.out.println(console);

			}
			boolean check = false;
			int counter = 0;
			Date start = Calendar.getInstance().getTime();
			String error = null;
			while (!check && counter < 3) {
				check = true;

				Date startx = Calendar.getInstance().getTime();
				log.log(Level.INFO, "DicomSeries to movie maker invoked at (" + (counter++) + ") " + startx);
				DICOMTOMOVIEInterface inter = new DICOMTOMOVIEInterface(xmlFilename, exec, dcmseries2movieDir);
				error = inter.checkCompletion();
				HashMap<String, String> failedInstances = new HashMap<String, String>();

				// Check if the required files are there

				for (String myName : keys) {
					String properties = outputDir + "/" + myName + ".properties";
					File propFile = new File(properties);
					if (!propFile.exists()) {
						check = false;
					} else {
						Properties prop = new Properties();
						prop.load(new FileInputStream(properties));
						int numplanes = Integer.parseInt(prop.getProperty("NUMPLANES"));
						// Check for necessary files
						for(int p=1;p<numplanes;p++){
							String pname = "PLANE"+p;
							String prefix = prop.getProperty(pname);
							if(prefix!=null){
								File props = new File(outputDir+"/"+prefix+".properties");
								File webm = new File(outputDir+"/"+prefix+".webm");
								File mp4 = new File(outputDir+"/"+prefix+".mp4");
								File ogv = new File(outputDir+"/"+prefix+".ogv");
								if(!(props.exists() && webm.exists() && mp4.exists() && ogv.exists())){
									log.log(Level.WARNING,"Failed for "+prefix);
									check = false;
									break;
								}
							}else{
								log.log(Level.WARNING,"Failed for properties of "+pname);
								check = false;
								break;
							}
						}
					}
					if (!check) {
						failedInstances.put(myName, myName);
					}
				}

				// Remove the successful instances and call dicomtomovie again
				if (counter < 3 && failedInstances.size() > 0) {
					Document doc = docBuilder.newDocument();
					Element rootElement = doc.createElement("ICMA");
					doc.appendChild(rootElement);

					if (outputDir != null) {
						Element directory = doc.createElement("OUTPUTDIRECTORY");
						if (outputJpegs) {
							directory.setAttribute("JPEGS", "true");
						}
						directory.appendChild(doc.createTextNode(outputDir));
						rootElement.appendChild(directory);
					}

					int numdcms = keys.size();
					for (int i = 0; i < numdcms; i++) {
						String instanceName = keys.get(i);
						if (failedInstances.containsKey(instanceName)) {
							Element file = doc.createElement("FILE");
							Element name = doc.createElement("NAME");
							file.appendChild(name);
							ArrayList<String> myfiles = uri.get(instanceName);
							for(String f: myfiles){
								Element url = doc.createElement("URI");
								url.appendChild(doc.createTextNode(f));
								file.appendChild(url);
							}
							rootElement.appendChild(file);
						}
					}

					// Create temporary file
					xmlFilename = outputDir + "/dcm2movie" + (num + 1) + ".xml";
					xmlFile = new File(xmlFilename);
					xmlFile.deleteOnExit();
					// write the content into xml file
					TransformerFactory transformerFactory = TransformerFactory.newInstance();

					Transformer transformer = transformerFactory.newTransformer();
					DOMSource source = new DOMSource(doc);
					PrintWriter pr = new PrintWriter(xmlFile);
					StreamResult result = new StreamResult(pr);
					transformer.transform(source, result);
					log.log(Level.INFO, "Retrying with " + failedInstances.size() + " instances");
				}

			}
			if (!error.trim().startsWith("0")) {
				log.log(Level.WARNING, "DicomSeries2Movie failed for " + outputDir + "\t" + error);
				// Clean up the working directory as failure leaves orphaned
				// files
				cleanUp();
			}
			if (!check) {
				// System.out.println(error);
				log.log(Level.WARNING, "DicomSeries2Movie failed for " + outputDir);
				// Clean up the working directory as failure leaves orphaned
				// files
				cleanUp();
			} else {
				// Delete the xml File
				xmlFile.delete();
			}

			Date end = Calendar.getInstance().getTime();
			if (check) {
				log.log(Level.INFO, "Dicomseries to movie maker completed successfully at " + end + " walk clock time " + (end.getTime() - start.getTime()) / (1000) + " secs");
			} else {
				log.log(Level.INFO, "Dicomseries to movie maker failed!! at " + end + " using " + (end.getTime() - start.getTime()) / (1000) + " secs of walk clock time");
			}

			return check;
		} else {
			// Copy dir contents to output dir
			String debugdir = "/home/rjag008/SanityTest";
			File dir = new File(debugdir);
			File[] list = dir.listFiles();
			for (int i = 0; i < list.length; i++) {
				File tfile = new File(outputDir, list[i].getName());
				Files.copy(Paths.get(list[i].getAbsolutePath()), Paths.get(tfile.getAbsolutePath()), java.nio.file.StandardCopyOption.REPLACE_EXISTING);
			}

			return true;
		}
	}

	@Override
	public void run() {
		try {
			success = createMovie();
		} catch (Exception exx) {
			Logger log = Logger.getLogger(this.getClass().getSimpleName());
			log.log(Level.SEVERE, "DicomSeries To Movie maker failed with exception " + exx);
			success = false;
		}
	}

	public void cleanUp() {
		File wDir = new File(outputDir);
		File exec = new File(dcmseries2movieDir);
		String execFile = exec.getName();
		File[] list = wDir.listFiles();
		Logger log = Logger.getLogger(this.getClass().getSimpleName());
		log.log(Level.INFO, "Cleaning up " + wDir.getAbsolutePath());
		for (File oFile : list) {
			if (!oFile.getName().equalsIgnoreCase(execFile)) {
				if (oFile.isDirectory())
					deleteDir(oFile);
				else
					oFile.delete();
			}
		}
		if (outputDir != null)
			deleteDir(new File(outputDir));
	}

	private boolean deleteDir(File dir) {
		if (dir.isDirectory()) {
			String[] children = dir.list();
			for (int i = 0; i < children.length; i++) {
				boolean success = deleteDir(new File(dir, children[i]));
				if (!success) {
					return false;
				}
			}
		}
		// The directory is now empty so delete it
		return dir.delete();
	}

	public static void main(String args[]) throws Exception {
		DicomSeries2Movie mo = new DicomSeries2Movie("/home/rjag008/ClevelandClinic/CPP/DICOMSERIESTOMOVIE/Release/movie",
				"/home/rjag008/ClevelandClinic/CPP/DICOMSERIESTOMOVIE/Release/test");
		ArrayList<String> files = new ArrayList<String>();
		files.add("/home/rjag008/ClevelandClinic/CPP/DICOMSERIESTOMOVIE/Debug/SeriesDataAll");
		mo.addSeries("ICMA", files);
		mo.outputJpeg();
		mo.setOutputDirectory("/home/rjag008/ClevelandClinic/CPP/DICOMSERIESTOMOVIE/Release/output");
		mo.createMovie();
	}

}
