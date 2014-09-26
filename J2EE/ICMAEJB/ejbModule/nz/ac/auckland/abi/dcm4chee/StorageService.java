package nz.ac.auckland.abi.dcm4chee;

/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is part of dcm4che, an implementation of DICOM(TM) in
 * Java(TM), hosted at http://sourceforge.net/projects/dcm4che.
 *
 * The Initial Developer of the Original Code is
 * TIANI Medgraph AG.
 * Portions created by the Initial Developer are Copyright (C) 2002-2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * Gunter Zeilinger <gunter.zeilinger@tiani.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

import java.io.IOException;
import java.net.URL;
import java.text.ParseException;
import java.util.Enumeration;
import java.util.Vector;

import org.dcm4che.data.Command;
import org.dcm4che.data.FileMetaInfo;
import org.dcm4che.dict.Tags;
import org.dcm4che.dict.UIDs;
import org.dcm4che.net.AcceptorPolicy;
import org.dcm4che.net.ActiveAssociation;
import org.dcm4che.net.AssociationFactory;
import org.dcm4che.net.DcmServiceBase;
import org.dcm4che.net.DcmServiceRegistry;
import org.dcm4che.net.Dimse;
import org.dcm4che.server.DcmHandler;
import org.dcm4che.server.Server;
import org.dcm4che.server.ServerFactory;
import org.dcm4che.util.DcmProtocol;
import org.dcm4che.util.SSLContextAdapter;


/**
 * Implementation of a DICOM server.
 * <p>
 * <p> Usage:
 * <p> 1. Create a new instance of this class.
 * <p> 2. Register one or more listeners for StorageServiceEvents. These events are fired for each received DICOM object.
 * <p> 3. Use the start method to start the server.
 * <p> 4. Use the stop method to stop the server.
 * <p>
 * <p>Details of how to run the server is given in another configuration property file.
 * A sample may be found at "./resources/StorageService.cfg".
 * <p>
 * <p>Based on dcm4che 1.4.0 sample: DcmRcv.java revision date 2006-04-06
 * <p>
 * <p>See: PS 3.4 - Annex B STORAGE SERVICE CLASS
 * 
 * @author Thomas Hacklaender
 * @version 2006-07-25
 */
public class StorageService extends DcmServiceBase {
      
    /** The DEBUG flag is set, if the logging level of this class is Debug */
    final static boolean DEBUG = false;
    
    
    //>>>> Factorys >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    private final static ServerFactory srvFact = ServerFactory.getInstance();
    private final static AssociationFactory fact = AssociationFactory.getInstance();
    //private final static DcmParserFactory pFact = DcmParserFactory.getInstance();
    //private final static DcmObjectFactory oFact = DcmObjectFactory.getInstance();
    
    
    //>>>> Fields >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    
    /** TLS context. Set by method initTLS */
    private SSLContextAdapter tls = null;
    
    /** Type of encryption used for connections [default=DICOM]. */
    private DcmProtocol protocol = DcmProtocol.DICOM;
    
    /** Acceptor policy of the server. */
    private AcceptorPolicy policy = fact.newAcceptorPolicy();
    
    /** ??? */
    private DcmServiceRegistry services = fact.newDcmServiceRegistry();
    
    /** The server handler instance.*/
    private DcmHandler handler = srvFact.newDcmHandler(policy, services);
    
    /** The server instance.*/
    private Server server = srvFact.newServer(handler);
    
    /** Additional delay of response [in sec] (useful for testing async mode). */
    private long rspDelay = 0L;
    
    /** Status code in C-STORE-RSP in decimal (#####) or hex (####H) [default=0].
     * Useful for testing modality behavior in case of status != 0 */
    private int rspStatus = 0;
    
    /** A Vector of listeners registered to receive StorageServiceClassEvents. */
    private Vector eventListeners = new Vector();
    
    
    //>>>> Constructor >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    
    /**
     *  Constructor for the StorageService object. Initializes everything.
     * 
     * @param cfg the configuration properties for this class.
     * @throws ParseException
     */
    public StorageService(DCMConfigProperties cfg) throws ParseException {
        // Additional delay of response [in sec] (useful for testing async mode).
        rspDelay = Integer.parseInt(cfg.getProperty("rsp-delay", "0")) * 1000L;
        
        // Status code in C-STORE-RSP in decimal (#####) or hex (####H) [default=0]. Useful for testing modality behavior in case of status != 0
        String decOrHex = cfg.getProperty("rsp-status", "0");
        if (decOrHex.endsWith("H")) {
            rspStatus = Integer.parseInt(decOrHex.substring(0, decOrHex.length()-1), 16);
        } else {
            rspStatus = Integer.parseInt(decOrHex);
        }
        
        initServer(cfg);
        initTLS(cfg);
        initPolicy(cfg);
    }
    
    
    /**
     * Initializes server related parameters.
     *
     * @param cfg the configuration properties for this class.
     */
    private void initServer(DCMConfigProperties cfg) {
        //>>>> Setup parameters of the server itself
        
        // Port number of the server [default=104]
        server.setPort(Integer.parseInt(cfg.getProperty("port", "104")));
        // Maximum number of simultaneous clients [default=10].
        server.setMaxClients(Integer.parseInt(cfg.getProperty("max-clients", "10")));
        
        //>>>> Setup parameters of the server handler
        
        // Time-out waiting [in msec] for A-ASSOCIATE-RQ after TCP connect. 0 is interpreted as an infinite timeout [default=5000].
        handler.setRqTimeout(Integer.parseInt(cfg.getProperty("rq-timeout", "5000")));
        // Time-out waiting [in msec] for DIMSE on open association. 0 is interpreted as an infinite timeout [default=0].
        handler.setDimseTimeout(Integer.parseInt(cfg.getProperty("dimse-timeout", "0")));
        // Time delay [in msec] for socket close after sending A-RELEASE-RP or A-ABORT [default=500].
        handler.setSoCloseDelay(Integer.parseInt(cfg.getProperty("so-close-delay", "500")));
        // Activates packing of command PDV (Presentation Data Value) + (first) data PDV into one P-DATA-TF PDU (Protocol Data Unit)
        handler.setPackPDVs("true".equalsIgnoreCase(cfg.getProperty("pack-pdvs", "false")));
    }
    
    
    /**
     * Initializes TLS (Transport Layer Security, predecessor of SSL, Secure 
     * Sockets Layer) connection related parameters. TLS expects RSA (Ron Rivest, 
     * Adi Shamir and Len Adleman) encoded keys and certificates.
     *
     * <p>Keys and certificates may be stored in PKCS #12 (Public Key 
     * Cryptography Standards) or JKS (Java Keystore) containers.
     *
     * <p>TSL is used to encrypt data during transmission, which is accomplished 
     * when the connection between the two partners A (normally the client) and B 
     * (normally the server) is established. If A asks B to send TSL encrypted 
     * data, both partners exchange some handshake information. In a first step 
     * B tries to authenticate itself against A (server authentication, optional 
     * in TSL but implemented in this way in dcm4che). For that B presents its 
     * public key for A to accept or deny. If the authentication is accepted, 
     * A tries to authenticate itself against B (client authentication, optional 
     * in TSL but implemented in this way in dcm4che).If B accepts the 
     * authentication A and B agree on a hash (symmetric key, which is 
     * independent of the private/public keys used for authentication) for the 
     * duration of their conversation, which is used to encrypt the data.
     *
     * <p>To be able to establish a TSL connection B needs a private/public key 
     * pair, which identifies B unambiguously. For that the private key is 
     * generated first; than the root-public key is derived from that private 
     * key. To bind the root-public key with the identity of B a Certificate 
     * Authority (CA) is used: The root-public key is send to CA, which returns 
     * a certified-public key, which includes information about the CA as well 
     * as the root-public key. Optionally this process can be repeated several 
     * times so that the certified-public key contains a chain of certificates 
     * of different CA's.
     *
     * <p>The certified-public key of B is presented to A during server 
     * authentication. Partner A should accept this key, if it can match the 
     * last certificate in the chain with a root-certificate found in its local 
     * list of root-certificates of CA's. That means, that A does not check the 
     * identity of B, but "trusts" B because it was certified by an authority. 
     * The same happens for client authentication. Handling of authentication 
     * of the identity of the communication partner is subject of PS 3.15 - 
     * Security and System Management Profiles.
     *
     * <p>In the configuration files of this method the certified-public key is 
     * stored in the property "tls-key" and the root-certificates of the known 
     * CA's in "tls-cacerts".
     *
     * <p>Usually the certified-public keys of A and B are different, but they 
     * also may be the same. In this case the root-certificates are also the same.
     *
     * <p>It is possible to establish a TLS connection without using a CA: In 
     * this case both partners creates a individual container holding their 
     * private and root-public key. These containers could be used for certifying 
     * also, because the length of the certifying chain is one and therefore the 
     * root-public key is also the last certified-public key. Therefore the 
     * root-public key works in this scenario also as the root-certificate of 
     * the "certifying authoroty".
     *
     * <p>If no keystores are specified in the configuration properties, the 
     * not-certified default-keystore "resources/identityJava.jks" is used for 
     * "tls-key" and "tls-cacerts" when establishing the connection.
     *
     * @param cfg the configuration properties for this class.
     * @throws ParseException
     */
    private void initTLS(DCMConfigProperties cfg) throws ParseException {
        char[]  keystorepasswd;
        char[]  keypasswd;
        char[]  cacertspasswd;
        URL     keyURL;
        URL     cacertURL;
        String  value;

        try {
            
            // Get the requested protocol from the configuration properties
            this.protocol = DcmProtocol.valueOf(cfg.getProperty("protocol", "dicom"));
            
            // If no TLS encryption is requested, nothing to do
            if (!protocol.isTLS()) {
                return;
            }
            
            // Get a new TLS context
            tls = SSLContextAdapter.getInstance();
            
            
            //>>>> Managing the keystore file containing the privat key and
            //>>>> certified-public key to establish the communication
            
            // Password of the keystore [default: secret]
            keystorepasswd = cfg.getProperty("tls-keystore-passwd", "secret").toCharArray();
            
            // Password of the private key [default: secret]
            keypasswd = cfg.getProperty("tls-key-passwd", "secret").toCharArray();
            
            // URL of the file containing the default-keystore
            keyURL = DCMAccessManager.class.getResource("resources/identityJava.jks");
            
            // If available, replace URL with the one specified in the configuration file
            if ((value = cfg.getProperty("tls-key")) != null) {
                try {
                    // Property specified, try to set to specified value
                    keyURL = DCMConfigProperties.fileRefToURL(DCMAccessManager.class.getResource(""), value);
                } catch (Exception e) {
                	System.out.println("Wrong value for tls-key: " + value + ". tls-key was set to default value.");
                }
            }
            
            // log.info("Key URL: " + keyURL.toString());
            
            // Sets the key attribute of the SSLContextAdapter object
            // API doc: SSLContextAdapter.loadKeyStore(java.net.URL url, char[] password)
            // API doc: SSLContextAdapter.setKey(java.security.KeyStore key, char[] password)
            tls.setKey(tls.loadKeyStore(keyURL, keystorepasswd), keypasswd);
            
            //>>>> Managing the keystore containing the root-certificates of the Ceritify Authorities
            //>>>> used for signing the public key
            
            // Password of the keystore [default: secret]
            cacertspasswd = cfg.getProperty("tls-cacerts-passwd", "secret").toCharArray();
            
            // URL of the file containing the default-keystore
            cacertURL = DCMAccessManager.class.getResource("resources/identityJava.jks");
            
            // If available, replace URL with the one specified in the configuration file
            if ((value = cfg.getProperty("tls-cacerts")) != null) {
                try {
                    // Property specified, try to set to specified value
                    cacertURL = DCMConfigProperties.fileRefToURL(DCMAccessManager.class.getResource(""), value);
                } catch (Exception e) {
                	System.out.println("Wrong value for tls-cacerts: " + value + ". tls-cacerts was set to default value.");
                }
            }
            
            // log.info("Root-certificate of CA URL: " + cacertURL.toString());
            
            // Sets the trust attribute of the SSLContextAdapter object
            // API doc: SSLContextAdapter.loadKeyStore(java.net.URL url, char[] password)
            // API doc: SSLContextAdapter.setTrust(java.security.KeyStore cacerts)
            tls.setTrust(tls.loadKeyStore(cacertURL, cacertspasswd));
            
            // Get ciphers for selected protocol
            this.server.setServerSocketFactory(tls.getServerSocketFactory(protocol.getCipherSuites()));
            
        } catch (Exception ex) {
            throw new ParseException("Could not initialize TLS configuration.", 0);
        }
    }
    
    
    /**
     * Initializes acceptor policy related parameters.
     *
     * @param cfg the configuration properties for this class.
     */
    private void initPolicy(DCMConfigProperties cfg) {
        
        // Own AET (Application Entity Title).
        // Default is null, that means no AET specified.
        // The provided property value should be a comma or space separated list of individual AETs.
        // The string "<any>" is interpreted also as null.
        policy.setCalledAETs(cfg.tokenize(cfg.getProperty("called-aets", null, "<any>", null)));
        
        // AETs  (Application Entity Titles) of the storage service users.
        // Default is null, that means association of any AET is accepted.
        // The provided property value should be a comma or space separated list of individual AETs.
        // The string "<any>" is interpreted also as null.
        policy.setCallingAETs(cfg.tokenize(cfg.getProperty("calling-aets", null, "<any>", null)));
        
        // Maximal length of receiving PDUs (Protocol Data Unit) [default=16352]
        policy.setMaxPDULength(Integer.parseInt(cfg.getProperty("max-pdu-len", "16352")));
        
        // Maximal number of invoked operations with outstanding response.
        policy.setAsyncOpsWindow(Integer.parseInt(cfg.getProperty("max-op-invoked", "0")), 1);
        
        for (@SuppressWarnings("rawtypes")
		Enumeration it = cfg.keys(); it.hasMoreElements(); ) {
            String key = (String) it.nextElement();
            
            // Setup available transfer syntaces for storage SOP classes
            // PS 3.4 - Annex B STORAGE SERVICE CLASS
            // PS 3.4 - B.5 STANDARD SOP CLASSES
            if (key.startsWith("pc.")) {
                initPresContext(key.substring(3), cfg.tokenize(cfg.getProperty(key)));
            }
            
            // Setup user/provider role for Management Service Classes
            // PS3.4 - Annex E PATIENT MANAGEMENT SERVICE CLASS (retired since 2004)
            //                 -- Detached Patient Management SOP class
            // PS3.4 - Annex G RESULTS MANAGEMENT SERVICE CLASS (retired since 2004)
            //                 -- Detached Result Management SOP class
            //                 -- Detached Interpretation Management SOP class
            if (key.startsWith("role.")) {
                initRole(key.substring(5), cfg.tokenize(cfg.getProperty(key)));
            }
        }
    }
    
    
    /**
     * Only used by method initPolicy:
     * Setup available transfer syntaces for storage SOP classes.
     * @param asName name of the storage SOP class (PS 3.4 - B.5 STANDARD SOP CLASSES)
     * @param tsNames the list of transfer syntaces.
     */
    private void initPresContext(String asName, String[] tsNames) {
        String as = UIDs.forName(asName);
        String[] tsUIDs = new String[tsNames.length];
        for (int i = 0; i < tsUIDs.length; ++i) {
            tsUIDs[i] = UIDs.forName(tsNames[i]);
        }
        policy.putPresContext(as, tsUIDs);
        services.bind(as, this);
    }
    
    
    /**
     * Only used by method initPolicy:
     * <p>Setup user/provider role for Management Service Classes.
     * <p>PS3.4 - Annex E PATIENT MANAGEMENT SERVICE CLASS (retired since 2004)
     * <p>                -- Detached Patient Management SOP class
     * <p>PS3.4 - Annex G RESULTS MANAGEMENT SERVICE CLASS (retired since 2004)
     * <p>                -- Detached Result Management SOP class
     * <p>                -- Detached Interpretation Management SOP class
     * @param asName the name of the Detached Management SOP class.
     * @param roles the list of implemented roles. Enumerated: "scu" and "scp".
     */
    private void initRole(String asName, String[] roles) {
        String as = UIDs.forName(asName);
        policy.putRoleSelection(as, contains(roles, "scu"), contains(roles, "scp"));
    }
    
    
    /**
     * Only used by method initRole:
     * Returns true, if one of the Strings in roles are equal to the key String.
     * @param roles an array of Strings.
     * @param key a key to match.
     */
    private boolean contains(String[] roles, String key) {
        for (int i = 0; i < roles.length; i++) {
            if (key.equalsIgnoreCase(roles[i]))
                return true;
        }
        return false;
    }
    
    
    /**
     * Add a listener to StorageServiceClassEvenEvents.
     * @param l the listener object.
     */
    @SuppressWarnings("unchecked")
	public synchronized void addStorageServiceClassEventListener(StorageServiceEventListener l) {
        // add a listener if it is not already registered
        if (!eventListeners.contains(l)) {
            eventListeners.addElement(l);
        }
    }
    
    
    /**
     * Removes a listener to StorageServiceClassEvenEvents.
     * @param l the listener object.
     */
    public synchronized void removeStorageServiceClassEvenEventListener(StorageServiceEventListener l) {
        // remove a listener if it is not already registered
        if (eventListeners.contains(l)) {
            eventListeners.removeElement(l);
        }
    }
    
    
    /**
     * Notify all listener to StorageServiceClassEvenEvents.
     * @param evt the event object.
     */
    @SuppressWarnings("rawtypes")
	private void fireStorageServiceClassEvent(StorageServiceEvent evt) {
        Vector v = null;
        
        // make a copy of the listener object so that it cannot be
        // changed while we are firing events
        synchronized(this) {
            v = (Vector) eventListeners.clone();
        }
        
        // fire the events
        for (int i = 0; i < v.size(); i++) {
            StorageServiceEventListener client = (StorageServiceEventListener) v.elementAt(i);
            client.handleStorageServiceEvent(evt);
        }
    }
    
    
    /**
     *  Start the server.
     *
     * @exception  IOException
     */
    public void start() throws IOException {
        server.start();
    }
    
    
    /**
     * Stops the server.
     */
    public void stop() {
        server.stop();
    }
    
    
    /**
     * If a DICOM object is received,this method is automatically invoken by the 
     * server implementation of the toolkit. This method extracts the Dataset of 
     * this object is send to registered listeners as a StorageServiceEvent.
     * <p>
     * <p>Overwrites the doCStore method of the parent class DcmServiceBase.
     * 
     * @param assoc
     * @param rq
     * @param rspCmd
     * @exception IOException
     */
    public void doCStore(ActiveAssociation assoc, Dimse rq, Command rspCmd) throws IOException {
        Command rqCmd = rq.getCommand();
        FileMetaInfo fmi = objFact.newFileMetaInfo(rqCmd.getAffectedSOPClassUID(), rqCmd.getAffectedSOPInstanceUID(), rq.getTransferSyntaxUID());
        
        // Send the event
        
        System.out.println("Inside DoCCStore");
        fireStorageServiceClassEvent(new StorageServiceEvent(this, rq.getDataset()));
        
        if (rspDelay > 0L) {
            try {
                Thread.sleep(rspDelay);
            } catch (InterruptedException ie) {
            	System.out.println(ie.getMessage());
            }
        }
        rspCmd.putUS(Tags.Status, rspStatus);
    }
    
}
