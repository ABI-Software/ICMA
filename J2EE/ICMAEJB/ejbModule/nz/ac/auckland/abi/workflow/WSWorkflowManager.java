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
package nz.ac.auckland.abi.workflow;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Hashtable;
import java.util.List;
import java.util.Vector;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.zip.GZIPOutputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

import javax.annotation.Resource;
import javax.ejb.EJB;
import javax.ejb.LocalBean;
import javax.ejb.Stateless;
import javax.ejb.TransactionAttribute;
import javax.ejb.TransactionAttributeType;
import javax.naming.InitialContext;
import javax.sql.DataSource;

import nz.ac.auckland.abi.businesslogic.FEMModelsBeanRemote;
import nz.ac.auckland.abi.entities.FEMModel;
import nz.ac.auckland.abi.helper.ProcessSpeckleTrackingOutput;
import nz.ac.auckland.abi.helper.SpeckleToFEMXMLInputGenerator;
import nz.ac.auckland.abi.helper.WSDicomDownloader;
import nz.ac.auckland.abi.icmaconfiguration.CMSContent;
import nz.ac.auckland.abi.icmaconfiguration.ResourceConfigurationManager;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.json.simple.parser.JSONParser;

/**
 * Session Bean implementation class WSWorkflowManager
 */
@Stateless
@LocalBean
public class WSWorkflowManager implements WSWorkflowManagerRemote {

	@EJB
	private ResourceConfigurationManager rManger;

	@EJB
	private FEMModelsBeanRemote modelBean;

	@Resource(mappedName = "java:jboss/datasources/MySQLDS")
	DataSource dataSource;

	private String wadoPort = null;

	private String hostname = null;

	private Logger log = null;

	public WSWorkflowManager() throws Exception {
		InitialContext context = new InitialContext();
		wadoPort = (String) context.lookup("java:global/PACSWADOPORT");
		hostname = (String) context.lookup("java:global/PACSHOSTNAME");
		log = Logger.getLogger(this.getClass().getSimpleName());
	}

	@TransactionAttribute(TransactionAttributeType.NOT_SUPPORTED)
	public String trackSpeckles(JSONObject object) throws Exception {

		String modelName = (String) object.get("modelname");
		if (modelName == null) {
			modelName = "Generic" + Math.random();
		}
		// Get the patient, study and series id
		WSSpeckleTrackingXMLInputEncoder xmlInput = new WSSpeckleTrackingXMLInputEncoder();
		xmlInput.setModelName(modelName);

		boolean targetSet = false;
		String[] ids = null;
		String[] views = { "aplax", "tch", "fch", "saxapex", "saxmid", "saxbase" };
		log.log(Level.FINE, "Received json Object " + object.toJSONString());
		for (int ctr = 0; ctr < views.length; ctr++) {
			if (object.containsKey(views[ctr])) {
				try {
					JSONObject image = (JSONObject) object.get(views[ctr]);
					String instanceID = (String) image.get("imageid");
					if (ids == null) {
						ids = getIds(instanceID);
					}
					StringBuffer buf = new StringBuffer();
					JSONArray jsonArray = (JSONArray) image.get("coordinates");
					if (jsonArray != null) {
						int len = jsonArray.size();
						for (int i = 0; i < len - 1; i++) {
							JSONObject coord = (JSONObject) jsonArray.get(i);
							buf.append(coord.get("x") + ", " + coord.get("y") + ",");
						}
						JSONObject coord = (JSONObject) jsonArray.get(len - 1);
						buf.append(coord.get("x") + ", " + coord.get("y"));
					}
					String bpm = null;
					try {
						Long lbp = (Long) image.get("bpm");
						bpm = "" + lbp;
					} catch (Exception exx) {
						exx.printStackTrace();
					}
					String edFrame = (String) image.get("rwavestart");
					String ecFrame = (String) image.get("rwaveend");
					String viewName = views[ctr].toUpperCase();
					xmlInput.addView(viewName, ids[0], ids[1], ids[2], instanceID, buf.toString(), edFrame, ecFrame, bpm, hostname, wadoPort,
							!targetSet && !views[ctr].startsWith("sax"));
					if (image.get("esframe") != null) {// Load esframe data
						String esframe = (String) image.get("esframe");
						buf = new StringBuffer();
						jsonArray = (JSONArray) image.get("escoordinates");
						if (jsonArray != null) {
							int len = jsonArray.size();
							for (int i = 0; i < len - 1; i++) {
								JSONObject coord = (JSONObject) jsonArray.get(i);
								buf.append(coord.get("x") + ", " + coord.get("y") + ",");
							}
							JSONObject coord = (JSONObject) jsonArray.get(len - 1);
							buf.append(coord.get("x") + ", " + coord.get("y"));
						}
						xmlInput.addESFrameData(viewName, esframe, buf.toString());
					}
					if (!targetSet)
						targetSet = !views[ctr].startsWith("sax");
				} catch (Exception exx) {
					// Ignore that view
					exx.printStackTrace();
				}
			} else {
				log.log(Level.FINE, "Object does not contain " + views[ctr]);
			}
		}

		String xml = xmlInput.getXML();
		// log.log(Level.INFO, "Converted to XML form as \n" + xml);

		SpeckleTrackingInterface sti = new SpeckleTrackingInterface();
		sti.submitJob(xml);

		WSWorkflowInfo wInflow = new WSWorkflowInfo();
		wInflow.setPatientID(ids[0]);
		wInflow.setStudyID(ids[1]);
		wInflow.setSeriesID(ids[2]);
		if (object.get("username") != null)
			wInflow.setModelAuthor((String) object.get("username"));
		wInflow.setSpeckleTrackingInterface(sti);
		rManger.addWsWorkflow(wInflow.getWorkFlowID(), wInflow);
		return wInflow.getWorkFlowID();
	}

	@TransactionAttribute(TransactionAttributeType.NOT_SUPPORTED)
	public JSONObject fitMRIModel(JSONObject jsonObject) throws Exception {
		JSONObject result = new JSONObject();
		try {
			String modelName = (String) jsonObject.get("modelname");
			if (modelName == null) {
				modelName = "Generic" + Math.random();
			}
			String workDir = rManger.getDiskScratchSpace();
			File wDir = new File(workDir, "MRIImages0");
			int fctr = 1;
			while (wDir.exists()) {
				wDir = new File(workDir, "MRIImages" + fctr++);
			}
			wDir.mkdirs();
			fctr = 0;
			// Parse JSON and create XML Object
			// Note that the downstream program may not be able to access
			// the urls due to security privileges
			// So load them into an archive

			ByteArrayOutputStream bos = new ByteArrayOutputStream();
			ZipOutputStream zos = new ZipOutputStream(bos);

			WSMRIModelFittingXMLInputEncoder encoder = new WSMRIModelFittingXMLInputEncoder();
			encoder.setModelName((String) jsonObject.get("modelname"));
			String studyid = (String) jsonObject.get("studyUID");
			String patientid = (String) jsonObject.get("patientID");
			ArrayList<String> seriesids = new ArrayList<String>();
			JSONArray jsonArray = (JSONArray) jsonObject.get("planes");
			if (jsonArray != null) {
				int len = jsonArray.size();
				for (int i = 0; i < len; i++) {
					JSONObject planes = (JSONObject) jsonArray.get(i);
					String edtime = "";
					try {
						edtime = "" + ((Double) planes.get("edtime"));
					} catch (ClassCastException ex) {
						edtime = "" + ((Long) planes.get("edtime"));
					}
					String estime = "";
					try {
						estime = "" + ((Double) planes.get("estime"));
					} catch (ClassCastException ex) {
						estime = "" + ((Long) planes.get("estime"));
					}

					String endcycle = "";
					try {
						endcycle = "" + ((Double) planes.get("endcycle"));
					} catch (ClassCastException ex) {
						endcycle = "" + ((Long) planes.get("endcycle"));
					}

					String server = (String) planes.get("server");
					String seriesid = (String) planes.get("seriesid");
					String type = (String) planes.get("type");
					String planeid = (String) planes.get("planeid");

					seriesids.add(seriesid);

					JSONObject frames = (JSONObject) planes.get("frames");
					int numframes = Integer.parseInt((String) frames.get("NUMFRAMES"));

					String[] urls = new String[numframes];
					for (int j = 0; j < numframes; j++) {
						String uri = (String) frames.get("" + j);
						String nodepath = uri.substring(uri.indexOf("/ICMADOCS") + 10); // Remove
						CMSContent myContent = rManger.getCMSNodeAt(nodepath);
						/*
						 * File imgFile = new File(wDir, "Image" + (fctr++) +
						 * ".jpg"); FileOutputStream fos = new
						 * FileOutputStream(imgFile);
						 * fos.write(myContent.output.toByteArray());
						 * fos.close(); urls[j] = imgFile.getName();
						 */
						String file = "Image" + (fctr++) + ".jpg";
						urls[j] = file;
						ZipEntry ze = new ZipEntry(file);
						zos.putNextEntry(ze);
						zos.write(myContent.output.toByteArray());
						zos.closeEntry();
					}

					String fwidth = (String) planes.get("WIDTH");
					String fheight = (String) planes.get("HEIGHT");

					String[] tlc, blc, trc, brc;
					trc = ((String) frames.get("TRC")).split(",");
					blc = ((String) frames.get("BLC")).split(",");
					tlc = ((String) frames.get("TLC")).split(",");
					brc = ((String) frames.get("BRC")).split(",");
					String[] edlm = null, eslm = null;
					JSONArray edlmA = (JSONArray) planes.get("EDLM");
					if (edlmA != null) {
						int size = edlmA.size();
						edlm = new String[size];
						for (int ei = 0; ei < size; ei++) {
							JSONArray coord = (JSONArray) edlmA.get(ei);
							edlm[ei] = coord.get(0) + "," + coord.get(1) + "," + coord.get(2);
						}
					}
					JSONArray eslmA = (JSONArray) planes.get("ESLM");
					if (eslmA != null) {
						int size = eslmA.size();
						eslm = new String[size];
						for (int ei = 0; ei < size; ei++) {
							JSONArray coord = (JSONArray) eslmA.get(ei);
							eslm[ei] = coord.get(0) + "," + coord.get(1) + "," + coord.get(2);
						}
					}
					encoder.addPlane(planeid, urls, tlc, trc, blc, brc, seriesid);
					if (edlm != null)
						encoder.addPlaneEDLM(planeid, edlm);
					if (eslm != null)
						encoder.addPlaneESLM(planeid, eslm);
					if (!endcycle.equalsIgnoreCase("null"))
						encoder.addPlaneENDCycleTime(planeid, endcycle);
					if (!edtime.equalsIgnoreCase("null"))
						encoder.addPlaneEDTime(planeid, edtime);
					if (!estime.equalsIgnoreCase("null"))
						encoder.addPlaneESTime(planeid, estime);
					if (type != null)
						encoder.addPlaneType(planeid, type);
					if (fwidth != null)
						encoder.addPlaneWidth(planeid, fwidth);
					if (fheight != null)
						encoder.addPlaneHeight(planeid, fheight);
				}
			}
			zos.close();
			encoder.addServer(wDir.getAbsolutePath());
			String seriesprefix = seriesids.get(0);
			for (int i = 1; i < seriesids.size(); i++) {
				seriesprefix = findLargestCommonPrefix(seriesprefix, seriesids.get(i));
			}
			if (seriesprefix.length() == 0) {
				seriesprefix = "MRISeries";
			}

			String xml = encoder.getXML();
			// log.log(Level.INFO, "Converted to XML form as \n" + xml);
			//Create a copy of the json object and pass series id
			JSONObject viewObject = new JSONObject(jsonObject);
			viewObject.put("seriesid",seriesprefix+Math.random());
			MRIModelFittingInterface mrfit = new MRIModelFittingInterface(viewObject);
			String workflowid = mrfit.submitJob(xml, bos.toByteArray());
			WSWorkflowInfo wInflow = new WSWorkflowInfo();
			wInflow.setPatientID(patientid);
			wInflow.setStudyID(studyid);
			wInflow.setSeriesID(seriesprefix);
			if (jsonObject.get("username") != null)
				wInflow.setModelAuthor((String) jsonObject.get("username"));
			wInflow.setMRIModelFittingInterface(mrfit, workflowid);
			rManger.addWsWorkflow(wInflow.getWorkFlowID(), wInflow);

			result.put("workflowID", wInflow.getWorkFlowID());
		} catch (Exception exx) {
			StringWriter sw = new StringWriter();
			PrintWriter pw = new PrintWriter(sw);
			exx.printStackTrace(pw);
			result.put("EXCEPTION", sw.toString());
		}
		return result;
	}

	private String findLargestCommonPrefix(String a, String b) {
		int minLength = Math.min(a.length(), b.length());
		for (int i = 0; i < minLength; i++) {
			if (a.charAt(i) != b.charAt(i)) {
				return a.substring(0, i);
			}
		}
		return a.substring(0, minLength);
	}

	@TransactionAttribute(TransactionAttributeType.NOT_SUPPORTED)
	public String fitModelToSpeckles(JSONObject object, String workflowID) throws Exception {
		WSWorkflowInfo owf = rManger.getWsWorkFlowInfo(workflowID);
		//Create a new workflow
		WSWorkflowInfo info = new WSWorkflowInfo();
		info.setPatientID(owf.getPatientID());
		info.setStudyID(owf.getStudyID());
		info.setSeriesID(owf.getSeriesID());
		rManger.addWsWorkflow(info.getWorkFlowID(), info);
		
		if (object.get("username") != null)
			info.setModelAuthor((String) object.get("username"));
		SpeckleToFEMInterface smi = null;
		while (smi == null) {
			try {
				smi = new SpeckleToFEMInterface();
			} catch (Exception exx) {
				log.log(Level.INFO, "Failed to create speckle to FEM service interface handle ");
				Thread.sleep(10000);
			}
		}
		info.setSpeckleToFEMInterface(smi);
		// Get the xml for fitting
		SpeckleToFEMXMLInputGenerator gen = new SpeckleToFEMXMLInputGenerator(object);
		String input = gen.getXML();
		log.log(Level.FINE, "Speckles for model");
		log.log(Level.FINE, object.toJSONString());
		log.log(Level.FINE, input);
		log.log(Level.FINE, "DICOM PATH " + object.get("WS.DCM"));
		// Get the dicom file
		CMSContent content = rManger.getCMSNodeAt((String) object.get("WS.DCM"));
		// Since dummy is created for parallel load, check the size is larger
		// than 1 byte to proceed
		while (content.output.size() < 1) {
			log.log(Level.FINE, "Wating for WS.DCM dicom to be loaded for " + workflowID);
			Thread.sleep(10000);
			content = rManger.getCMSNodeAt((String) object.get("WS.DCM"));
		}
		// Compress the dicom for saving memory
		ByteArrayOutputStream bos = new ByteArrayOutputStream();
		GZIPOutputStream gos = new GZIPOutputStream(bos);
		gos.write(content.output.toByteArray());
		gos.close();
		byte[] dcmData = bos.toByteArray();
		smi.submitJob(input, dcmData);
		return info.getWorkFlowID();
	}

	@TransactionAttribute(TransactionAttributeType.NOT_SUPPORTED)
	public JSONObject getProgress(String workflowID) throws Exception {
		JSONObject result = new JSONObject();
		WSWorkflowInfo info = rManger.getWsWorkFlowInfo(workflowID);
		SpeckleTrackingInterface sti = info.getSpeckleTrackingInterface();
		SpeckleToFEMInterface smi = info.getSpeckleToFEMInterface();
		MRIModelFittingInterface mrifit = info.getMRIModelFittingInterface();
		if (sti != null) {
			double progress = sti.getProgress();
			result.put("tracking", "" + progress);
		}
		if (smi != null) {
			double progress = smi.getProgress();
			result.put("fitting", "" + progress);
		}
		if (mrifit != null) {
			double progress = mrifit.getProgress(info.getMRIFitWorkFlowId());
			result.put("mrifitting", "" + progress);
		}

		return result;
	}

	@TransactionAttribute(TransactionAttributeType.NOT_SUPPORTED)
	public JSONObject getSpeckleTrackingOutput(String workflowID) throws Exception {
		JSONObject result = new JSONObject();
		WSWorkflowInfo info = rManger.getWsWorkFlowInfo(workflowID);
		String wfID = info.getWorkFlowID();
		String timeStamp = "" + info.getTimeStamp();
		if (info.getStiStatus() != true) {// True if uploaded to docs
			SpeckleTrackingInterface sti = info.getSpeckleTrackingInterface();
			try {
				Hashtable<String, byte[]> images = sti.getImages();
				List<String> ikeys = new ArrayList<String>(images.keySet());
				for (String key : ikeys) {
					rManger.addWorkFlowDocs(wfID, timeStamp, key, "image/jpeg", images.get(key));
				}
				// rManger.addWorkFlowDocs(wfID, timeStamp, "WS.DCM",
				// "application/dicom", sti.getDicom());
				rManger.addWorkFlowDocs(wfID, timeStamp, "ST.XML", "application/xml", sti.getXML().getBytes());
				// Start a thread to load the ws dicom, this ensures that the
				// client is not waiting while the dicom is loaded
				// Create dummy record for the workflow and then load dicom file
				// in the same path
				rManger.addWorkFlowDocs(wfID, timeStamp, "WS.DCM", "application/dicom", "".getBytes());
				WSDicomDownloader ddown = new WSDicomDownloader(rManger, wfID, timeStamp, "WS.DCM", sti);
				Thread downloader = new Thread(ddown);
				downloader.start();

			} catch (Exception exx) {
				info.setStiException(exx);
			}
			info.setStiStatus(true);
		}
		if (info.getStiException() == null) {// No errors in the process
			List<String> paths = rManger.getWorkFlowDocs(wfID, timeStamp);
			String[] views = { "APLAX", "FCH", "TCH", "SAXAPEX", "SAXMID", "SAXBASE" };
			for (String str : paths) {
				if (str.indexOf("WS.DCM") > 0) {
					result.put("WS.DCM", str);
					continue;
				}
				if (str.indexOf("ST.XML") > 0) {
					result.put("ST.XML", str);
					continue;
				}
				for (int i = 0; i < views.length; i++) {
					if (str.indexOf(views[i]) > 0) {
						JSONArray arr = (JSONArray) result.get(views[i]);
						if (arr != null) {
							arr.add(str);
						} else {
							arr = new JSONArray();
							result.put(views[i], arr);
							arr.add(str);
						}
						break;
					}
				}
			}

			// Convert the data into client processable json format
			// Convert the xml output to motion history
			CMSContent content = rManger.getCMSNodeAt((String) result.get("ST.XML"));
			ProcessSpeckleTrackingOutput psto = new ProcessSpeckleTrackingOutput(new String(content.output.toByteArray(), "UTF-8"));
			JSONObject xtobject = psto.getJSON();
			xtobject.put("WS.DCM", result.get("WS.DCM"));

			for (int i = 0; i < views.length; i++) {
				final String viewName = views[i];
				JSONArray varr = (JSONArray) result.get(views[i]);
				if (varr != null) {
					String[] urls = (String[]) varr.toArray(new String[varr.size()]);
					Arrays.sort(urls, new Comparator<String>() {
						@Override
						public int compare(String o1, String o2) {
							int bx = o1.lastIndexOf(viewName) + viewName.length();
							int ex = o1.lastIndexOf(".JPG");
							int b2x = o2.lastIndexOf(viewName) + viewName.length();
							int e2x = o2.lastIndexOf(".JPG");
							try {
								int val1 = Integer.parseInt(o1.substring(bx, ex));
								int val2 = Integer.parseInt(o2.substring(b2x, e2x));
								return val1 - val2;
							} catch (Exception exx) {
								exx.printStackTrace();
								return 0;
							}
						}
					});
					for (int j = 0; j < urls.length; j++) {
						varr.set(j, urls[j]);
					}
					JSONObject vtype = (JSONObject) xtobject.get(viewName);
					vtype.put("URLS", varr);
				}

			}
			return xtobject;
		} else {
			JSONObject xtobject = new JSONObject();
			StringWriter sw = new StringWriter();
			PrintWriter pw = new PrintWriter(sw);
			info.getStiException().printStackTrace(pw);
			xtobject.put("EXCEPTION", sw.toString());
			return xtobject;
		}

	}

	@TransactionAttribute(TransactionAttributeType.NOT_SUPPORTED)
	public JSONObject getModelFittingOutput(String workflowID) throws Exception {
		JSONObject result = new JSONObject();
		WSWorkflowInfo info = rManger.getWsWorkFlowInfo(workflowID);
		if (info.getType().equalsIgnoreCase("US")) {
			if (info.getSMiStatus() != true) {// True if uploaded to icmadocs
				SpeckleToFEMInterface smi = info.getSpeckleToFEMInterface();
				try {
					String scratch = rManger.getDiskScratchSpace();
					File scratchDir = new File(scratch + "/Fit" + Math.random());
					while (scratchDir.exists()) {
						scratchDir = new File(scratch + "/Fit" + Math.random());
					}
					scratchDir.mkdirs();
					File dicomFile = new File(scratchDir, "FEM.DCM");
					FileOutputStream fos = new FileOutputStream(dicomFile);
					fos.write(smi.getDicom());
					fos.close();
					// System.out.println("Adding workflow "+info.getWorkFlowID()+" dicomfile is "+dicomFile.getAbsolutePath());
					String instanceID = modelBean.addModel(dicomFile, info.getPatientID(), info.getStudyID(), info.getSeriesID(), info.getModelAuthor());
					//log.log(Level.INFO,"Model instance id is "+instanceID);
					info.setInstanceID(instanceID);
					try {
						ResourceConfigurationManager.removeDirectory(scratchDir.getAbsolutePath());
					} catch (Exception exx1) {

					}
				} catch (Exception exx) {
					exx.printStackTrace();
					info.setSMiException(exx);
				}
				info.setStiStatus(true);
			}
			if (info.getSMiException() == null) {// No process errors
				if (info.getInstanceID() != null) {
					FEMModel model = modelBean.getModel(info.getInstanceID());
					log.log(Level.INFO,"Bundling new model \""+model.getModelName()+"\" for rendering; model uid "+model.getId());
					JSONParser parser = new JSONParser();
					JSONObject modelj = (JSONObject) parser.parse(model.getJSON());
					modelj.put("workflowId", info.getWorkFlowID());
					result = modelj;
				} else {
					result.put("status", "ERROR");
					result.put("shorterror", "Failed to fit");
					result.put("message", "Dicom instance is not available found");
				}
			} else {
				StringWriter sw = new StringWriter();
				PrintWriter pw = new PrintWriter(sw);
				info.getSMiException().printStackTrace();

				info.getSMiException().printStackTrace(pw);

				result.put("EXCEPTION", sw.toString());
			}

		} else {
			if (info.getMriUpdated() != true) {// True if uploaded to icmadocs
				MRIModelFittingInterface mrifit = info.getMRIModelFittingInterface();
				String mrifitWorkFlowID = info.getMRIFitWorkFlowId();
				try {
					String scratch = rManger.getDiskScratchSpace();
					File scratchDir = new File(scratch + "/Fit" + Math.random());
					while (scratchDir.exists()) {
						scratchDir = new File(scratch + "/Fit" + Math.random());
					}
					scratchDir.mkdirs();
					File dicomFile = new File(scratchDir, "FEM.DCM");
					FileOutputStream fos = new FileOutputStream(dicomFile);
					fos.write(mrifit.getDicom(mrifitWorkFlowID));
					fos.close();
					// System.out.println("Adding workflow "+info.getWorkFlowID()+" dicomfile is "+dicomFile.getAbsolutePath());
					String instanceID = modelBean.addModel(dicomFile, mrifit.getViewData(), info.getModelAuthor());
					// System.out.println("Model instance id is "+instanceID);
					info.setInstanceID(instanceID);
					try {
						ResourceConfigurationManager.removeDirectory(scratchDir.getAbsolutePath());
					} catch (Exception exx1) {

					}
				} catch (Exception exx) {
					exx.printStackTrace();
					info.setSMiException(exx);
				}
				info.setStiStatus(true);
			}
			if (info.getSMiException() == null) {// No process errors
				if (info.getInstanceID() != null) {
					FEMModel model = modelBean.getModel(info.getInstanceID());
					JSONParser parser = new JSONParser();
					JSONObject modelj = (JSONObject) parser.parse(model.getJSON());
					modelj.put("workflowId", info.getWorkFlowID());
					result = modelj;
				} else {
					result.put("status", "ERROR");
					result.put("shorterror", "Failed to fit");
					result.put("message", "Dicom instance is not available found");
				}
			} else {
				StringWriter sw = new StringWriter();
				PrintWriter pw = new PrintWriter(sw);
				info.getSMiException().printStackTrace();

				info.getSMiException().printStackTrace(pw);

				result.put("EXCEPTION", sw.toString());
			}
		}
		return result;
	}

	@TransactionAttribute(TransactionAttributeType.NOT_SUPPORTED)
	public void cleanup(String workflowID) throws Exception {
		WSWorkflowInfo info = rManger.getWsWorkFlowInfo(workflowID);
		SpeckleTrackingInterface sti = info.getSpeckleTrackingInterface();
		String wfID = info.getWorkFlowID();
		String timeStamp = "" + info.getTimeStamp();
		if (info.getStiStatus() == true) { // Clean up these docs
			Vector<String> paths = new Vector<String>(rManger.getWorkFlowDocs(wfID, timeStamp));
			rManger.removeCMSNodes(paths);
		}
		sti.cleanUp();
	}

	public String saveModel(String workflowID) throws Exception {
		// The calling entity is expected to have saved the model
		// All that the workflowManager does is to clean up
		rManger.removeWsWorkFlow(workflowID);
		return "{\"status\":\"success\"}";
	}

	public String discardModel(String workflowID) throws Exception {
		WSWorkflowInfo info = rManger.getWsWorkFlowInfo(workflowID);
		if (info.getInstanceID() != null) {
			try {
				FEMModel model = modelBean.getModel(info.getInstanceID());
				modelBean.removeFEMModel(model, this.getClass().getSimpleName());
				rManger.removeWsWorkFlow(workflowID);
			} catch (Exception exx) { // TODO report to admin
				return "{\"status\":\"error\",\"shorterror\":\"Failed to discard\",\"message\":\"" + exx + "\"}";
			}
		}
		return "{\"status\":\"success\"}";
	}

	private String[] getIds(String instanceID) throws Exception {
		String selectSQL = "SELECT patient.pat_id, study.study_id, instances.series_id FROM USStudyInstances as instances, PACSStudy as study, Patient as patient where instances.study_id=study.study_id and study.pat_id=patient.pat_id and instances.instance_id = ?";
		Connection con = dataSource.getConnection();
		PreparedStatement preparedStatement = con.prepareStatement(selectSQL);
		preparedStatement.setString(1, instanceID);
		ResultSet rs = preparedStatement.executeQuery();
		rs.next();
		String[] ids = new String[3];
		ids[0] = rs.getString(1);
		ids[1] = rs.getString(2);
		ids[2] = rs.getString(3);
		con.close();
		return ids;

	}
}
