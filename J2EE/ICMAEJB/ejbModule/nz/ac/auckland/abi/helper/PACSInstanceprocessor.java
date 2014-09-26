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
package nz.ac.auckland.abi.helper;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.Properties;
import java.util.Vector;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.zip.GZIPOutputStream;

import nz.ac.auckland.abi.dcm4chee.DCMAccessManager;
import nz.ac.auckland.abi.dcm4chee.InstanceRecord;
import nz.ac.auckland.abi.dicomprocessing.Dicom2Movie;
import nz.ac.auckland.abi.entities.FEMModel;
import nz.ac.auckland.abi.entities.USStudyInstances;
import nz.ac.auckland.abi.icmaconfiguration.ProcessManager;
import nz.ac.auckland.abi.icmaconfiguration.ResourceConfigurationManager;

public class PACSInstanceprocessor {
	private final boolean debug = false;
	Logger log = null;
	ResourceConfigurationManager rManager;
	ProcessManager processManger;
	private String author = "ICMA";
	private final String instancePrefix = "Instance";
	private static int instanceCounter = 0;
	private Vector<USStudyInstances> entityInstances;
	private Vector<FEMModel> entityModels;
	Dicom2Movie processor;
	private String myWorkingDir;
	private Vector<String> createdNodes;

	public PACSInstanceprocessor(String author, ResourceConfigurationManager rM, String patientID, String studyID, Vector<InstanceRecord> instances) throws Exception {
		rManager = rM;
		this.author = author;
		processManger = rManager.getProcessManager();
		log = Logger.getLogger(PACSInstanceprocessor.class.getSimpleName());
		processor = new Dicom2Movie();
		boolean success = false;
		int ctr = 0;
		createdNodes = new Vector<String>();
		// rM.startTx();

		if (!debug) {
			myWorkingDir = rManager.getDiskScratchSpace() + "/pip" + instanceCounter++;
			// Create the directory
			File workDir = new File(myWorkingDir);
			while (workDir.exists()) {
				myWorkingDir = rManager.getDiskScratchSpace() + "/pip" + instanceCounter++;
				workDir = new File(myWorkingDir);
			}
			workDir.mkdirs();
			processor.setOutputDirectory(myWorkingDir);

			for (ctr = 0; ctr < instances.size(); ctr++) {
				InstanceRecord rec = instances.elementAt(ctr);

				processor.addUri(instancePrefix + ctr, DCMAccessManager.getInstanceWADOUrl(studyID, rec.getSeriesID(), rec.getSopIuid()));
				// Rather than passing the wado uri to the target instance,
				// retrive the file and give it
				// 17 July 2013, unable to find a dcmqr request that would
				// download the instance, using URl instead
				/*
				 * File filename = new File(workDir,studyID+ctr+".dcm"); URL
				 * dicomUrl = new
				 * URL(DCMAccessManager.getInstanceWADOUrl(studyID,
				 * rec.getSeriesID(), rec.getSopIuid())); ReadableByteChannel
				 * rbc = Channels.newChannel(dicomUrl.openStream());
				 * FileOutputStream fos = new FileOutputStream(filename);
				 * fos.getChannel().transferFrom(rbc, 0, Long.MAX_VALUE);
				 * fos.close();
				 * processor.addUri(instancePrefix+ctr,filename.getAbsolutePath
				 * ());
				 */
			}
			// success = processor.createMovie();
			processManger.processInQueue(processor);
			success = processor.wasSuccessful();
		} else {
			//myWorkingDir = rManager.getDiskScratchSpace() + "/debug";
			myWorkingDir = "/home/data/SanityTest";
			success = true;
		}
		entityInstances = new Vector<USStudyInstances>();
		entityModels = new Vector<FEMModel>();

		if (success) {
			// Load prepared files and upload them to the context repository
			// Create a PACSStudyInstance instance
			// If they are CMISS instances
			// Process appropriately and prepare FEMModel instances
			ctr = 0;
			for (ctr = 0; ctr < instances.size(); ctr++) {
				InstanceRecord rec = instances.elementAt(ctr);
				String sopInstanceUID = rec.getSopIuid()+Math.random(); //To avoid same view uid clashes
				String myName = instancePrefix + ctr;
				String workingDirectory = myWorkingDir;
				String properties = myWorkingDir + "/" + myName + ".properties";
				File propertiesFile = new File(properties);
				if (!propertiesFile.exists()) {
					log.log(Level.INFO, "File " + properties + " does not exist!!");

				} else {
					Properties prop = new Properties();
					try {
						// load a properties file
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
								String webm = workingDirectory + "/" + myName + ".webm";
								String mp4 = workingDirectory + "/" + myName + ".mp4";
								String oggv = workingDirectory + "/" + myName + ".ogv";
								String poster = workingDirectory + "/" + myName + "POSTER.jpg";
								String dispFile = null;
								if (prop.getProperty("DISPLACEMENTFIELD") != null) {// Add
																					// displacement
																					// field
																					// data
									dispFile = workingDirectory + "/" + prop.getProperty("DISPLACEMENTFIELD");
									prop.remove("DISPLACEMENTFIELD");
								}

								ByteArrayOutputStream bos = new ByteArrayOutputStream();
								prop.storeToXML(bos, "DICOM");
								String propertyString = bos.toString();
								String instanceID = sopInstanceUID;
								// Load them into CMS
								String webmN = rManager.addMovieToCMS(studyID, instanceID, "us.webm", new File(webm));
								String mp4N = rManager.addMovieToCMS(studyID, instanceID, "us.mp4", new File(mp4));
								String oggvN = rManager.addMovieToCMS(studyID, instanceID, "us.oggv", new File(oggv));
								String posterN = rManager.addImageToCMS(studyID, instanceID, "POSTER.jpg", new File(poster));

								createdNodes.add(webmN);
								createdNodes.add(mp4N);
								createdNodes.add(oggvN);
								createdNodes.add(posterN);
								if (dispFile != null) {
									String dispNode = rManager.addZipToCMS(studyID, instanceID, "disp.gz", new File(dispFile));
									createdNodes.add(dispNode);
								}

								String movieMetaData = prop.getProperty("TOTALMOVIEFRAMES", "") + "#" + prop.getProperty("MOVIEFRAMERATE", "") + "#"
										+ prop.getProperty("MOVIETIME", "");

								USStudyInstances instance = new USStudyInstances(studyID, instanceID, rec.getSeriesID(), posterN, mp4N, webmN, oggvN, movieMetaData,
										propertyString);
								if (prop.contains("PatientSize")) {
									instance.setSize(Double.parseDouble(prop.getProperty("PatientSize")));
								}
								if (prop.contains("PatientWeight")) {
									instance.setWeight(Double.parseDouble(prop.getProperty("PatientWeight")));
								}

								entityInstances.add(instance);
							}
						} else {
							String viewEncoding = prop.getProperty("VIEWENCODING");
							String[] views = viewEncoding.split("#");
							String wDir = workingDirectory + "/" + myName;
							// Hashtable<String, Properties> viewUrls = new
							// Hashtable<String, Properties>();
							Properties viewUrls = new Properties();
							Properties viewUrlsTx = new Properties();
							for (int i = 0; i < views.length; i++) {
								try {
									String vRec[] = new String[2];
									int spIndx = views[i].indexOf('|');
									vRec[0] = views[i].substring(0, spIndx).trim();
									vRec[1] = views[i].substring(spIndx + 1).trim();
									int numImages = Integer.parseInt(vRec[1].trim());
									Properties urls = new Properties();
									for (int j = 0; j < numImages; j++) {
										String filename = wDir + "/" + vRec[0] + "/" + vRec[0] + String.format("%03d", j) + ".jpg";
										String cmsXpath = rManager.addImageToCMS(studyID, sopInstanceUID, vRec[0] + "_" + j + ".jpg", new File(filename));
										// System.out.println("Created "+cmsXpath.getPath());
										urls.put(vRec[0] + "_" + j, cmsXpath);
										createdNodes.add(cmsXpath);
									}
									viewUrls.put(vRec[0], urls);
									ByteArrayOutputStream bos = new ByteArrayOutputStream();
									urls.storeToXML(bos, vRec[0]);
									viewUrlsTx.put(vRec[0], bos.toString());
								} catch (Exception exx) {
									// System.out.println(views[i]);
									exx.printStackTrace();
								}
							}

							// Check if metadata is an xml or dicom file
							String fileName = prop.getProperty("METADATAFILE");
							// Parse the xml file and obtain the exregion data
							ICMAMetaData processMetaData = new ICMAMetaData(fileName);
							Vector<String> exregionFiles = processMetaData.getEXRegionFiles();
							Properties urls = new Properties();
							// Load exregion files into CMS
							for (int exctr = 0; exctr < exregionFiles.size(); exctr++) {
								String exr = exregionFiles.elementAt(exctr);
								String cmsXpath = rManager.addBytesToCMS(studyID, sopInstanceUID, "Heart." + exctr + ".exregion", "text/plain", exr.getBytes());
								urls.put("Heart." + exctr, cmsXpath);
								// System.out.println("Created "+cmsXpath.getPath());
								createdNodes.add(cmsXpath);
							}
							// Discard the author settings from the fitting
							// metadata
							processMetaData.setAuthor(this.author);
							processMetaData.setStatusAuthor(this.author);
							Properties metaProps = new Properties();
							metaProps.put("MODELJSON", processMetaData.getJSON(sopInstanceUID, viewUrls, urls));
							metaProps.put("MODELXML", processMetaData.getMetaData());

							ByteArrayOutputStream bosMP = new ByteArrayOutputStream();
							metaProps.storeToXML(bosMP, "Metadata");

							ByteArrayOutputStream bosVU = new ByteArrayOutputStream();
							viewUrlsTx.storeToXML(bosVU, "Views");

							ByteArrayOutputStream bosEX = new ByteArrayOutputStream();
							urls.storeToXML(bosEX, "FieldML");

							// Load other information
							// Compress it
							ByteArrayOutputStream zipOut = new ByteArrayOutputStream();
							GZIPOutputStream zip = new GZIPOutputStream(zipOut);
							zip.write(bosMP.toByteArray());
							zip.close();
							zipOut.close();

							FEMModel model = new FEMModel(patientID, studyID, rec.getSeriesID(), sopInstanceUID, processMetaData.getAuthor(),
									processMetaData.getModelAnnotation(), processMetaData.getModelName(), bosVU.toString(), bosEX.toString(), zipOut.toByteArray());
							log.log(Level.INFO, "Created model " + processMetaData.getModelName());
							entityModels.add(model);
						}
					} catch (IOException ex) {
						log.log(Level.INFO, "Failed to load properties " + properties);
						ex.printStackTrace();
					}
				}
			}
		} else {
			log.log(Level.INFO, "Creating movie failed for patient " + patientID + "\n working Dir" + myWorkingDir);
		}
		if (debug) {
			throw new Exception("Debugging complete");
		}
		// rM.endTx();
	}

	public PACSInstanceprocessor(String author, ResourceConfigurationManager rM, String patientID, String studyID, String seriesID, String dicomFile) throws Exception {
		rManager = rM;
		this.author = author;
		log = Logger.getLogger(PACSInstanceprocessor.class.getSimpleName());
		processor = new Dicom2Movie();
		boolean success = false;
		int ctr = 0;
		createdNodes = new Vector<String>();
		// rM.startTx();
		
		if (!debug) {
			myWorkingDir = rManager.getDiskScratchSpace() + "/pip" + instanceCounter++;
			// Create the directory
			File workDir = new File(myWorkingDir);
			while (workDir.exists()) {
				myWorkingDir = rManager.getDiskScratchSpace() + "/pip" + instanceCounter++;
				workDir = new File(myWorkingDir);
			}
			workDir.mkdirs();
			processor.setOutputDirectory(myWorkingDir);
			processor.addUri(instancePrefix + ctr, dicomFile);
			success = processor.createMovie();
		} else {
			myWorkingDir = rManager.getDiskScratchSpace() + "/debug";
			success = true;
		}
		entityModels = new Vector<FEMModel>();

		if (success) {
			// Load prepared files and upload them to the context repository
			// Create a PACSStudyInstance instance
			// If they are CMISS instances
			// Process appropriately and prepare FEMModel instances
			ctr = 0;
			String myName = instancePrefix + ctr;
			String workingDirectory = myWorkingDir;
			String properties = myWorkingDir + "/" + myName + ".properties";
			File propertiesFile = new File(properties);
			if (!propertiesFile.exists()) {
				log.log(Level.INFO, "File " + properties + " does not exist!!");
			}
			Properties prop = new Properties();
			try {
				// load a properties file
				prop.load(new FileInputStream(properties));
				if (prop.getProperty("ICMA_FEM") != null) {// DICOM Instance
					String viewEncoding = prop.getProperty("VIEWENCODING");
					String sopiuid = prop.getProperty("SOPIUID")+Math.random();
					prop.setProperty("SOPIUID", sopiuid);
					String[] views = viewEncoding.split("#");
					String wDir = workingDirectory + "/" + myName;
					// Hashtable<String, Properties> viewUrls = new
					// Hashtable<String, Properties>();
					Properties viewUrls = new Properties();
					Properties viewUrlsTx = new Properties();
					for (int i = 0; i < views.length; i++) {
						try {
							String vRec[] = new String[2];
							int spIndx = views[i].indexOf('|');
							vRec[0] = views[i].substring(0, spIndx).trim();
							vRec[1] = views[i].substring(spIndx + 1).trim();
							int numImages = Integer.parseInt(vRec[1].trim());
							Properties urls = new Properties();
							for (int j = 0; j < numImages; j++) {
								String filename = wDir + "/" + vRec[0] + "/" + vRec[0] + String.format("%03d", j) + ".jpg";
								String cmsXpath = rManager.addImageToCMS(studyID, sopiuid, vRec[0] + "_" + j + ".jpg", new File(filename));
								// System.out.println("Created "+cmsXpath.getPath());
								urls.put(vRec[0] + "_" + j, cmsXpath);
								createdNodes.add(cmsXpath);
							}
							viewUrls.put(vRec[0], urls);
							ByteArrayOutputStream bos = new ByteArrayOutputStream();
							urls.storeToXML(bos, vRec[0]);
							viewUrlsTx.put(vRec[0], bos.toString());
						} catch (Exception exx) {
							// System.out.println(views[i]);
							exx.printStackTrace();
						}
					}

					// Check if metadata is an xml or dicom file
					String fileName = prop.getProperty("METADATAFILE");
					// Parse the xml file and obtain the exregion data
					ICMAMetaData processMetaData = new ICMAMetaData(fileName);
					Vector<String> exregionFiles = processMetaData.getEXRegionFiles();
					Properties urls = new Properties();
					// Load exregion files into CMS
					for (int exctr = 0; exctr < exregionFiles.size(); exctr++) {
						String exr = exregionFiles.elementAt(exctr);
						String cmsXpath = rManager.addBytesToCMS(studyID, sopiuid, "Heart." + exctr + ".exregion", "text/plain", exr.getBytes());
						urls.put("Heart." + exctr, cmsXpath);
						// System.out.println("Created "+cmsXpath.getPath());
						createdNodes.add(cmsXpath);
					}
					// Discard the author information from fitting metadata
					processMetaData.setAuthor(author);
					processMetaData.setStatusAuthor(author);

					Properties metaProps = new Properties();
					metaProps.put("MODELJSON", processMetaData.getJSON(sopiuid, viewUrls, urls));
					metaProps.put("MODELXML", processMetaData.getMetaData());

					ByteArrayOutputStream bosMP = new ByteArrayOutputStream();
					metaProps.storeToXML(bosMP, "Metadata");

					ByteArrayOutputStream bosVU = new ByteArrayOutputStream();
					viewUrlsTx.storeToXML(bosVU, "Views");

					ByteArrayOutputStream bosEX = new ByteArrayOutputStream();
					urls.storeToXML(bosEX, "FieldML");

					// Load other information
					// Compress it
					ByteArrayOutputStream zipOut = new ByteArrayOutputStream();
					GZIPOutputStream zip = new GZIPOutputStream(zipOut);
					zip.write(bosMP.toByteArray());
					zip.close();
					zipOut.close();

					FEMModel model = new FEMModel(patientID, studyID, seriesID, sopiuid, processMetaData.getAuthor(), processMetaData.getModelAnnotation(),
							processMetaData.getModelName(), bosVU.toString(), bosEX.toString(), zipOut.toByteArray());
					log.log(Level.INFO, "Created model " + processMetaData.getModelName());
					entityModels.add(model);
				}else{
					log.log(Level.INFO, "Not a ICMA model file "+properties);
					prop.list(System.out);
				}
			} catch (IOException ex) {
				log.log(Level.INFO, "Failed to load properties " + properties);
				ex.printStackTrace();
			}
		} else {
			log.log(Level.INFO, "Creating movie failed for patient " + patientID + "\n working Dir" + myWorkingDir);
		}
		if (debug) {
			throw new Exception("Debugging complete");
		}
		// rM.endTx();
	}

	public void setAuthor(String author) {
		this.author = author;
	}

	public Vector<String> getCreatedNodes() {
		return createdNodes;
	}

	public Vector<USStudyInstances> getPACSInstances() {
		return entityInstances;
	}

	public Vector<FEMModel> getFEMModels() {
		return entityModels;
	}

	public void cleanup() {
		if(!debug)
			removeDirectory(myWorkingDir);
	}

	public static boolean removeDirectory(String file) {
		File directory = new File(file);
		if (directory.isFile()) {
			return directory.delete();
		} else {

			String[] list = directory.list();

			// Some JVMs return null for File.list() when the
			// directory is empty.
			if (list != null) {
				for (int i = 0; i < list.length; i++) {
					File entry = new File(directory, list[i]);
					if (entry.isDirectory()) {
						if (!removeDirectory(entry.getPath()))
							return false;
					} else {
						entry.delete();
					}
				}
			}

			return directory.delete();
		}
	}

}
