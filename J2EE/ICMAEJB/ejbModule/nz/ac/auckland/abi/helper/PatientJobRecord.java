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
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Properties;
import java.util.Vector;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.zip.GZIPOutputStream;

import nz.ac.auckland.abi.businesslogic.ICMAPatientRecord;
import nz.ac.auckland.abi.dcm4chee.DCMAccessManager;
import nz.ac.auckland.abi.dcm4chee.InstanceRecord;
import nz.ac.auckland.abi.dcm4chee.PatientRecord;
import nz.ac.auckland.abi.dcm4chee.StudyRecord;
import nz.ac.auckland.abi.dicomprocessing.Dicom2Movie;
import nz.ac.auckland.abi.dicomprocessing.DicomSeries2Movie;
import nz.ac.auckland.abi.dicomprocessing.Moviemaker;
import nz.ac.auckland.abi.entities.FEMModel;
import nz.ac.auckland.abi.entities.MRStudyInstances;
import nz.ac.auckland.abi.entities.PACSStudy;
import nz.ac.auckland.abi.entities.Patient;
import nz.ac.auckland.abi.entities.USStudyInstances;
import nz.ac.auckland.abi.icmaconfiguration.ResourceConfigurationManager;

public class PatientJobRecord {
	private final String instancePrefix = "Instance";
	private PatientRecord record;
	private Hashtable<StudyRecord, Vector<InstanceRecord>> pstudies;
	private Hashtable<String, StudyRecord> pprocessID;
	private Hashtable<String, Moviemaker> pprocess;
	private Hashtable<String, Boolean> pprocessStatus;
	private Vector<USStudyInstances> usInstances;
	private Vector<MRStudyInstances> mrInstances;
	private Vector<PACSStudy> studyInstances;
	private Vector<FEMModel> entityModels;
	private Vector<String> createdNodes;
	private String author;
	private int lookBack = -1;
	private ResourceConfigurationManager rManager;

	private ICMAPatientRecord patientRecord;

	Logger log;

	public PatientJobRecord(PatientRecord pat, String author, ResourceConfigurationManager rcm, ICMAPatientRecord pr, int lb) throws Exception {
		lookBack = lb;
		rManager = rcm;
		patientRecord = pr;
		record = pat;
		String patientID = pat.getPatientID();
		this.author = author;
		pstudies = new Hashtable<StudyRecord, Vector<InstanceRecord>>();
		pprocessID = new Hashtable<String, StudyRecord>();
		pprocess = new Hashtable<String, Moviemaker>();
		pprocessStatus = new Hashtable<String, Boolean>();
		log = Logger.getLogger(this.getClass().getSimpleName());
		Vector<StudyRecord> studies = DCMAccessManager.getPatientStudies(patientID, lookBack);

		int workDirCtr = 0;
		for (StudyRecord rec : studies) {
			String studyID = rec.getStudyInstanceUID();
			String studyModalities = rec.getStudyModalities().toUpperCase();

			if (studyModalities.indexOf("US") > -1) {
				Vector<InstanceRecord> instances = DCMAccessManager.getStudyInstances(rec.getStudyInstanceUID(), lookBack);
				pstudies.put(rec, instances);
				String myWorkingDir = rManager.getDiskScratchSpace() + "/pip/" + getSantitizedPatientID(patientID) + workDirCtr++;
				Dicom2Movie processor = new Dicom2Movie();

				File workDir = new File(myWorkingDir);
				if (!workDir.exists()) {
					workDir.mkdirs();
				}
				processor.setOutputDirectory(myWorkingDir);

				for (int ctr = 0; ctr < instances.size(); ctr++) {
					InstanceRecord irec = instances.elementAt(ctr);
					if (irec.isCineInstance()) {

						processor.addUri(instancePrefix + ctr, DCMAccessManager.getInstanceWADOUrl(studyID, irec.getSeriesID(), irec.getSopIuid()));
					}
				}
				String processID = patientID + "#" + (workDirCtr - 1);
				Moviemaker movieprocessor = new Moviemaker(false);
				movieprocessor.setDicom2Movie(processor);
				pprocessID.put(processID, rec);
				pprocess.put(processID, movieprocessor);
				pprocessStatus.put(processID, new Boolean(false));
			}
			// Check for MR
			if (studyModalities.indexOf("MR") > -1) {
				// Unlike ultrasound dicom, the MR dicom frames are stored as a
				// series under the same series id
				// Start a new thread to load the series data into the database
				// Check for image position as SAX images may be bundled
				// together into a single series

				Vector<InstanceRecord> instances = DCMAccessManager.getStudyInstances(rec.getStudyInstanceUID(), lookBack);
				pstudies.put(rec, instances);
				String myWorkingDir = rManager.getDiskScratchSpace() + "/pip/MRI/" + getSantitizedPatientID(patientID) + workDirCtr++;
				File workDir = new File(myWorkingDir);
				if (!workDir.exists()) {
					workDir.mkdirs();
				}
				ArrayList<String> dicomFiles = new ArrayList<String>();
//				int ic = 0;
				for (InstanceRecord irec : instances) {
					String wurl = DCMAccessManager.getInstanceWADOUrl(studyID, irec.getSeriesID(), irec.getSopIuid());
/*					//If downloading files using curl takes longer 
					//So download files ahead
					File dicomFile = new File(workDir, "MR" + (ic++) + ".dcm");
					URL url = new URL(wurl);
					ReadableByteChannel rbc = Channels.newChannel(url.openStream());
					FileOutputStream fos = new FileOutputStream(dicomFile);
					fos.getChannel().transferFrom(rbc, 0, Long.MAX_VALUE);
					fos.close();
					rbc.close();
					dicomFiles.add(dicomFile.getAbsolutePath());*/
					dicomFiles.add(wurl);
				}

				DicomSeries2Movie processor = new DicomSeries2Movie();
				processor.addSeries(instancePrefix, dicomFiles);
				processor.setOutputDirectory(myWorkingDir);
				processor.outputJpeg();
				String processID = patientID + "#" + (workDirCtr - 1);
				Moviemaker movieprocessor = new Moviemaker(true);
				movieprocessor.setDicomSeries2Movie(processor);
				pprocessID.put(processID, rec);
				pprocess.put(processID, movieprocessor);
				pprocessStatus.put(processID, new Boolean(false));
			}
		}
	}

	private String getSantitizedPatientID(String patientID) {
		return patientID.replaceAll("[^A-Za-z0-9]", "");
	}

	public Hashtable<String, Moviemaker> getProcessRecord() {
		return pprocess;
	}

	public void setJobCompletionStatus(String processID) {
		if (pprocessStatus.containsKey(processID)) {
			pprocessStatus.put(processID, new Boolean(true));
		}
	}

	public boolean getCompletionStatus() {
		boolean completed = true;
		Enumeration<Boolean> elem = pprocessStatus.elements();

		while (elem.hasMoreElements() && completed) {
			completed = completed && elem.nextElement().booleanValue();
		}
		/*
		 * { ArrayList<String> kk = new
		 * ArrayList<String>(pprocessStatus.keySet()); for(String k: kk){
		 * log.log(Level.INFO,k+"\t"+pprocessStatus.get(k)); } }
		 */
		if (completed) {
			createdNodes = new Vector<String>();
			studyInstances = new Vector<PACSStudy>();
			usInstances = new Vector<USStudyInstances>();
			mrInstances = new Vector<MRStudyInstances>();
			entityModels = new Vector<FEMModel>();
			// Execute as thread to avoid transaction timeout
			Thread update = new Thread(new Runnable() {

				public void run() {
					int ctr = 0;
					String patientID = record.getPatientID();
					boolean loadingFailed = false;
					log.log(Level.FINE, "Update database thread started for " + patientID);
					// Update the database
					ArrayList<String> keys = new ArrayList<String>(pprocessID.keySet());
					for (String key : keys) {
						// Dicom2Movie movieMaker = pprocess.get(key);
						Moviemaker movieprocessor = pprocess.get(key);
						if (!movieprocessor.isSeriesType()) {
							Dicom2Movie movieMaker = movieprocessor.getDicom2Movie();
							StudyRecord studyRec = pprocessID.get(key);
							if (movieMaker.wasSuccessful()) {
								log.log(Level.INFO, "Movie maker was successful for " + studyRec.getStudyInstanceUID());
								String studyID = studyRec.getStudyInstanceUID();
								try {
									PACSStudy newStudy = new PACSStudy(patientID, studyRec.getStudyInstanceUID(), studyRec.getStudyDate(), studyRec.getStudyDescription());
									studyInstances.add(newStudy);
									Vector<InstanceRecord> instances = pstudies.get(studyRec);

									for (ctr = 0; ctr < instances.size(); ctr++) {
										InstanceRecord rec = instances.elementAt(ctr);
										if (!rec.isCineInstance())
											continue;
										String myName = instancePrefix + ctr;
										String workingDirectory = movieMaker.getOutputDirectory();
										String properties = workingDirectory + "/" + myName + ".properties";
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
													if (prop.getProperty("DICOMTYPE") == null) {
														// DICOMTYPE=SINGLE for
														// non
														// multiframe files and
														// these will not be
														// loaded
														String webm = workingDirectory + "/" + myName + ".webm";
														String mp4 = workingDirectory + "/" + myName + ".mp4";
														String oggv = workingDirectory + "/" + myName + ".ogv";
														String poster = workingDirectory + "/" + myName + "POSTER.jpg";

														ByteArrayOutputStream bos = new ByteArrayOutputStream();
														prop.storeToXML(bos, "DICOM");
														String propertyString = bos.toString();
														String instanceID = rec.getSopIuid();
														// Load them into CMS
														String webmN = rManager.addMovieToCMS(studyID, instanceID, "us.webm", new File(webm));
														String mp4N = rManager.addMovieToCMS(studyID, instanceID, "us.mp4", new File(mp4));
														String oggvN = rManager.addMovieToCMS(studyID, instanceID, "us.oggv", new File(oggv));
														String posterN = rManager.addImageToCMS(studyID, instanceID, "POSTER.jpg", new File(poster));

														createdNodes.add(webmN);
														createdNodes.add(mp4N);
														createdNodes.add(oggvN);
														createdNodes.add(posterN);

														String movieMetaData = prop.getProperty("TOTALMOVIEFRAMES", "") + "#" + prop.getProperty("MOVIEFRAMERATE", "") + "#"
																+ prop.getProperty("MOVIETIME", "");

														USStudyInstances instance = new USStudyInstances(studyID, instanceID, rec.getSeriesID(), posterN, mp4N, webmN, oggvN,
																movieMetaData, propertyString);
														if (prop.contains("PatientSize")) {
															instance.setSize(Double.parseDouble(prop.getProperty("PatientSize")));
														}
														if (prop.contains("PatientWeight")) {
															instance.setWeight(Double.parseDouble(prop.getProperty("PatientWeight")));
														}
														if(!prop.contains("ReferringPhysicianName")){
															prop.put("ReferringPhysicianName", record.getPhysicianName());
														}
														usInstances.add(instance);
													}
												} else {
													String viewEncoding = prop.getProperty("VIEWENCODING");
													String[] views = viewEncoding.split("#");
													String wDir = workingDirectory + "/" + myName;
													/*
													 * Hashtable<String,
													 * Properties> viewUrls =
													 * new Hashtable<String
													 * ,Properties>();
													 */
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
																String cmsXpath = rManager.addImageToCMS(studyID, rec.getSopIuid(), vRec[0] + "_" + j + ".jpg", new File(filename));
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

													// Check if metadata is an
													// xml
													// or dicom file
													String fileName = prop.getProperty("METADATAFILE");
													// Parse the xml file and
													// obtain
													// the exregion data
													ICMAMetaData processMetaData = new ICMAMetaData(fileName);
													Vector<String> exregionFiles = processMetaData.getEXRegionFiles();
													Properties urls = new Properties();
													// Load exregion files into
													// CMS
													for (int exctr = 0; exctr < exregionFiles.size(); exctr++) {
														String exr = exregionFiles.elementAt(exctr);
														String cmsXpath = rManager.addBytesToCMS(studyID, rec.getSopIuid(), "Heart." + exctr + ".exregion", "text/plain",
																exr.getBytes());
														urls.put("Heart." + exctr, cmsXpath);
														// System.out.println("Created "+cmsXpath.getPath());
														createdNodes.add(cmsXpath);
													}
													// Discard the author
													// settings
													// from the
													// fitting metadata
													processMetaData.setAuthor(author);
													processMetaData.setStatusAuthor(author);
													Properties metaProps = new Properties();
													metaProps.put("MODELJSON", processMetaData.getJSON(rec.getSopIuid(), viewUrls, urls));
													metaProps.put("MODELXML", processMetaData.getMetaData());
													if(!metaProps.contains("ReferringPhysicianName")){
														metaProps.put("ReferringPhysicianName", record.getPhysicianName());
													}

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

													FEMModel model = new FEMModel(patientID, studyID, rec.getSeriesID(), rec.getSopIuid(), processMetaData.getAuthor(),
															processMetaData.getModelAnnotation(), processMetaData.getModelName(), bosVU.toString(), bosEX.toString(), zipOut
																	.toByteArray());
													log.log(Level.INFO, "Created model " + processMetaData.getModelName());
													entityModels.add(model);
												}
											} catch (IOException ex) {
												log.log(Level.INFO, "Failed to load properties " + properties);
												ex.printStackTrace();
											}
										}
									}

								} catch (Exception e) {
									log.log(Level.INFO, "Failed to add study " + studyID + " of patient " + patientID + "\t" + e.toString());
									e.printStackTrace();
								}
							} else {
								log.log(Level.INFO, "Movie maker failed for " + studyRec.getStudyInstanceUID() + ". Continuing without study");
								/*
								 * log.log(Level.INFO,"Movie maker failed for "+
								 * studyRec
								 * .getStudyInstanceUID()+". Terminating update"
								 * ); log.log(Level.INFO,
								 * "******Failed to add patient " +
								 * record.getPatientID() + " record ");
								 * loadingFailed = true; break;
								 */}
						} else { // Mr type data
							DicomSeries2Movie movieMaker = movieprocessor.getDicomSeries2Movie();
							StudyRecord studyRec = pprocessID.get(key);
							if (movieMaker.wasSuccessful()) {
								log.log(Level.INFO, "Movie maker was successful for study " + studyRec.getStudyInstanceUID());
								// String studyID =
								// studyRec.getStudyInstanceUID();
								try {
									String studyID = studyRec.getStudyInstanceUID();
									PACSStudy newStudy = new PACSStudy(patientID, studyID, "MR", studyRec.getStudyDate(), studyRec.getStudyDescription());
									studyInstances.add(newStudy);
									String workingDirectory = movieMaker.getOutputDirectory();
									String properties = workingDirectory + "/" + instancePrefix + ".properties";
									// Access the properties file
									// This file contains the number of planes
									// withing the series
									// And their prefixes
									File propertiesFile = new File(properties);
									if (propertiesFile.exists()) {
										// Process the properties file for each
										// plane
										Properties prop = new Properties();
										prop.load(new FileInputStream(properties));
										int numplanes = Integer.parseInt(prop.getProperty("NUMPLANES"));
										// Check for necessary files
										for (int p = 1; p < numplanes; p++) {
											String pname = "PLANE" + p;
											String prefix = prop.getProperty(pname);
											if (prefix != null) {
												File props = new File(workingDirectory + "/" + prefix + ".properties");
												File webm = new File(workingDirectory + "/" + prefix + ".webm");
												File mp4 = new File(workingDirectory + "/" + prefix + ".mp4");
												File oggv = new File(workingDirectory + "/" + prefix + ".ogv");
												String poster = workingDirectory + "/" + prefix + "POSTER.jpg";

												ByteArrayOutputStream bos = new ByteArrayOutputStream();
												Properties propp = new Properties();
												propp.load(new FileInputStream(props));
												if(!propp.contains("ReferringPhysicianName")){
													propp.put("ReferringPhysicianName", record.getPhysicianName());
												}

												String instanceID = "" + p;
												// Load them into CMS
												String webmN = rManager.addMovieToCMS(studyID, instanceID, "us.webm", webm);
												String mp4N = rManager.addMovieToCMS(studyID, instanceID, "us.mp4", mp4);
												String oggvN = rManager.addMovieToCMS(studyID, instanceID, "us.oggv", oggv);
												String posterN = rManager.addImageToCMS(studyID, instanceID, "POSTER.jpg", new File(poster));

												createdNodes.add(webmN);
												createdNodes.add(mp4N);
												createdNodes.add(oggvN);
												createdNodes.add(posterN);
												//Load the image plane related images
												int numframes = Integer.parseInt(propp.getProperty("NUMFRAMES"));
												for(int fct=0;fct<numframes;fct++){
													String imgFileName = workingDirectory + "/" +prefix+String.format("%03d.jpg", fct);
													String planeimg = rManager.addImageToCMS(studyID, instanceID, "Img"+fct+".jpg", new File(imgFileName));
													createdNodes.add(planeimg);
													propp.put("IMG"+fct+"url", planeimg);
												}

												propp.storeToXML(bos, "DICOM");
												String propertyString = bos.toString();
												String imageplanedata = "COORD"+p;
												String movieMetaData = numframes+"#"+prop.getProperty(imageplanedata); //Hash delimited number of images and image plane corner positions
												MRStudyInstances instance = new MRStudyInstances(studyID, instanceID, propp.getProperty("SERIESUID"), propp
														.getProperty("POSITIONPATIENT_0"), propp.getProperty("ORIENTATIONPATIENT_0"), posterN, mp4N, webmN, oggvN, movieMetaData,
														propertyString);
												mrInstances.add(instance);
											} else {
												log.log(Level.INFO, "File " + properties + " does not exist!!");
												break;
											}
										}
									} else {
										log.log(Level.INFO, "File " + properties + " does not exist!!");
									}
								} catch (Exception exx) {
									log.log(Level.INFO, "Exception "+exx+" occured while adding patient "+record.getPatientID());
								}
							}
						}
					}
					if (!loadingFailed) {
						if (usInstances.size() > 0 || entityModels.size() > 0 || mrInstances.size() > 0) {
							// Add the patient to ensure foreign key consistency
							Patient entity = null;
							// When a patient is added to the db, a check is
							// made if the patient with the same id exists
							entity = new Patient(record.getPatientID(), record.getPatientName(), record.getPatientBirthDate(), record.getPatientSex());
							
							try {
								patientRecord.batchAdd(entity, studyInstances, usInstances, mrInstances, entityModels);
								// Clean up disk
								for (String key : keys) {
									Moviemaker movieMaker = pprocess.get(key);
									movieMaker.cleanUp();
								}
								log.log(Level.INFO, "Successfully added patient " + record.getPatientID() + " record ");
							} catch (Exception exx) {
								log.log(Level.INFO, "Failed to add patient " + record.getPatientID() + " record " + exx.toString());
								exx.printStackTrace();
								loadingFailed = true;// setup cleanup
							}
						}
					}

					if (loadingFailed) {// Clean up
						try {
							rManager.removeCMSNodes(createdNodes);
							// Clean up disk
							for (String key : keys) {
								Moviemaker movieMaker = pprocess.get(key);
								movieMaker.cleanUp();
							}
						} catch (Exception ex) {
							ex.printStackTrace();
						}
					}
				}
			});

			update.start();
		}
		return completed;
	}

	public Vector<USStudyInstances> getPACSInstances() {
		return usInstances;
	}

	public Vector<MRStudyInstances> getMRInstances() {
		return mrInstances;
	}

	public Vector<FEMModel> getFEMModels() {
		return entityModels;
	}
}
