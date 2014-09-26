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
package nz.ac.auckland.abi.businesslogic;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.URL;
import java.text.SimpleDateFormat;
import java.util.List;
import java.util.Properties;
import java.util.Vector;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.zip.GZIPOutputStream;

import javax.ejb.EJB;
import javax.ejb.Stateless;
import javax.ejb.TransactionAttribute;
import javax.ejb.TransactionAttributeType;
import javax.persistence.EntityManager;
import javax.persistence.PersistenceContext;
import javax.persistence.Query;

import org.json.simple.JSONObject;

import nz.ac.auckland.abi.dcm4chee.DCMAccessManager;
import nz.ac.auckland.abi.dcm4chee.PACSStudyOperations;
import nz.ac.auckland.abi.entities.FEMModel;
import nz.ac.auckland.abi.entities.FEMModelModification;
import nz.ac.auckland.abi.entities.PACSStudy;
import nz.ac.auckland.abi.entities.Patient;
import nz.ac.auckland.abi.helper.MRIFitData;
import nz.ac.auckland.abi.helper.PACSInstanceprocessor;
import nz.ac.auckland.abi.icmaconfiguration.CMSContent;
import nz.ac.auckland.abi.icmaconfiguration.ResourceConfigurationManager;

/**
 * Session Bean implementation class FEMModelsBean
 */
@Stateless
public class FEMModelsBean implements FEMModelsBeanRemote {

	@PersistenceContext(unitName = "ICMADB")
	private EntityManager entityManager;

	@EJB
	private ResourceConfigurationManager resourceManger;

	Logger log = null;

	/**
	 * Default constructor.
	 */
	public FEMModelsBean() {
		log = Logger.getLogger(this.getClass().getSimpleName());
	}

	// @TransactionAttribute(TransactionAttributeType.REQUIRES_NEW)
	public void addFEMModel(FEMModel femModel, String dicom, String author) {
		log.log(Level.INFO, "Adding model " + femModel.getModelName());
		try {
			entityManager.persist(femModel);
		} catch (javax.persistence.PersistenceException pex) {
			entityManager.merge(femModel);
			log.log(Level.INFO, "Persistence failed " + femModel.getModelName());
		}
		String modelPk = "" + femModel.getPk();
		FEMModelModification modTx = new FEMModelModification(modelPk, author, FEMModelModification.submit);
		String metaData = null;
		try {
			if (dicom != null) {
				byte[] fileData = java.nio.file.Files.readAllBytes(java.nio.file.FileSystems.getDefault().getPath(dicom));
				metaData = resourceManger.addBytesToCMS(femModel.getStudyID(), femModel.getSeriesID(), femModel.getId(), "application/dicom", fileData);
				modTx.setAction(FEMModelModification.create); // To distinguish
																// between newly
																// created
																// models and
																// existing
																// models
			} else {
				String instanceURL = DCMAccessManager.getInstanceWADOUrl(femModel.getStudyID(), femModel.getSeriesID(), femModel.getId());
				InputStream resultInStream = null;
				ByteArrayOutputStream resultOutStream = new ByteArrayOutputStream();
				URL imageUrl = new URL(instanceURL);

				resultInStream = imageUrl.openStream();
				byte[] buffer = new byte[4096];
				int bytes_read;
				while ((bytes_read = resultInStream.read(buffer)) != -1) {
					resultOutStream.write(buffer, 0, bytes_read);
				}
				resultOutStream.flush();
				metaData = resourceManger.addBytesToCMS(femModel.getStudyID(), femModel.getSeriesID(), femModel.getId(), "application/dicom", resultOutStream.toByteArray());
			}

		} catch (Exception dicomEx) {
			dicomEx.printStackTrace();
			Logger log = Logger.getLogger(this.getClass().getName());
			log.log(Level.INFO, "Exception occured while adding model " + dicomEx + " continuing with null meta data");
		}
		log.log(Level.INFO, "Adding model metadata\t" + femModel.getModelName());
		if (metaData != null)
			modTx.setMetadata(metaData);
		entityManager.persist(modTx);
	}

	@TransactionAttribute(TransactionAttributeType.REQUIRES_NEW)
	public void removeFEMModel(FEMModel femModel, String author) {

		if (author.equalsIgnoreCase("purge")) { // Leave no trace
			try {
				// Remove from CMS
				String metaData = getCMSNode(femModel);
				Vector<String> nodes = new Vector<String>();
				nodes.add(metaData);
				//Get the associated image and exregion files
				String[] imgURLs = femModel.getImageUris();
				for(int i=0;i<imgURLs.length;i++){
					nodes.add(imgURLs[i]);
				}
				String[] fieldMLURLs = femModel.getImageUris();
				for(int i=0;i<fieldMLURLs.length;i++){
					nodes.add(fieldMLURLs[i]);
				}				
				resourceManger.removeCMSNodes(nodes);
			} catch (Exception exx) {
				Logger log = Logger.getLogger(this.getClass().getName());
				log.log(Level.INFO, "Exception occured while deleteing model " + femModel + " from CMS " + exx + " continuing");
			}
			entityManager.remove(entityManager.merge(femModel));
			// Remove history from modifications
			String modelPk = "" + femModel.getPk();
			String q = "DELETE from  FEMModelModification where fem_model_pk = :modelPk";
			Query query = entityManager.createNativeQuery(q).setParameter("modelPk", modelPk);
			query.executeUpdate();
		} else {
			// Get the FEMmodel DICOM
			try {
				String modelPk = "" + femModel.getPk();
				entityManager.remove(entityManager.merge(femModel));
				if (resourceManger.syncModelsWithPACS()) {
					// Remove instance from PACS
					Logger log = Logger.getLogger(this.getClass().getName());
					log.log(Level.INFO, "The PACS instance was not deleted, feature yet to be coded");
				}
				// Update the records
				FEMModelModification modTx = new FEMModelModification(modelPk, author, FEMModelModification.delete);

				entityManager.persist(modTx);
			} catch (Exception exx) {
				Logger log = Logger.getLogger(this.getClass().getName());
				log.log(Level.INFO, "Exception occured while deleteing model " + exx);
			}
		}
	}

	@TransactionAttribute(TransactionAttributeType.REQUIRES_NEW)
	public void updateModel(FEMModel femModel, String author) {
		try {
			entityManager.persist(femModel);
		} catch (javax.persistence.PersistenceException pex) {
			entityManager.merge(femModel);
		}
		String modelPk = "" + femModel.getPk();
		FEMModelModification modTx = new FEMModelModification(modelPk, author, FEMModelModification.modified);
		entityManager.persist(modTx);
		entityManager.flush();
		log.log(Level.INFO, "Model update completed");
	}

	public List<FEMModel> getPatientModels(Patient patient) {
		String q = "SELECT p from " + FEMModel.class.getName() + " p where p.patientID= :patient_id";
		Query query = entityManager.createQuery(q).setParameter("patient_id", patient.getId());
		List<FEMModel> models = query.getResultList();
		return models;
	}

	public List<FEMModel> getStudyModels(PACSStudy study) {
		String q = "SELECT p from " + FEMModel.class.getName() + " p where p.studyID = :study_id";
		Query query = entityManager.createQuery(q).setParameter("study_id", study.getStudyID());
		List<FEMModel> models = query.getResultList();
		return models;
	}

	public FEMModel getModel(String instanceID) {
		String q = "SELECT p from " + FEMModel.class.getName() + " p where p.id = :sopid";
		Query query = entityManager.createQuery(q).setParameter("sopid", instanceID);
		List<FEMModel> models = query.getResultList();
		if (models.size() > 0) {
			return models.get(0);
		}
		return null;
	}

	public int getModelStatus(FEMModel femModel) {
		String q = "SELECT p from " + FEMModelModification.class.getName() + " p where p.id = :modelid order by p.pk desc";
		Query query = entityManager.createQuery(q).setParameter("modelid", "" + femModel.getPk());
		List<FEMModelModification> audit = query.getResultList();
		// Return the Action value of the latest audit record
		return audit.get(0).getAction();
	}

	private String getCMSNode(FEMModel model) {
		String q = "SELECT p from " + FEMModelModification.class.getName() + " p where p.id = :modelid and p.action = :act";
		Query query = entityManager.createQuery(q).setParameter("modelid", model.getId()).setParameter("act", FEMModelModification.create);
		List<FEMModelModification> models = query.getResultList();
		return models.get(0).getMetadata();
	}

	public String addModel(File meshXML, JSONObject viewData, String author) throws Exception {

		Vector<String> createdNodes = new Vector<String>();

		MRIFitData mfd = new MRIFitData(meshXML.getAbsolutePath(), viewData);
		mfd.setAuthor(author);
		mfd.setStatusAuthor(author);
		Vector<String> exregionFiles = mfd.getEXRegionFiles();
		Properties urls = new Properties();
		// Load exregion files into CMS
		for (int exctr = 0; exctr < exregionFiles.size(); exctr++) {
			String exr = exregionFiles.elementAt(exctr);
			String cmsXpath = resourceManger.addBytesToCMS(mfd.getStudyUID(), mfd.getSopintanceUID(), "Heart." + exctr + ".exregion", "text/plain", exr.getBytes());
			urls.put("Heart." + exctr, cmsXpath);
			createdNodes.add(cmsXpath);
		}

		try {
			// Discard the author settings from the fitting
			// metadata
			mfd.setAuthor(author);
			mfd.setStatusAuthor(author);
			Properties metaProps = new Properties();
			metaProps.put("MODELJSON", mfd.getJSON(urls));
			metaProps.put("MODELXML", mfd.getMetaData());

			ByteArrayOutputStream bosMP = new ByteArrayOutputStream();
			metaProps.storeToXML(bosMP, "Metadata");

			ByteArrayOutputStream bosEX = new ByteArrayOutputStream();
			urls.storeToXML(bosEX, "FieldML");

			Properties viewUrlsTx = new Properties(); //View data is shared with scan views, so leave it empty, else will be deleted when model is deleted

			ByteArrayOutputStream bosVU = new ByteArrayOutputStream();
			viewUrlsTx.storeToXML(bosVU, "Views");

			// Load other information
			// Compress it
			ByteArrayOutputStream zipOut = new ByteArrayOutputStream();
			GZIPOutputStream zip = new GZIPOutputStream(zipOut);
			zip.write(bosMP.toByteArray());
			zip.close();
			zipOut.close();

			FEMModel model = new FEMModel(mfd.getPatientID(), mfd.getStudyUID(), (String) viewData.get("seriesid"), mfd.getSopintanceUID(), author, mfd.getModelAnnotation(),
					mfd.getModelName(), bosVU.toString(), bosEX.toString(), zipOut.toByteArray());
			log.log(Level.INFO, "Created model " + mfd.getModelName());
			SimpleDateFormat formatter = new SimpleDateFormat();
			model.setAuthor(author);
			model.setAnnotation("Initial non-validated model. Created on " + formatter.format(new java.util.Date(System.currentTimeMillis())));
			addFEMModel(model, meshXML.getCanonicalPath(), author);
			return model.getId();
		} catch (Exception exx) {
			resourceManger.removeCMSNodes(createdNodes);
			throw exx;
		}
	}

	public String addModel(File dicomFile, String patientID, String studyID, String seriesID, String author) throws Exception {

		PACSInstanceprocessor processInstances = new PACSInstanceprocessor(author, resourceManger, patientID, studyID, seriesID, dicomFile.getCanonicalPath());
		Vector<FEMModel> feminst = processInstances.getFEMModels();
		// log.log(Level.INFO,"Number of models post pacs processing "+feminst.size());
		String sopIUID = null;
		for (FEMModel model : feminst) {
			try {
				SimpleDateFormat formatter = new SimpleDateFormat();
				model.setAuthor(author);
				model.setAnnotation("Initial non-validated model. Created on " + formatter.format(new java.util.Date(System.currentTimeMillis())));
				addFEMModel(model, dicomFile.getCanonicalPath(), author);
				sopIUID = model.getId();
				// Do not add model to PACS, right now
				// Do that in update Model

			} catch (Exception instEx) {
				log.log(Level.INFO, "Exception occured while adding FEM Model Instance record with ID " + model.getId() + "\n" + instEx);
				sopIUID = null;

			}
		}
		processInstances.cleanup();
		return sopIUID;
	}

	@TransactionAttribute(TransactionAttributeType.REQUIRES_NEW)
	public boolean removeModel(String instanceID, String statusAuthor) throws Exception {
		FEMModel model = getModel(instanceID);
		if (model != null) {
			removeFEMModel(model, statusAuthor);
			return true;
		} else {
			return false;
		}
	}

	@TransactionAttribute(TransactionAttributeType.REQUIRES_NEW)
	public boolean updateModel(String instanceID, String annotation, String status, String statusAuthor) throws Exception {
		// Check if model record exists in PACS, if not update PACS
		try {
			if (resourceManger.syncModelsWithPACS()) {
				FEMModel model = getModel(instanceID);
				if (model != null) {
					String modelPk = "" + model.getPk();
					String q = "SELECT p from " + FEMModelModification.class.getName() + " p where p.id = :modelid and p.action = :act";
					Query query = entityManager.createQuery(q).setParameter("modelid", modelPk).setParameter("act", FEMModelModification.create);
					List<FEMModelModification> models = query.getResultList();
					String metaData = models.get(0).getMetadata();
					CMSContent dicom = resourceManger.getCMSNodeAt(metaData);
					// Create a temp directory
					File tempDir = new File(resourceManger.getDiskScratchSpace(), "upTemp");
					while (tempDir.exists()) {
						tempDir = new File(resourceManger.getDiskScratchSpace(), "upTemp" + Math.random());
					}
					tempDir.mkdirs();
					File dicomFile = new File(tempDir, "pacs.dcm");
					OutputStream ostream = new FileOutputStream(dicomFile);
					ostream.write(dicom.output.toByteArray());
					ostream.close();
					try {
						// Upload the DICOM file into the pacs
						PACSStudyOperations.dcmSND(dicomFile.getCanonicalPath());
					} catch (Exception dicomEx) {
						log.log(Level.INFO, "Exception occured while uploading model to PACS " + dicomEx + " continuing with ICMA updates");
					}
					// Cleanup
					ResourceConfigurationManager.removeDirectory(tempDir.getCanonicalPath());
				}
			}
		} catch (Exception exx) {
			log.log(Level.INFO, "Exception occured while uploading model to PACS " + exx + " continuing with ICMA updates");
		}

		boolean result = false;
		FEMModel model = getModel(instanceID);
		if (model != null) {
			if (annotation != null)
				model.setAnnotation(annotation);
			if (status != null)
				model.setModelStatus(status);
			else
				model.setModelStatus("Modified");
			log.log(Level.INFO, "Updating model");
			updateModel(model, statusAuthor);
			result = true;
		} else {
			result = false;
		}

		return result;
	}

}
