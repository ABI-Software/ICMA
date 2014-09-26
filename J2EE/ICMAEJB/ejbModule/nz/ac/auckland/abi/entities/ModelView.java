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
package nz.ac.auckland.abi.entities;

import java.io.BufferedInputStream;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.Serializable;
import java.util.Properties;
import java.util.zip.Adler32;
import java.util.zip.CheckedInputStream;
import java.util.zip.GZIPInputStream;

import javax.persistence.Column;
import javax.persistence.Entity;
import javax.persistence.Id;
import javax.persistence.Lob;
import javax.sql.rowset.serial.SerialBlob;

import org.json.simple.JSONObject;
import org.json.simple.parser.JSONParser;

@Entity(name = "ModelView")
/*
 * @Table(name="ModelView")
 * 
 * @SqlResultSetMapping(name="implicit",
 * entities=@EntityResult(entityClass=ModelView.class))
 */
public class ModelView implements Serializable {
	private static final long serialVersionUID = 3L;

	@Id
	@Column(name = "model_pk")
	private long pk;

	@Column(name = "pat_id")
	private String patientId;

	@Column(name = "pat_name")
	private String patientName;

	@Column(name = "pat_sex")
	private String patientGender;

	@Column(name = "pat_birthdate")
	private String patientBirthDate;

	@Column(name = "study_date")
	private String studyDate;

	@Column(name = "model_id")
	private String modelID;
	
	@Column(name = "model_name")
	private String modelName;

	@Column(name = "model_status")
	private String modelStatus;

	@Column(name = "Annotation")
	private String modelAnnotation;

	@Column(name = "metadata")
	@Lob
	private byte[] metaData;

	public long getPk() {
		return pk;
	}

	public String getPatientId() {
		return patientId;
	}

	public String getPatientName() {
		return patientName;
	}

	public String getPatientGender() {
		return patientGender;
	}

	public String getPatientBirthDate() {
		return patientBirthDate;
	}

	public String getStudyDate() {
		return studyDate;
	}

	public String getModelID() {
		return modelID;
	}
	
	public String getModelName() {
		return modelName;
	}

	public String getModelStatus() {
		return modelStatus;
	}

	public String getModelAnnotation() {
		return modelAnnotation;
	}

	public JSONObject getMetaData() throws Exception {
		final int BUFFER = 2048;
		// The blob is the zipped overlay data
		// Create a new input stream as the input stream is closed by downstream
		// processing
		SerialBlob msBlob = new SerialBlob(metaData);

		CheckedInputStream checksum = new CheckedInputStream(
				msBlob.getBinaryStream(), new Adler32());
		GZIPInputStream zis = new GZIPInputStream(new BufferedInputStream(
				checksum));

		int count = -1;
		byte data[] = new byte[BUFFER];

		ByteArrayOutputStream fos = new ByteArrayOutputStream();
		try {
			while ((count = zis.read(data, 0, BUFFER)) != -1) {
				fos.write(data, 0, count);
			}
		} catch (Exception exx) {
			exx.printStackTrace();
		}
		fos.close();
		zis.close();
		Properties bundle = new Properties();
		bundle.loadFromXML(new ByteArrayInputStream(fos.toByteArray()));
		String modelJSON = bundle.getProperty("MODELJSON");
		JSONParser parser = new JSONParser();
		return (JSONObject) parser.parse(modelJSON);
	}
}
