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
import java.nio.file.FileSystem;
import java.nio.file.FileSystems;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.Calendar;
import java.util.Date;
import java.util.HashMap;
import java.util.Properties;
import java.util.Vector;
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

public class Dicom2Movie implements Runnable {
	boolean debug = false;
	Vector<String> uri;
	Vector<String> names;
	String outputDir;
	boolean success = false;
	boolean outputJpegs = false;

	String dcm2movieExec;
	String dcm2movieDir;

	public Dicom2Movie() throws Exception {
		// Use JNDI to get the deployment values as this class is a POJO
		Context context = new InitialContext();
		dcm2movieExec = (String) context.lookup("java:global/ICMADCM2MOVIE");
		dcm2movieDir = (String) context.lookup("java:global/ICMADCM2MOVIEDIR");
		uri = new Vector<String>();
		names = new Vector<String>();
		if (debug) {
			System.out.println("*********************\nWarning! Dicom2Movie in debug mode\n*********************");
		}
	}

	public Dicom2Movie(String dcm2movieExec, String dcm2movieDir) {
		uri = new Vector<String>();
		names = new Vector<String>();
		this.dcm2movieExec = dcm2movieExec;
		this.dcm2movieDir = dcm2movieDir;
	}


	public boolean wasSuccessful() {
		return success;
	}

	public void run() {
		try {
			success = createMovie();
		} catch (Exception exx) {
			Logger log = Logger.getLogger(this.getClass().getSimpleName());
			log.log(Level.SEVERE, "Dicom To Movie maker failed with exception " + exx);
			success = false;
		}
	}

	public void addUri(String name, String url) {
		uri.add(url);
		names.add(name);
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
		String exec = dcm2movieExec;
		if (!debug) {
			Logger log = Logger.getLogger(this.getClass().getSimpleName());
			// Create the XML file
			DocumentBuilderFactory docFactory = DocumentBuilderFactory.newInstance();
			DocumentBuilder docBuilder = docFactory.newDocumentBuilder();
			// Create temporary file
			int num = (int) (Math.random() * 1000);
			String xmlFilename = outputDir + "/dcm2movie" + num + ".xml";
			File xmlFile = null;
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

				int numdcms = names.size();
				for (int i = 0; i < numdcms; i++) {
					Element file = doc.createElement("FILE");
					Element name = doc.createElement("NAME");
					Element url = doc.createElement("URI");
					name.appendChild(doc.createTextNode(names.elementAt(i)));
					url.appendChild(doc.createTextNode(uri.elementAt(i)));
					file.appendChild(name);
					file.appendChild(url);
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
				// StringWriter sr = new StringWriter();
				// StreamResult sresult = new StreamResult(sr);

				transformer.transform(source, result);
				// transformer.transform(source, sresult);

				// Output to console for testing
				/*
				 * StreamResult console = new StreamResult(System.out);
				 * transformer.transform(source, console);
				 * System.out.println(console);
				 */
			}
			boolean check = false;
			int counter = 0;
			Date start = Calendar.getInstance().getTime();
			String error = null;
			while (!check && counter < 3) {
				check = true;

				Date startx = Calendar.getInstance().getTime();
				log.log(Level.INFO, "Dicom to movie maker invoked at (" + (counter++) + ") " + startx);
				DICOMTOMOVIEInterface inter = new DICOMTOMOVIEInterface(xmlFilename, exec, dcm2movieDir);
				error = inter.checkCompletion();
				HashMap<String, String> failedInstances = new HashMap<String, String>();

				// Check if the required files are there
				for (String myName : names) {
					String properties = outputDir + "/" + myName + ".properties";
					File propFile = new File(properties);
					if (!propFile.exists()) {
						check = false;
					} else {
						Properties prop = new Properties();
						prop.load(new FileInputStream(properties));
						if (prop.getProperty("ICMA_FEM") == null) {// DICOM
																	// Instance
							if (prop.getProperty("DICOMTYPE") == null) {// DICOMTYPE=SINGLE
																		// for
																		// non
																		// multiframe
																		// files
																		// and
																		// these
																		// will
																		// not
																		// be
																		// loaded
								FileSystem defaultFS = FileSystems.getDefault();

								if (!Files.exists(defaultFS.getPath(outputDir + "/" + myName + ".webm"))) {
									check = false;
									log.log(Level.INFO, defaultFS.getPath(outputDir + "/" + myName + ".webm").toAbsolutePath() + " does not exist");
								}
								if (!Files.exists(defaultFS.getPath(outputDir + "/" + myName + ".mp4"))) {
									check = false;
									log.log(Level.INFO, defaultFS.getPath(outputDir + "/" + myName + ".mp4").toAbsolutePath() + " does not exist");
								}
								if (!Files.exists(defaultFS.getPath(outputDir + "/" + myName + ".ogv"))) {
									check = false;
									log.log(Level.INFO, defaultFS.getPath(outputDir + "/" + myName + ".ogv").toAbsolutePath() + " does not exist");
								}
								if (!Files.exists(defaultFS.getPath(outputDir + "/" + myName + "POSTER.jpg"))) {
									check = false;
									log.log(Level.INFO, defaultFS.getPath(outputDir + "/" + myName + "POSTER.jpg").toAbsolutePath() + " does not exist");
								}
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

					int numdcms = names.size();
					for (int i = 0; i < numdcms; i++) {
						String instanceName = names.elementAt(i);
						if (failedInstances.containsKey(instanceName)) {
							Element file = doc.createElement("FILE");
							Element name = doc.createElement("NAME");
							Element url = doc.createElement("URI");
							name.appendChild(doc.createTextNode(instanceName));
							url.appendChild(doc.createTextNode(uri.elementAt(i)));
							file.appendChild(name);
							file.appendChild(url);
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
				log.log(Level.WARNING, "Dicom2Movie failed for " + outputDir + "\t" + error);
				// Clean up the working directory as failure leaves orphaned
				// files
				cleanUp();
			}
			if (!check) {
				// System.out.println(error);
				log.log(Level.WARNING, "Dicom2Movie failed for " + outputDir);
				// Clean up the working directory as failure leaves orphaned
				// files
				cleanUp();
			} else {
				// Delete the xml File
				xmlFile.delete();
			}

			Date end = Calendar.getInstance().getTime();
			if (check) {
				log.log(Level.INFO, "Dicom to movie maker completed successfully at " + end + " walk clock time " + (end.getTime() - start.getTime()) / (1000) + " secs");
			} else {
				log.log(Level.INFO, "Dicom to movie maker failed!! at " + end + " using " + (end.getTime() - start.getTime()) / (1000) + " secs of walk clock time");
			}

			return check;
		} else {
			// Copy dir contents to output dir
			String debugdir = "/home/data/SanityTest";
			File dir = new File(debugdir);
			File[] list = dir.listFiles();
			for (int i = 0; i < list.length; i++) {
				File tfile = new File(outputDir, list[i].getName());
				Files.copy(Paths.get(list[i].getAbsolutePath()), Paths.get(tfile.getAbsolutePath()), java.nio.file.StandardCopyOption.REPLACE_EXISTING);
			}

			return true;
		}
	}

	public void cleanUp() {
		// File wDir = new File(dcm2movieDir);
		File wDir = new File(outputDir);
		File exec = new File(dcm2movieExec);
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
}
