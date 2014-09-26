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

import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.io.Serializable;
import java.io.StringWriter;
import java.util.Enumeration;
import java.util.Properties;

import javax.persistence.Basic;
import javax.persistence.Column;
import javax.persistence.Entity;
import javax.persistence.FetchType;
import javax.persistence.Id;
import javax.persistence.Lob;
import javax.sql.rowset.serial.SerialBlob;

import nz.ac.auckland.abi.helper.DICOMMetaDataFilter;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;

@Entity(name = "USStudyInstances")
public class USStudyInstances implements Serializable{
	private static final long serialVersionUID = 5L;
	
	@Id
    @Column(name = "study_id")
    private String studyID;
	
    @Id
	@Column(name = "instance_id")
    private String instanceID;
    
	@Column(name = "series_id")
    private String seriesID;  

	@Column(name = "size")
    private double size;
    
	@Column(name = "weight")
    private double weight;

    @Column(name = "head_shot_uri")
    private String headShotUri;

    @Column(name = "mp4_uri")
    private String mp4Uri;
 	
    @Column(name = "webm_uri")
    private String webmUri;
    
    @Column(name = "ogv_uri")
    private String ogvUri;

    @Column(name = "movie_metadata") //Number of Frames#Frame Rate#Movie Time
    private String movieMetaData;
    
    @Column(name = "metadata")
    @Lob @Basic(fetch=FetchType.LAZY)
    private byte[] metadata;
    
    
	
	public USStudyInstances(){
		super();
	}
	
	public USStudyInstances(String sid, String iid, String srid, String headuri, String muri, String wuri, String ouri, String movieMeta, String meta) throws Exception{
		studyID = sid;
		instanceID = iid;
		seriesID = srid;
		headShotUri = headuri;
		mp4Uri = muri;
		webmUri = wuri;
		ogvUri = ouri;
		movieMetaData = movieMeta;
		metadata = meta.getBytes();
	}
    
	@Override
 	public String toString(){
 		return "Instance : "+studyID+"\t"+instanceID+"\t"+metadata+"\t"+headShotUri;
 	}

	@Override
	public int hashCode() {
		final int prime = 31;
		int result = 1;
		result = prime * result
				+ ((instanceID == null) ? 0 : instanceID.hashCode());
		result = prime * result + ((studyID == null) ? 0 : studyID.hashCode());
		return result;
	}

	@Override
	public boolean equals(Object obj) {
		if (this == obj)
			return true;
		if (obj == null)
			return false;
		if (getClass() != obj.getClass())
			return false;
		USStudyInstances other = (USStudyInstances) obj;
		if (instanceID == null) {
			if (other.instanceID != null)
				return false;
		} else if (!instanceID.equals(other.instanceID))
			return false;
		if (studyID == null) {
			if (other.studyID != null)
				return false;
		} else if (!studyID.equals(other.studyID))
			return false;
		return true;
	}

	public String getStudyID() {
		return studyID;
	}

	public void setStudyID(String studyID) {
		this.studyID = studyID;
	}

	public String getInstanceID() {
		return instanceID;
	}

	public void setInstanceID(String instanceID) {
		this.instanceID = instanceID;
	}

	public String getHeadShotUri() {
		return headShotUri;
	}

	public void setHeadShotUri(String headShotUri) {
		this.headShotUri = headShotUri;
	}

	public String getMp4Uri() {
		return mp4Uri;
	}

	public void setMp4Uri(String mp4Uri) {
		this.mp4Uri = mp4Uri;
	}

	public String getWebmUri() {
		return webmUri;
	}

	public void setWebmUri(String webmUri) {
		this.webmUri = webmUri;
	}

	public String getOgvUri() {
		return ogvUri;
	}

	public void setOgvUri(String ogvUri) {
		this.ogvUri = ogvUri;
	}

	public byte[] getMetadata() throws Exception{
		return metadata;
	}

	public void setMetadata(String metadata) throws Exception{
		this.metadata = metadata.getBytes();
	}
 	
	public String getMovieMetaData() {
		return movieMetaData;
	}

	public void setMovieMetaData(String movieMetaData) {
		this.movieMetaData = movieMetaData;
	}

	public double getSize() {
		return size;
	}

	public void setSize(double size) {
		this.size = size;
	}

	public double getWeight() {
		return weight;
	}

	public void setWeight(double weight) {
		this.weight = weight;
	}

	public String getJSON() throws Exception {
		JSONObject study = new JSONObject();
		study.put("imageid", instanceID);							
		Properties myproperties = new Properties();
		//Access metadata
		SerialBlob msBlob = new SerialBlob(metadata);
		StringWriter writer = new StringWriter();
		InputStream is = msBlob.getBinaryStream();
		char[] buffer = new char[1024];
        try {
            Reader reader = new BufferedReader(
                    new InputStreamReader(is, "UTF-8"));
            int n;
            while ((n = reader.read(buffer)) != -1) {
                writer.write(buffer, 0, n);
            }
        } finally {
            is.close();
        }
		String theString = writer.toString();
		myproperties.loadFromXML(new ByteArrayInputStream(theString.getBytes()));
		//myproperties.list(System.out);
/*		study.put("beginningRwave", myproperties.getProperty("ICMACYCLESTARTFRAME"));
		study.put("followingRwave", myproperties.getProperty("ICMACYCLEENDFRAME"));
		study.put("rwaveOffset", myproperties.getProperty("ICMACYCLESTARTFRAME"));
*/
		
		//String brw = myproperties.getProperty("ICMACYCLESTARTFRAME");
		String erw = myproperties.getProperty("ICMACYCLEENDFRAME");
		//TODO change default values to ICMA values
		study.put("lastFrame", erw);
		study.put("beginningRwave", myproperties.getProperty("RWAVESTART","1"));
		study.put("followingRwave", myproperties.getProperty("RWAVEEND","54"));
		study.put("rwaveOffset", myproperties.getProperty("RWAVESTART","1"));
		study.put("beatsPerMin", myproperties.getProperty("HeartRate"));
		study.put("image", headShotUri);
		
		String[] movieMetaDatat = movieMetaData.split("#");
		study.put("numberOfMovieFrames", movieMetaDatat[0]);
		study.put("movieFrameRate", movieMetaDatat[1]);
		study.put("movieTime", movieMetaDatat[2]);
		
		JSONObject video = new JSONObject();
		video.put("mp4", mp4Uri);
		video.put("ogg", ogvUri);
		video.put("webm", webmUri);
		study.put("video", video);
		study.put("zoom", "0"); //This should be appropriately updated
		
		JSONArray props = new JSONArray();
		Enumeration<Object> pKeys = myproperties.keys();
		while(pKeys.hasMoreElements()){
			String key = (String) pKeys.nextElement();
			String value = myproperties.getProperty(key);
			//if(!(key.startsWith("Study")||key.startsWith("Series")||key.endsWith("UID")||key.startsWith("Patient"))){
			if(DICOMMetaDataFilter.accept(key)){
				JSONObject pr = new JSONObject();
    			//Convert property names to Sentence case from camelCase
    			if(!key.startsWith("Comments")){
					pr.put(getSentenceCase(key), value);
					props.add(pr);
    			}else{
					pr.put("Comments", value);
					props.add(pr);    				
    			}
			}
			if(key.equalsIgnoreCase("PatientSize")||key.equalsIgnoreCase("Patient'sSize")){
				if(size==0.0){
					size = Double.parseDouble(value);
				}
			}else if(key.equalsIgnoreCase("PatientWeight")||key.equalsIgnoreCase("Patient'sWeight")){
				if(weight==0.0){
					weight = Double.parseDouble(value);
				}
			}
		}
		
		study.put("properties", props);
		
		return study.toJSONString();
	}
	
	public String getSentenceCase(String word){
		StringBuffer buf = new StringBuffer("");
		try{
			for(int i=0;i<word.length();i++){
				if(Character.isUpperCase(word.charAt(i))){
					buf.append(" ");
				}
				buf.append(word.charAt(i));
			}
			return buf.toString();
		}catch(Exception exx){
			return word;
		}
	}
}

