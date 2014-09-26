package nz.ac.auckland.abi.dcm4chee;
/*
 * Copyright (C) 2006 Thomas Hacklaender
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */


import java.io.*;
import java.text.ParseException;

import org.dcm4che.data.*;
import org.dcm4che.dict.*;



/**
 * Implementation of an adapter class for the StorageService. It implements
 * the StorageServiceEventListener to receive StorageServiceEvent when
 * the server receives an DICOM object. The adapter class stores the received object
 * into the local filesystem.
 * <p>Details of how to run the server is given in another configuration property file.
 * A sample may be found at "resources/StorageService.cfg".
 * <p>The details how to store the object is given in a configuration property file.
 * A sample may be found at "resources/SaveFilesystem.cfg".
 *
 * @author Thomas Hacklaender
 * @version 2006-06-21
 */
public class StorageServiceAdapter implements StorageServiceEventListener {
    
    
    /** The configuration properties of the DcmRcv working class. */
    private DCMConfigProperties cfgStorageSC = null;
    
    /** The configuration properties for saving the DICOM objects to the filesystem. */
    private DCMConfigProperties cfgSaveFilesystem = null;
    
    /** The server. */
    private StorageService storageSC = null;
    
    
    //>>>> Local fields defined in configuration properties for sving to filesystem >>>>
    
    /** Directory to save the Dataset */
    private File directory = DCMConfigProperties.uriToFile("./");
    
    /** Extension of the file to save */
    private String extension = "dcm";
    
    /**
     * Name of file to save. If the String starts with the character '$' the filename
     * is set from the DICOM element named in the remaining part of the string. */
    private String filename = "DICOM_object";
    
    /** If true, write files in the subdirectory "$PatientName_$PatientBirthDate/<directory>/" */
    private boolean separate_patients = false;
    
    /** Transfersyntax of the saved file. One of the strings  ImplicitVRLittleEndian,
     * ExplicitVRLittleEndian, ExplicitVRBigEndian. The string may start with
     * the praefix-character '$' */
    private String transfersyntax = "ImplicitVRLittleEndian";
    
    /** Write files in the subdirectory "[$PatientName_$PatientBirthDate/]<directory>/<use_subdirectory>/". */
    private String use_subdirectory = "";
    
    /**
     * If true:
     * Construct the the file ID of the file to save from the following file ID
     * components:
     * <p>1. First letter of family name followed by first letter of given name
     * followed by date of birth, 6 character. Example: HT570522
     * <p>2. Study date, 6 character. Example: 043012
     * <p>3. Modality, 2 character followed by study time, 4 character. Example: MR1531
     * <p>4. Study ID. Example: 4711
     * <p>5. Series number. Example: 3
     * <p>6. Instance number. Example: 54
     * <p><p>If false:
     * <p>Construct the the name of the file to save from directory, filename and extension. */
    private boolean write_dir_tree = true;
    
    /** Include a File Meta Information block to the saved file. */
    private boolean write_fmi = true;
    
    
    //>>>> Constructor >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    
    /**
     * Creates a new instance of StorageServiceAdapter.
     *
     * @throws ParseException in case of a parsing error of the configuration properties.
     */
    public StorageServiceAdapter(DCMConfigProperties cfgStorageSC, DCMConfigProperties cfgSaveFilesystem) throws ParseException {
        // Store configuration properties local
        this.cfgStorageSC = cfgStorageSC;
        this.cfgSaveFilesystem = cfgSaveFilesystem;
        
        // Initialize local fields from the configuration properties for saving to filesystem
        initFieldsFromCfgSaveFilesystem();
        
        // Initialize the working class with the actual configuration properties.
        storageSC = new StorageService(cfgStorageSC);
        
        // Register to server as a listener for StorageServiceClassEvents
        storageSC.addStorageServiceClassEventListener(this);
    }
    
    
    /**
     * Return the working class StorageService. This can be usedto add additional 
     * listeners for ServiceClassEvent's.
     *
     * @return the working class StorageService.
     */
    public StorageService getStorageService() {
        return storageSC;
    }
    
    /**
     * Starts the server.
     *
     * @throws IOException 
     */
    public void start() throws IOException {
        // Start the instance of the working class
        storageSC.start();
    }
    
    
    /**
     * Stops the server.
     */
    public void stop() {
        storageSC.stop();
    }
    
    
    /**
     * Handles StorageServiceClassEvents send from the server.
     *
     * @param e the StorageServiceEvent send from the server.
     */
    public void handleStorageServiceEvent(StorageServiceEvent e) {
        File     parent = null;
        File     dcmFile = null;
        String   value;
        Dataset  ds = e.getDataset();
        
        // log.info(">>>> Received: " + ds.getString(Tags.Modality) + " : "+ ds.getString(Tags.SOPClassUID));
        
        // If no Dataset: nothing to do
        if (ds == null) {
        	System.out.println("No Dataset received.");
            return;
        }
        
        // Filename may be derived from received DICOM object
        value = cfgSaveFilesystem.getProperty("file-name");
        
        if (value != null) {
            filename = getTagStringOrValue(value, ds);
            if (filename == null) {
            	System.out.println("Can't find element" + value.substring(1) +  " in received Dataset.");
                return;
            }
        }
        
        // Set working directory
        parent = directory;
        if (separate_patients) {
            parent = addPatientDirectory(parent, ds);
        }
        
        // Dataset toFile
        if ((dcmFile = createFilePathToSave(parent, ds)) == null) {
        	System.out.println("Could not craete File from Dataset.");
            return;
        }
        
        // Exportieren file containing DICOM object
        if (! saveDataset(ds, dcmFile)) {
        	System.out.println("Could not save File.");
            return;
        }
    }
    
    
    //>>>> Methods for storing a received DICOM object to the filesystem >>>>>>
    
    /**
     * Inits local fieldfrom the configuration properties.
     */
    public void initFieldsFromCfgSaveFilesystem() {
        String value;
        File   newDirectory;
        
        value = cfgSaveFilesystem.getProperty("directory");
        if (value != null) {
            if ((newDirectory = DCMConfigProperties.uriToFile(value)) != null) {
                directory = newDirectory;
            }
        }
        
        value = cfgSaveFilesystem.getProperty("file-extension");
        if (value != null) {
            extension = value;
        }
        
        value = cfgSaveFilesystem.getProperty("separate-patients");
        if (value != null) {
            if (value.toLowerCase().charAt(0) == 't') {
                separate_patients = true;
            } else {
                separate_patients = false;
            }
        }
        
        value = cfgSaveFilesystem.getProperty("transfersyntax");
        if (value != null) {
            if (value.charAt(0) == '$') {
                transfersyntax = value.substring(1);
            } else {
                transfersyntax = value;
            }
        }
        
        value = cfgSaveFilesystem.getProperty("use-subdirectory");
        if (value != null) {
            use_subdirectory = value;
        }
        
        value = cfgSaveFilesystem.getProperty("write-dir-tree");
        if (value != null) {
            if (value.toLowerCase().charAt(0) == 't') {
                write_dir_tree = true;
            } else {
                write_dir_tree = false;
            }
        }
        
        value = cfgSaveFilesystem.getProperty("write-fmi");
        if (value != null) {
            if (value.toLowerCase().charAt(0) == 't') {
                write_fmi = true;
            } else {
                write_fmi = false;
            }
        }
    }
    
    
    /**
     * Create the file path of the file to save in the directory parent
     * depending of the property use_subdirectory.
     * @param parent the working directory.
     * @param ds the dataset to analyse.
     * @return the file.
     */
    protected File createFilePathToSave(File parent, Dataset ds) {
        File path;
        
        if (use_subdirectory != null) {
            if (! use_subdirectory.equals("")) {
                parent = new File(parent, use_subdirectory);
            }
        }
        
        if (write_dir_tree) {
            path = new File(parent, datasetToTreeFileID(ds));
        } else {
            // Direkt in das angegebene Verzeichnis speichern
            path = new File(parent, filename + "." + extension);
        }
        
        return path;
    }
    
    
    /**
     * Saves the Dataset to the local filesystem.
     *
     * @param ds the Dataset to save.
     * @param f the File to which the Dataset should be save.
     */
    protected boolean saveDataset(Dataset ds, File f) {
        FileOutputStream fos = null;
        DcmEncodeParam param = null;
        FileMetaInfo fmi;
        
        try {
            if (f.exists()) {
                // File besteht bereits. Erst loeschen, dann neu erstellen
                f.delete();
                f.createNewFile();
            } else {
                if (!f.getParentFile().exists()) {
                    // Directory Pfad existiert noch nicht
                    f.getParentFile().mkdirs();
                }
            }
            
            fos = new FileOutputStream(f);
            param = DcmEncodeParam.valueOf(UIDs.forName(transfersyntax));
            
            // File mit/ohne File Meta Information Block schreiben
            if (write_fmi) {
                fmi = DcmObjectFactory.getInstance().newFileMetaInfo(ds,
                    UIDs.forName(transfersyntax));
                ds.setFileMetaInfo(fmi);
                
                // Die Methode Dataset#writeFile schreibt -sofern != null- einen
                // File Meta Information Block und ruft dann Dataset#writeDataset
                // mit den selben Parametern auf.
                ds.writeFile(fos, param);
                
                // File Meta Information Block wieder loeschen
                ds.setFileMetaInfo(null);
            } else {
                ds.writeDataset(fos, param);
            }
            
            // Finished without errors
            return true;
            
        } catch (Exception e) {
        	System.out.println("Can't save file - Exception: " + e.getMessage());
            
            return false;
        } finally {
            try {
                fos.close();
            } catch (Exception ignore) {}
        }
    }
    
    
    /**
     * Builds a file ID from the given Dataset. Components are separated by
     * the '/' caharcter:<br>
     * 1. componet: First letter of family name followed by first letter of given
     * name followed by date of birth, 6 character. Example: TH570522<br>
     * 2. componet: Study date, 6 character. Example: 043012<br>
     * 3. componet: Modality, 2 character followed by study time, 4 character.
     * Example: MR1531<br>
     * 4. componet: Study ID. Example: 4711<br>
     * 5. componet: Series number. Example: 3<br>
     * 6. componet: Instance number. Example: 54<br>
     * All components are compatible to DICOM part 12, e.g. they have a maximum
     * of 8 characters and consists of upper case caracters, numbers or '_'.
     * @param ds the dataset to analyse.
     * @return the file ID as a absolute pathString.
     */
    protected String datasetToTreeFileID(Dataset ds) {
        String[] nameArray;
        String pathString;
        
        nameArray = datasetToNameArray(ds);
        
        pathString = nameArray[0];
        for (int i = 1; i < nameArray.length; i++) {
            pathString += "/" + nameArray[i];
        }
        
        return pathString;
    }
    
    
    /**
     * Builds an array of strings from the given Dataset:<br>
     * [0]: First letter of famely name followed by first letter of given
     * name followed by date of birth, 6 character. Example: TH570522<br>
     * [1]: Study date, 6 character. Example: 043012<br>
     * [2]: Modality, 2 character followed by study time, 4 character.
     * Example: MR1531<br>
     * [3]: Study ID. Example: 4711<br>
     * [4]: Series number. Example: 3<br>
     * [5]: Instance number. Example: 54<br>
     * All components are compatible to DICOM part 12, e.g. they have a maximum
     * of 8 characters and consists of upper case caracters, numbers or '_'.
     * @param ds the dataset to analyse.
     * @return the array of strings.
     */
    protected String[] datasetToNameArray(Dataset ds) {
        String  s;
        String  pn;
        int i;
        String[] nameArray = new String[6];
        
        String patientName = ds.getString(Tags.PatientName, "X^X");
        String patientBirthDate = ds.getString(Tags.PatientBirthDate, "19990101");
        String studyDate = ds.getString(Tags.StudyDate, "20050101");
        String studyTime = ds.getString(Tags.StudyTime, "123456.789");
        String modality = ds.getString(Tags.Modality, "OT");
        String studyID = ds.getString(Tags.StudyID, "0");
        String seriesNumber = ds.getString(Tags.SeriesNumber, "0");
        String instanceNumber = ds.getString(Tags.InstanceNumber, "0");
        
        // Patientenname mindestens 2 Buchstaben lang machen
        pn = patientName + "XX";
        
        s = pn.substring(0, 1);
        i = pn.indexOf('^');
        if (i != -1) {
            // Vorname vorhanden
            s += pn.substring(i + 1, i + 2);
        } else {
            // Kein Vorname vorhanden, die erstenb zwei Buchstaben des Nachnamens verwenden
            s = pn.substring(0, 2);
        }
        
        s += patientBirthDate.substring(2);
        nameArray[0] = stringToFileIDComponentString(s);
        
        s = studyDate.substring(2);
        nameArray[1] = stringToFileIDComponentString(s);
        
        s = modality.substring(0, 2);
        s += studyTime.substring(0, 4);
        nameArray[2] = stringToFileIDComponentString(s);
        
        nameArray[3] = stringToFileIDComponentString(studyID);
        
        nameArray[4] = stringToFileIDComponentString(seriesNumber);
        
        nameArray[5] = stringToFileIDComponentString(instanceNumber);
        
        return nameArray;
    }
    
    
    /**
     * Checks, if the given file ID is compatible to DICOM Part 12:
     * It may only contain the characters 'A'..'Z','0'..'9' and '_'. The
     * maximal length is 8 character.
     * This method truncates the file ID after 8 characters, converts lower case
     * to upper case characters and replaces all other not valid characters with
     * the '_' character.
     * @param s the file ID to check.
     * @return the converted file ID.
     */
    protected String stringToFileIDComponentString(String s) {
        if ((s == null) || (s.length() == 0)) {
            return "__NULL__";
        }
        
        char[] in = s.toUpperCase().toCharArray();
        char[] out = new char[Math.min(8, in.length)];
        
        for (int i = 0; i < out.length; ++i) {
            out[i] = (((in[i] >= '0') && (in[i] <= '9')) ||
                ((in[i] >= 'A') && (in[i] <= 'Z'))) ? in[i] : '_';
        }
        
        return new String(out);
    }
    
    
    /**
     * Adds a subdirectory to a given path. The name of the subdirectory is
     * composed of the patients name followed by '_' followed by the birthdate.
     * @param parent the parent directory.
     * @param ds the dataset to analyse.
     * @return the composed directory.
     */
    protected File addPatientDirectory(File parent, Dataset ds) {
        String patientName = ds.getString(Tags.PatientName, "X^X");
        String patientBirthDate = ds.getString(Tags.PatientBirthDate, "19990101");
        return new File(parent, patientName + "_" + patientBirthDate);
    }
    
    
    /**
     * Returns the given value or, if it is a "Tag String", the contents of the tag.
     * Tag Strings are value starts wich start with the '$' character it is a
     * reference by name or with '@' if it is a reference by group/element.
     * @param value the property value.
     * @param ds the dataset to analyse.
     * @return the value or content of tag. null, if unknown tag.
     */
    protected String getTagStringOrValue(String value, Dataset ds) {
        int tag = getTagFromPropertyString(value);
        
        // Falls kein Tag String -> value zurueckgeben
        if (tag <= 0) {
            return value;
        }
        
        // Ist ein Tag -> dessen Inhalt zurueckgeben
        try {
            return ds.getString(tag);
        } catch (Exception e) {
            return null;
        }
        
    }
    
    
    /**
     * Returns the tag described by a given value. If the value starts with the
     * '$' character it is a reference by name, if it starts with '@' it is a
     * reference by group/element.
     * @param value the property value.
     * @return the tag. 0, if value does not begin with '$' or '@'. -1, if unknown tag.
     */
    protected int getTagFromPropertyString(String value) {
        final int       NO_TAG = 0;
        final int       UNKNOWN_TAG = -1;
        int             tag;
        TagDictionary   td = DictionaryFactory.getInstance().getDefaultTagDictionary();
        
        if (value.length() == 0) return NO_TAG;
        
        if (value.charAt(0) == '$') {
            try {
                tag = Tags.forName(value.substring(1));
                return tag;
            } catch (IllegalArgumentException e) {
                return UNKNOWN_TAG;
            }
        }
        
        if (value.charAt(0) == '@') {
            // Reference by group/element
            try {
                tag = Integer.parseInt(value.substring(1), 16);
                TagDictionary.Entry e = td.lookup(tag);
                if (e != null) {
                    return tag;
                } else {
                    return UNKNOWN_TAG;
                }
            } catch (NumberFormatException e) {
                return UNKNOWN_TAG;
            }
        }
        
        return NO_TAG;
    }
    
}