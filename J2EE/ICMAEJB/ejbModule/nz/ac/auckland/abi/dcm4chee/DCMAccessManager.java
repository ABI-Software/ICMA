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
package nz.ac.auckland.abi.dcm4chee;

import java.io.BufferedInputStream;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.StringWriter;
import java.net.ConnectException;
import java.net.Socket;
import java.net.URL;
import java.security.GeneralSecurityException;
import java.text.ParseException;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.Enumeration;
import java.util.GregorianCalendar;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Vector;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.dcm4che.data.Command;
import org.dcm4che.data.Dataset;
import org.dcm4che.data.DcmObjectFactory;
import org.dcm4che.data.DcmParserFactory;
import org.dcm4che.dict.DictionaryFactory;
import org.dcm4che.dict.Tags;
import org.dcm4che.dict.UIDDictionary;
import org.dcm4che.dict.UIDs;
import org.dcm4che.net.AAssociateAC;
import org.dcm4che.net.AAssociateRQ;
import org.dcm4che.net.ActiveAssociation;
import org.dcm4che.net.Association;
import org.dcm4che.net.AssociationFactory;
import org.dcm4che.net.Dimse;
import org.dcm4che.net.FutureRSP;
import org.dcm4che.net.PDU;
import org.dcm4che.net.PresContext;
import org.dcm4che.util.DcmURL;
import org.dcm4che.util.SSLContextAdapter;
import org.dcm4che2.data.DicomObject;
import org.dcm4che2.tool.dcmsnd.DcmSnd;
import org.json.simple.JSONObject;

/**
 * Implementation of C-DIMSE services.
 * <p>
 * <p>
 * Usage:
 * <p>
 * 1. Create a new instance of this class.
 * <p>
 * 2. Use the Connect method to establish an association.
 * <p>
 * 3. Use the query method to query the archive.
 * <p>
 * 4. Use the move method to move an object from the archive to a destination
 * server.
 * <p>
 * 5. Use the store to store an object into an archive.
 * <p>
 * 6. Use the ping to verify a association.
 * <p>
 * 7. If you are ready with the C-DIMSE services use the Disconnect method to
 * close the association.
 * <p>
 * <p>
 * The query/retrieve levels used by query and move are defined as enummerated
 * constants. Use the method getQueryRetrieveLevel to convert these values to
 * the String representation used in the DICOM element QueryRetrieveLevel
 * (0008,0052).
 * <p>
 * <p>
 * Based on dcm4che 1.4.0 sample: MoveStudy.java revision date 2005-10-05
 * <p>
 * Based on dcm4che 1.4.0 sample: DcmSnd.java revision date 2005-10-05
 * <p>
 * <p>
 * See: PS 3.4 - Annex B STORAGE SERVICE CLASS
 * <p>
 * See: PS 3.4 - Annex C QUERY/RETRIEVE SERVICE CLASS
 * <p>
 * See: PS 3.4 - C.6 SOP CLASS DEFINITIONS
 * <p>
 * See: PS 3.4 - C.6.2 Study Root SOP Class Group
 * <p>
 * <p>
 * Details of how to run the services is given in a configuration property file.
 * A sample may be found at "./resources/CDimseService.cfg".
 * 
 * 
 * @author Thomas Hacklaender
 * @version 2006-08-28
 * 
 *          Modified by Jagir Hussan Date: 2011-07-15
 */

public class DCMAccessManager {
	static final boolean DEBUG = false;

	private Logger log = Logger.getLogger(DCMAccessManager.class.getSimpleName());

	static final UIDDictionary uidDict = DictionaryFactory.getInstance().getDefaultUIDDictionary();
	static final AssociationFactory associationFactory = AssociationFactory.getInstance();
	static final DcmObjectFactory objectFactory = DcmObjectFactory.getInstance();
	static final DcmParserFactory parserFactory = DcmParserFactory.getInstance();
	static final DcmObjectFactory dicomObjectFactory = DcmObjectFactory.getInstance();

	/**
	 * Default AE title used for the association if it is not explicit given in
	 * the url filed.
	 */
	static final String CALLING_AET = "ABIECHO";
	/**
	 * Query/Retrieve Level Values for Study = "PATIENT". See PS 3.4 - C.6 SOP
	 * CLASS DEFINITIONS
	 */
	public static final int PATIENT_LEVEL = 0;
	/**
	 * Query/Retrieve Level Values for Study = "STUDY". See PS 3.4 - C.6. SOP
	 * CLASS DEFINITIONS
	 */
	public static final int STUDY_LEVEL = 1;
	/**
	 * Query/Retrieve Level Values for Series = "SERIES". See PS 3.4 - C.6 SOP
	 * CLASS DEFINITIONS
	 */
	public static final int SERIES_LEVEL = 2;
	/**
	 * Query/Retrieve Level Values for Image = "IMAGE". See PS 3.4 - C.6 SOP
	 * CLASS DEFINITIONS
	 */
	public static final int IMAGE_LEVEL = 3;

	PresContext pc;

	/**
	 * The configuration properties used for initialising the instance
	 */
	public DCMConfigProperties properties = null;

	/**
	 * DICOM URL to define a communication partner for an association.
	 * <p>
	 * PROTOCOL://CALLED[:CALLING]@HOST[:PORT]
	 * <p>
	 * <p>
	 * PROTOCOL Specifies protocol. Possible values:
	 * <p>
	 * - dicom DICOM default (without TLS)
	 * <p>
	 * - dicom-tls DICOM on TLS (offer AES and DES encryption)
	 * <p>
	 * - dicom-tls.aes DICOM on TLS (force AES or DES encryption)
	 * <p>
	 * - dicom-tls.3des DICOM on TLS (force DES encryption)
	 * <p>
	 * - dicom-tls.nodes DICOM on TLS (no encryption)
	 * <p>
	 * CALLED Called AET in association request (max 16 chars)
	 * <p>
	 * CALLING Calling AET in association request (max 16 chars) [default id
	 * final field DEFAULT_CALLING_AET = MYAET]
	 * <p>
	 * HOST Name or IP address of host, where the server is running
	 * <p>
	 * PORT TCP port address, on which the server is listing for
	 * <p>
	 * incoming TCP Transport Connection Indication [default=104]
	 */
	public DcmURL url = null;
	/**
	 * Message priority. Possible values Command.LOW = 2, Command.MEDIUM = 0,
	 * Command.HIGH = 1
	 */
	int priority = 0;

	/**
	 * Time-out waiting [in msec] for A-ASSOCIATE-AC acknowledge, 0 is
	 * interpreted as an infinite timeout [default=5000].
	 */
	int acTimeout = 5000;

	/**
	 * Time-out waiting [in msec] for DIMSE on Connect association, 0 is
	 * interpreted as an infinite timeout [default=0].
	 */
	int dimseTimeout = 0;

	/**
	 * Time delay [in msec] for socket Disconnect after sending A-ABORT
	 * [default=500].
	 */
	int soCloseDelay = 500;

	/**
	 * Association Request package (A-ASSOCIATE-RQ) is part of the connection
	 * service ACSE (Association Control Service Element). In TCP/IP networks
	 * connection services are emulated by the "DICOM Upper Layer Service". This
	 * presentation service encapsulates the data in PDUs (Protocol Data Unit).
	 */
	AAssociateRQ assocRQ = associationFactory.newAAssociateRQ();

	/** Association object for establishing an active association. */
	Association association = null;

	/** Accepted association */
	ActiveAssociation activeAssociation = null;

	/**
	 * Activates packing of command PDV (Presentation Data Value) + (first) data
	 * PDV into one P-DATA-TF PDU (Protocol Data Unit)
	 */
	boolean packPDVs = false;

	/** TLS context. Set by method initTLS */
	SSLContextAdapter tls = null;

	/**
	 * An array of implemented ciphers for the communication protocol (given in
	 * url).
	 */
	String[] cipherSuites = null;

	/**
	 * Specifies Key Attributes used for query. Wildcards '*','?', '-' and '\'
	 * are allowed as element-values (see PS 3.4 - C.2.2.2 Attribute Matching).
	 * If an key attribute is defined, but has an empty value, it matches every
	 * value at the storage side.
	 * <p>
	 * Key attributes have a type (PS 3.4 - C.2.2.1 Attribute Types)
	 * <p>
	 * - U = one Attribute shall be defined as a Unique Key.
	 * <p>
	 * - R = a set of Attributes shall be defined as Required Keys.
	 * <p>
	 * - O = a set of Attributes shall be defined as Optional Keys.
	 * <p>
	 * The complete list of Key Attributes can be found at PS 3.4 - C.6.2 Study
	 * Root SOP Class Group.
	 * <p>
	 * As a result of query for each matching item in the archive a DIMSE object
	 * containing the Key Attributes is send back. In this object the Key
	 * Attributes are set corresponding to values found in the archive.
	 * Attributes of type "O" may be send back with empty value. Only used by
	 * query.
	 */
	Dataset keys = dicomObjectFactory.newDataset();

	/**
	 * Application Entity Title (AET) of the destination for the move. The AET
	 * must be known by the archive together with the host IP address and the
	 * port number. Only used by move.
	 */
	String dest;

	/**
	 * Initializes Association related parameters.
	 * 
	 * @param cfg
	 *            the configuration properties for this class.
	 * @param url
	 *            the DcmURL of the communication partner.
	 */
	protected final void initAssocParam(DCMConfigProperties cfg, DcmURL url) {
		String callingAET = null;

		// >>>> Get data for filling the Association object for establishing an
		// >>>> active association from configuration file
		acTimeout = Integer.parseInt(cfg.getProperty("ac-timeout", "5000"));
		dimseTimeout = Integer.parseInt(cfg.getProperty("dimse-timeout", "0"));
		soCloseDelay = Integer.parseInt(cfg.getProperty("so-close-delay", "500"));

		// >>>> Fill the Association Request package (A-ASSOCIATE-RQ)

		// Identifying the communication partner by an AET (Application Entity
		// Title)
		assocRQ.setCalledAET(url.getCalledAET());
		// Identifying ourselves by an AET (Application Entity Title)
		if (url.getCallingAET() != null)
			callingAET = url.getCallingAET();
		else {
			callingAET = CALLING_AET;
		}
		// Maximum size of one PDU (Protocol Data Unit)
		assocRQ.setCallingAET(callingAET);

		// Defines possibilities for asynchron DIMSE communication. Noramly
		// synchron DIMSE communication is used.
		// API doc: AssociationFactory.newAsyncOpsWindow(int maxOpsInvoked, int
		// maxOpsPerfomed)
		// PS 3.7 - Annex D.3.3.3 ASYNCHRONOUS OPERATIONS WINDOW NEGOTIATION
		// maxOpsInvoked: This field shall contain the
		// Maximum-number-operationsinvoked as defined for the
		// Association-requester
		// maxOpsPerfomed: This field shall contain the
		// Maximum-number-operationsperformed as defined for the
		// Association-requester
		assocRQ.setMaxPDULength(Integer.parseInt(cfg.getProperty("max-pdu-len", "16352")));

		assocRQ.setAsyncOpsWindow(associationFactory.newAsyncOpsWindow(Integer.parseInt(cfg.getProperty("max-op-invoked", "0")), 1));

		for (@SuppressWarnings("rawtypes")
		Enumeration it = cfg.keys(); it.hasMoreElements();) {
			String key = (String) it.nextElement();
			// Setup available transfer syntaxes for storage SOP classes
			// PS 3.4 - Annex B STORAGE SERVICE CLASS
			// PS 3.4 - B.5 STANDARD SOP CLASSES
			if (key.startsWith("pc."))
				initPresContext(Integer.parseInt(key.substring(3)), cfg.tokenize(cfg.getProperty(key), new LinkedList<String>()));
		}
	}

	/**
	 * Only used by method initAssocParam: Sets up available transfer syntaxes
	 * for storage SOP classes.
	 * 
	 * @param pcid
	 *            is a for the association unique odd number between 1 and 255.
	 * @param val
	 *            a list: First element is the symbolic name of the UID of the
	 *            SOP to transmit, the following elements are the supported
	 *            transfer syntax for that SOP.
	 */
	protected final void initPresContext(int pcid, List<String> val) {
		Iterator<String> it = val.iterator();
		String as = UIDs.forName((String) it.next());
		String[] tsUIDs = new String[val.size() - 1];
		for (int i = 0; i < tsUIDs.length; ++i) {
			tsUIDs[i] = UIDs.forName((String) it.next());
		}
		// API doc: AssociationFactory.newPresContext(int pcid, String asuid,
		// String[] tsuid)
		// pcid is a for the association unique odd number between 1 and 255.
		// asuid is the UID of a SOP class
		// TS list of transfer syntaces supported by this class for asuid.
		assocRQ.addPresContext(associationFactory.newPresContext(pcid, as, tsUIDs));
	}

	/**
	 * Initializes TLS (Transport Layer Security, predecessor of SSL, Secure
	 * Sockets Layer) connection related parameters. TLS expects RSA (Ron
	 * Rivest, Adi Shamir and Len Adleman) encoded keys and certificates.
	 * 
	 * <p>
	 * Keys and certificates may be stored in PKCS #12 (Public Key Cryptography
	 * Standards) or JKS (Java Keystore) containers.
	 * 
	 * <p>
	 * TSL is used to encrypt data during transmission, which is accomplished
	 * when the connection between the two partners A (normally the client) and
	 * B (normally the server) is established. If A asks B to send TSL encrypted
	 * data, both partners exchange some handshake information. In a first step
	 * B tries to authenticate itself against A (server authentication, optional
	 * in TSL but implemented in this way in dcm4che). For that B presents its
	 * public key for A to accept or deny. If the authentication is accepted, A
	 * tries to authenticate itself against B (client authentication, optional
	 * in TSL but implemented in this way in dcm4che).If B accepts the
	 * authentication A and B agree on a hash (symmetric key, which is
	 * independent of the private/public keys used for authentication) for the
	 * duration of their conversation, which is used to encrypt the data.
	 * 
	 * <p>
	 * To be able to establish a TSL connection B needs a private/public key
	 * pair, which identifies B unambiguously. For that the private key is
	 * generated first; than the root-public key is derived from that private
	 * key. To bind the root-public key with the identity of B a Certificate
	 * Authority (CA) is used: The root-public key is send to CA, which returns
	 * a certified-public key, which includes information about the CA as well
	 * as the root-public key. Optionally this process can be repeated several
	 * times so that the certified-public key contains a chain of certificates
	 * of different CA's.
	 * 
	 * <p>
	 * The certified-public key of B is presented to A during server
	 * authentication. Partner A should accept this key, if it can match the
	 * last certificate in the chain with a root-certificate found in its local
	 * list of root-certificates of CA's. That means, that A does not check the
	 * identity of B, but "trusts" B because it was certified by an authority.
	 * The same happens for client authentication. Handling of authentication of
	 * the identity of the communication partner is subject of PS 3.15 -
	 * Security and System Management Profiles.
	 * 
	 * <p>
	 * In the configuration files of this method the certified-public key is
	 * stored in the property "tls-key" and the root-certificates of the known
	 * CA's in "tls-cacerts".
	 * 
	 * <p>
	 * Usually the certified-public keys of A and B are different, but they also
	 * may be the same. In this case the root-certificates are also the same.
	 * 
	 * <p>
	 * It is possible to establish a TLS connection without using a CA: In this
	 * case both partners creates a individual container holding their private
	 * and root-public key. These containers could be used for certifying also,
	 * because the length of the certifying chain is one and therefore the
	 * root-public key is also the last certified-public key. Therefore the
	 * root-public key works in this scenario also as the root-certificate of
	 * the "certifying authority".
	 * 
	 * <p>
	 * If no keystores are specified in the configuration properties, the
	 * not-certified default-keystore "resources/identityJava.jks" is used for
	 * "tls-key" and "tls-cacerts" when establishing the connection.
	 * 
	 * @param cfg
	 *            the configuration properties for this class.
	 * @throws ParseException
	 */
	protected void initTLS(DCMConfigProperties cfg) throws ParseException {
		try {
			// Cipher suites for protokoll:
			// dicom = null
			// dicom-tls = SSL_RSA_WITH_NULL_SHA, TLS_RSA_WITH_AES_128_CBC_SHA,
			// SSL_RSA_WITH_3DES_EDE_CBC_SHA
			// dicom-tls.3des = SSL_RSA_WITH_3DES_EDE_CBC_SHA
			// dicom-tls.aes = TLS_RSA_WITH_AES_128_CBC_SHA,
			// SSL_RSA_WITH_3DES_EDE_CBC_SHA
			// dicom-tls.nodes = SSL_RSA_WITH_NULL_SHA
			cipherSuites = url.getCipherSuites();
			if (cipherSuites == null) {
				return;
			}
			// Get a new TLS context
			tls = SSLContextAdapter.getInstance();

			// >>>> Managing the keystore file containing the privat key and
			// >>>> certified-public key to establish the communication
			// Password of the keystore [default: secret]
			char[] keystorepasswd = cfg.getProperty("tls-keystore-passwd", "secret").toCharArray();
			// Password of the private key [default: secret]
			char[] keypasswd = cfg.getProperty("tls-key-passwd", "secret").toCharArray();
			// URL of the file containing the default-keystore
			URL keyURL = DCMAccessManager.class.getResource("/resources/identityJava.jks");
			// If available, replace URL with the one specified in the
			// configuration file
			String value;
			if ((value = cfg.getProperty("tls-key")) != null) {
				try {
					// Property specified, try to set to specified value
					keyURL = DCMConfigProperties.fileRefToURL(DCMAccessManager.class.getResource(""), value);
				} catch (Exception e) {
					log.log(Level.INFO, "Wrong value for tls-key: " + value + ". tls-key was set to default value.");
				}

			}

			// Sets the key attribute of the SSLContextAdapter object
			// API doc: SSLContextAdapter.loadKeyStore(java.net.URL url, char[]
			// password)
			// API doc: SSLContextAdapter.setKey(java.security.KeyStore key,
			// char[] password)
			tls.setKey(tls.loadKeyStore(keyURL, keystorepasswd), keypasswd);
			// >>>> Managing the keystore containing the root-certificates of
			// the Ceritifying Authorities
			// >>>> used for signing the public key

			// Password of the keystore [default: secret]
			char[] cacertspasswd = cfg.getProperty("tls-cacerts-passwd", "secret").toCharArray();

			// URL of the file containing the default-keystore
			URL cacertURL = DCMAccessManager.class.getResource("/resources/identityJava.jks");

			// If available, replace URL with the one specified in the
			// configuration file
			if ((value = cfg.getProperty("tls-cacerts")) != null) {
				try {
					cacertURL = DCMConfigProperties.fileRefToURL(DCMAccessManager.class.getResource(""), value);
				} catch (Exception e) {
					log.log(Level.INFO, "Wrong value for tls-cacerts: " + value + ". tls-cacerts was set to default value.");
				}

			}
			// Sets the trust attribute of the SSLContextAdapter object
			// API doc: SSLContextAdapter.loadKeyStore(java.net.URL url, char[]
			// password)
			// API doc: SSLContextAdapter.setTrust(java.security.KeyStore
			// cacerts)
			tls.setTrust(tls.loadKeyStore(cacertURL, cacertspasswd));

			// Init TLS context adapter
			tls.init();
		} catch (Exception ex) {
			throw new ParseException("Could not initalize TLS configuration.", 0);
		}
	}

	/**
	 * Prepares the Dataset representing the search key in query. As no values
	 * are set, the keys match to every content in the archive. The user has to
	 * specify concrete values to limit the search See PS 3.4 - C.6.2.1.2 Study
	 * level.
	 * <p>
	 * As the result for query these keys are filled with the values found in
	 * the archive.
	 * 
	 * @param cfg
	 *            the configuration properties for this class.
	 * @throws ParseException
	 *             if a given properties for the keys was not found.
	 */
	protected void initKeys(DCMConfigProperties cfg) throws ParseException {
		// Remove all keys
		keys = dicomObjectFactory.newDataset();

		// Query/Retrieve Level. PS 3.4 - C.6.2 Study Root SOP Class Group
		keys.putCS(524370, getQueryRetrieveLevel(STUDY_LEVEL));

		// UNIQUE STUDY LEVEL KEY FOR THE STUDY. See PS 3.4 - C.6.2.1.2 Study
		// level
		keys.putUI(2097165);
		// REQUIRED STUDY LEVEL KEY FOR THE STUDY. See PS 3.4 - C.6.2.1.2 Study
		// level
		keys.putDA(524320);
		// Not defined: StudyTime
		// Not defined: AccessionNumber
		keys.putPN(1048592);
		keys.putLO(1048608);
		// Not defined: StudyID

		// OPTIONAL STUDY LEVEL KEY FOR THE STUDY. See PS 3.4 - C.6.2.1.2 Study
		// level
		keys.putUS(2101766);
		keys.putUS(2101768);
		// much more defined...

		// Add the keys found in the configuration properties
		addQueryKeys(cfg);
	}

	/**
	 * Add the query keys found in the configuration properties to the Dataset
	 * "keys" used be the query method.
	 * 
	 * @param cfg
	 *            the configuration properties for this class.
	 * @throws ParseException
	 *             if a given properties for the keys was not found.
	 */
	@SuppressWarnings("rawtypes")
	protected void addQueryKeys(DCMConfigProperties cfg) throws ParseException {
		for (Enumeration it = cfg.keys(); it.hasMoreElements();) {
			String key = (String) it.nextElement();
			if (key.startsWith("key."))
				try {
					keys.putXX(Tags.forName(key.substring(4)), cfg.getProperty(key));
				} catch (Exception e) {
					throw new ParseException("Illegal entry in configuration filr: " + key + "=" + cfg.getProperty(key), 0);
				}
		}
	}

	/**
	 * Creates a stream socket and connects it to the specified port number on
	 * the named host.
	 * 
	 * @param host
	 *            the IP address.
	 * @param port
	 *            the port number.
	 * @return the Socket.
	 * @exception IOException
	 * @exception GeneralSecurityException
	 */
	protected Socket newSocket(String host, int port) throws IOException, GeneralSecurityException {

		// Test, if a secured connection is needed
		if (cipherSuites != null) {
			// Creates a socket for secured connection.
			// The SSLContextAdapter tls uses the javax.net.ssl package for
			// establishing
			// the connection.
			return tls.getSocketFactory(cipherSuites).createSocket(host, port);
		} else {

			// Creates a standard Java socket for unsecured connection.
			return new Socket(host, port);
		}
	}

	/**
	 * Starts a active association to a communication partner. See PS 3.8 - 7.1
	 * A-ASSOCIATE SERVICE
	 * 
	 * @return true, if association was successful established.
	 * @exception ConnectException
	 * @exception IOException
	 * @exception GeneralSecurityException
	 */
	public boolean Connect() throws ConnectException, IOException, GeneralSecurityException {
		// No association may be active
		if (association != null) {
			throw new ConnectException("Association already established");
		}

		// New Association object for establishing an active association
		association = associationFactory.newRequestor(newSocket(url.getHost(), url.getPort()));
		// >>>> Fill the Association object with relevant data
		association.setAcTimeout(acTimeout);
		association.setDimseTimeout(dimseTimeout);
		association.setSoCloseDelay(soCloseDelay);
		association.setPackPDVs(packPDVs);

		// 1. Create an communication channel to the communication partner
		// defined in the Association object
		// 2. Send the A-ASSOCIATE-RQ package
		// 3. Receive the aAssociation acknowledge/reject package from the
		// communication partner as a PDU (Protocol Data Unit)
		PDU assocAC = association.connect(assocRQ);

		if (!(assocAC instanceof AAssociateAC)) {
			// Acknowlage is A-ASSOCIATE-RJ
			// Association rejected
			association = null;
			// Return aASSOCIATE failed
			return false;
		}
		// Acknowledge is A-ASSOCIATE_AC
		// Association accepted

		// Start the accepted association
		// API doc: AssociationFactory.newActiveAssociation(Association assoc,
		// DcmServiceRegistry services
		activeAssociation = associationFactory.newActiveAssociation(association, null);
		activeAssociation.start();
		// Return successful opened
		return true;
	}

	/**
	 * Releases the active association. See PS 3.8 - 7.2 A-RELEASE SERVICE
	 * 
	 * @param waitOnRSP
	 *            if true, method waits until it receives the responds to the
	 *            release request.
	 * @exception InterruptedException
	 *                Description of the Exception
	 * @exception IOException
	 *                Description of the Exception
	 */
	public void Disconnect(boolean waitOnRSP) throws InterruptedException, IOException {
		if (association == null)
			return;
		try {
			activeAssociation.release(waitOnRSP);
		} finally {
			association = null;
			activeAssociation = null;
		}
	}

	/**
	 * Stores a DICOM object in an archive (Storage SCP).
	 * <p>
	 * See PS 3.4 - Annex B STORAGE SERVICE CLASS.
	 * 
	 * @param ds
	 *            the Dataset to store.
	 * @throws ConnectException
	 * @throws ParseException
	 * @throws IOException
	 * @throws InterruptedException
	 * @throws IllegalStateException
	 */
	public void store(Dataset ds) throws InterruptedException, IOException, ConnectException, ParseException {
		PresContext pc = null;
		log.log(Level.INFO, "In store call");
		// An association must be active
		if (activeAssociation == null)
			throw new ConnectException("No Association established");
		String sopClassUID;

		// SOP Class UID must be given
		if ((sopClassUID = ds.getString(524310)) == null) {
			throw new ParseException("No SOP Class UID in Dataset", 0);
		}
		// SOP Instance UID must be given
		String sopInstUID;
		if ((sopInstUID = ds.getString(524312)) == null) {
			throw new ParseException("No SOP Instance UID in Dataset", 0);
		}
		log.log(Level.INFO, "SOP Class UID " + sopClassUID);
		log.log(Level.INFO, "SOP Instance UID " + sopInstUID);
		// Test, if applicable presentation context was found
		if (((pc = activeAssociation.getAssociation().getAcceptedPresContext(sopClassUID, "1.2.840.10008.1.2")) == null)
				&& ((pc = activeAssociation.getAssociation().getAcceptedPresContext(sopClassUID, "1.2.840.10008.1.2.1")) == null)
				&& ((pc = activeAssociation.getAssociation().getAcceptedPresContext(sopClassUID, "1.2.840.10008.1.2.2")) == null)) {
			throw new ConnectException("No applicable presentation context found");
		}
		// New Command Set, see: DICOM Part 7: Message Exchange, 6.3.1 Command
		// Set Structure
		Command cStoreRQ = objectFactory.newCommand();
		// API doc: Command.initCStoreRQ(int msgID, String sopClassUID, String
		// sopInstUID, int priority)
		cStoreRQ.initCStoreRQ(association.nextMsgID(), sopClassUID, sopInstUID, priority);
		// API doc: AssociationFactorynewDimse(int pcid, Command cmd, Dataset
		// ds)
		// DIMSE (DICOM Message Service Element) ist ein Nachrichtendienst in DI
		Dimse storeRq = associationFactory.newDimse(pc.pcid(), cStoreRQ, ds);

		// PS 3.7 - 9.3.1 C-STORE PROTOCOL, 9.3.1.2 C-STORE-RSP
		// Always returns SUCESS result code.
		// Invoke active association with echo request Dimse
		FutureRSP future = activeAssociation.invoke(storeRq);

		log.log(Level.INFO, CALLING_AET + " is storing a dataset");

		// Response to the ping request.
		// The result cannot be accessed until it has been set
		Dimse storeRsp = future.get();
		Command rspCmd = storeRsp.getCommand();

		// PS 3.7 - 9.3.5 C-MOVE PROTOCOL, 9.3.5.2 C-ECHO-RSP
		int status = rspCmd.getStatus();
		switch (status) {
		case 0:
			break;
		default:
			log.log(Level.INFO, "STORE failed: " + Integer.toHexString(status));
		}
	}

	/*
	 * Retrieve Data associated with a patient/study
	 * 
	 * @param - ds, Details of the patient, study details
	 * 
	 * @return - Vector<Dataset>, list of data associated with patient/study
	 */
	public Vector<Dataset> retrieve(Dataset ds) throws ConnectException, IOException, InterruptedException {

		// An association must be active
		if (activeAssociation == null)
			throw new ConnectException("No Association established");
		PresContext pc;

		// Test, if Presentation Context for C-MOVE is supported
		// API doc: Association.getAcceptedPresContext(String asuid, String
		// tsuid)
		if (((pc = activeAssociation.getAssociation().getAcceptedPresContext("1.2.840.10008.5.1.4.1.2.2.3", "1.2.840.10008.1.2.1")) == null)
				&& ((pc = activeAssociation.getAssociation().getAcceptedPresContext("1.2.840.10008.5.1.4.1.2.2.3", "1.2.840.10008.1.2")) == null)) {
			throw new ConnectException("Association does not support presentation context for StudyRootQueryRetrieveInformationModelMOVE SOP.");
		}

		// Get the Study Instance UID of the study to mode
		String suid = ds.getString(524312);
		// Prepare info for logging
		String patName = ds.getString(1048592);
		String patID = ds.getString(1048608);
		String studyDate = ds.getString(524320);
		String prompt = "Study[" + suid + "] from " + studyDate + " for Patient[" + patID + "]: " + patName;

		// New Command Set, see: DICOM Part 7: Message Exchange, 6.3.1 Command
		// Set Structure
		Command rqCmd = dicomObjectFactory.newCommand();
		// API doc: Command.initCMoveRQ(int msgID, String sopClassUID, int
		// priority, String moveDest)
		rqCmd.initCGetRSP(association.nextMsgID(), "1.2.840.10008.5.1.4.1.2.2.3", priority);
		Dataset rqDs = dicomObjectFactory.newDataset();
		rqDs.putCS(524370, getQueryRetrieveLevel(1));
		// Only Unique Key allowed in MOVE. PS 3.4 -C.2.2.1 Attribute Types
		rqDs.putUI(524312, suid);
		// API doc: AssociationFactorynewDimse(int pcid, Command cmd, Dataset
		// ds)
		// DIMSE (DICOM Message Service Element) ist ein Nachrichtendienst in
		// DICOM
		Dimse moveRq = associationFactory.newDimse(pc.pcid(), rqCmd, rqDs);

		// Invoke active association with move request Dimse
		FutureRSP future = activeAssociation.invoke(moveRq);
		// Response to the C-MOVE request.
		// The result cannot be accessed until it has been set
		Dimse moveRsp = future.get();
		Command rspCmd = moveRsp.getCommand();

		if (DEBUG) {
			StringWriter w = new StringWriter();
			w.write("C-FIND RQ Identifier:\n");
			keys.dumpDataset(w, null);
			log.log(Level.INFO, w.toString());
		}
		// Invoke active association with find request Dimse

		// Response to the C-FIND request.
		// The result cannot be accessed until it has been set.

		// Get the list of found objects
		@SuppressWarnings("rawtypes")
		List dimseList = future.listPending();

		// >>>> Extract Dataset from Dimse
		Vector<Dataset> datasetVector = new Vector<Dataset>();

		// If no List of DIMSE objects was generated or it is empty return an
		// empty Vector
		if ((dimseList == null) || (dimseList.isEmpty())) {
			return datasetVector;
		}
		// Process all elements
		for (int i = 0; i < dimseList.size(); ++i) {
			datasetVector.addElement(((Dimse) dimseList.get(i)).getDataset());
			if (((Dimse) dimseList.get(i)).getDataset() == null) {
				log.log(Level.FINEST, "              Dataset created succesffully          ");
			}

		}
		// PS 3.7 - 9.3.4 C-MOVE PROTOCOL, 9.3.4.2 C-MOVE-RSP
		int status = rspCmd.getStatus();
		switch (status) {
		case 0:
			break;
		case 45056:
			log.log(Level.INFO, "One or more failures during move of " + prompt);
			break;
		default:
			log.log(Level.SEVERE, "Failed to move " + prompt + "\n\terror tstatus: " + Integer.toHexString(status));
		}

		log.log(Level.INFO, "The move sise is : " + datasetVector.size());
		return datasetVector;
	}

	/*
	 * Method to setup keys for querying
	 * 
	 * @param - ds, property pairs for querying
	 */
	public void setQueryKeys(Dataset ds) {
		keys = ds;
	}

	public Dataset getQueryKeys() {
		return keys;
	}

	/*
	 * Method to setup keys for querying
	 * 
	 * @param - cfg, properties for querying using DCMConfig
	 */
	public void setQueryKeys(DCMConfigProperties cfg) throws ParseException {
		keys = dicomObjectFactory.newDataset();

		addQueryKeys(cfg);
	}

	/**
	 * Queries the archive for DICOM objects matching Attribute Keys defined in
	 * the local field "keys". This field is set by the constructor out of the
	 * configuration parameters or by the methods setQueryKeys(Configuration)
	 * and setQueryKeys(Dataset). See PS 3.4 - Annex C QUERY/RETRIEVE SERVICE
	 * CLASS.
	 * <p>
	 * The method returns, when the result is received from the communication
	 * partner.
	 * 
	 * 
	 * @return the result of the query as a Vector of Dataset objects each
	 *         specifying one matching DICOM object. If no matching objects are
	 *         found an empty Vector is returned.
	 * @throws ConnectException
	 * @throws IOException
	 */
	public Vector<Dataset> query() throws ConnectException, IOException, InterruptedException {
		// An association must be active
		if (activeAssociation == null) {
			throw new ConnectException("No Association established");
		}
		// Test, if Presentation Context for C-FIND is supported
		// API doc: Association.getAcceptedPresContext(String asuid, String
		// tsuid)
		// UIDs.StudyRootQueryRetrieveInformationModelGET
		if (((pc = activeAssociation.getAssociation().getAcceptedPresContext("1.2.840.10008.5.1.4.1.2.2.1", "1.2.840.10008.1.2.1")) == null)
				&& ((pc = activeAssociation.getAssociation().getAcceptedPresContext("1.2.840.10008.5.1.4.1.2.2.1", "1.2.840.10008.1.2")) == null)) {
			throw new ConnectException("Association does not support presentation context for StudyRootQueryRetrieveInformationModelFIND SOP.");
		}

		// New Cammand Set, see: DICOM Part 7: Message Exchange, 6.3.1 Command
		// Set Structure
		Command rqCmd = dicomObjectFactory.newCommand();
		// API doc: Command.initCFindRQ(int msgID, String sopClassUID, int
		// priority)
		rqCmd.initCFindRQ(association.nextMsgID(), "1.2.840.10008.5.1.4.1.2.2.1", priority);

		// API doc: AssociationFactorynewDimse(int pcid, Command cmd, Dataset
		// ds)
		// DIMSE (DICOM Message Service Element) ist ein Nachrichtendienst in
		// DICO
		Dimse findRq = associationFactory.newDimse(pc.pcid(), rqCmd, keys);

		if (DEBUG) {
			StringWriter w = new StringWriter();
			w.write("C-FIND RQ Identifier:\n");
			keys.dumpDataset(w, null);
			log.log(Level.INFO, w.toString());
		}
		// Invoke active association with find request Dimse
		FutureRSP future = activeAssociation.invoke(findRq);

		// Response to the query request.
		// The result cannot be accessed until it has been set
		@SuppressWarnings("unused")
		Dimse findRsp = future.get();

		// Get the list of found objects
		@SuppressWarnings("rawtypes")
		List dimseList = future.listPending();
		// >>>> Extract Dataset from Dimse
		Vector<Dataset> datasetVector = new Vector<Dataset>();

		// If no List of DIMSE objects was generated or it is empty return an
		// empty Vector
		if ((dimseList == null) || (dimseList.isEmpty())) {
			return datasetVector;
		}
		// Process all elements
		for (int i = 0; i < dimseList.size(); ++i) {
			datasetVector.addElement(((Dimse) dimseList.get(i)).getDataset());
		}

		return datasetVector;
	}

	/**
	 * Ask archive to move one DICOM object to the destination (C-MOVE). See PS
	 * 3.4 - Annex C QUERY/RETRIEVE SERVICE CLASS.
	 * <p>
	 * Use the Study Root Query/Retrieve Information Model to communicate with
	 * the archive. See PS 3.4 - C.6.2 Study Root SOP Class Group.
	 * <p>
	 * PS 3.4 - C.4.2.1.4.1 Request Identifier Structure (for C-MOVE): An
	 * Identifier in a C-MOVE request shall contain:
	 * <p>
	 * - the Query/Retrieve Level (0008,0052) which defines the level of the
	 * retrieval
	 * <p>
	 * - Unique Key Attributes which may include Patient ID, Study Instance
	 * UIDs, Series Instance UIDs, and the SOP Instance UIDs
	 * <p>
	 * PS 3.4 - C.4.2.2.1 Baseline Behavior of SCU (of C-MOVE):
	 * <p>
	 * The SCU shall supply a single value in the Unique Key Attribute for each
	 * level above the Query/Retrieve level. For the level of retrieve, the SCU
	 * shall supply one unique key if the level of retrieve is above the STUDY
	 * level and shall supply one UID, or a list of UIDs if a retrieval of
	 * several items is desired and the retrieve level is STUDY, SERIES or
	 * IMAGE.
	 * 
	 * @param ds
	 *            the DICOM object represented as a Dataset.
	 * @return a result-code: 0x0000 = SUCCESS sub-operations complete -no
	 *         failures, 0xB000 = WARNING sub-operations complete - one or more
	 *         failures, other = errors defined in PS 3.4 - C.4.2.1.5 Status
	 * @throws ConnectException
	 * @throws InterruptedException
	 * @throws IOException
	 */
	public Vector<Dataset> move(Dataset ds) throws ConnectException, InterruptedException, IOException {
		// An association must be active
		if (activeAssociation == null)
			throw new ConnectException("No Association established");

		// Test, if Presentation Context for C-MOVE is supported
		// API doc: Association.getAcceptedPresContext(String asuid, String
		// tsuid)
		PresContext pc;
		if (((pc = activeAssociation.getAssociation().getAcceptedPresContext("1.2.840.10008.5.1.4.1.2.2.2", "1.2.840.10008.1.2.1")) == null)
				&& ((pc = activeAssociation.getAssociation().getAcceptedPresContext("1.2.840.10008.5.1.4.1.2.2.2", "1.2.840.10008.1.2")) == null)) {
			log.log(Level.SEVERE, "" + activeAssociation.getAssociation().listAcceptedPresContext("1.2.840.10008.5.1.4.1.2.2.2"));
			throw new ConnectException("Association does not support presentation context for StudyRootQueryRetrieveInformationModelMOVE SOP.");
		}
		// Get the Study Instance UID of the study to mode
		String suid = ds.getString(2097165);

		// Prepare info for loggin
		String patName = ds.getString(1048592);
		String patID = ds.getString(1048608);
		String studyDate = ds.getString(524320);
		String prompt = "Study[" + suid + "] from " + studyDate + " for Patient[" + patID + "]: " + patName;

		// New Cammand Set, see: DICOM Part 7: Message Exchange, 6.3.1 Command
		// Set Structure
		Command rqCmd = dicomObjectFactory.newCommand();
		// API doc: Command.initCMoveRQ(int msgID, String sopClassUID, int
		// priority, String moveDest)
		rqCmd.initCMoveRQ(association.nextMsgID(), "1.2.840.10008.5.1.4.1.2.2.2", priority, dest);
		Dataset rqDs = dicomObjectFactory.newDataset();
		rqDs.putCS(524370, getQueryRetrieveLevel(1));
		// Only Unique Key allowed in C-MOVE. PS 3.4 -C.2.2.1 Attribute Types
		rqDs.putUI(2097165, suid);

		// API doc: AssociationFactorynewDimse(int pcid, Command cmd, Dataset
		// ds)
		// DIMSE (DICOM Message Service Element) ist ein Nachrichtendienst in
		// DICOM
		Dimse moveRq = associationFactory.newDimse(pc.pcid(), rqCmd, rqDs);
		// Invoke active association with move request Di
		FutureRSP future = activeAssociation.invoke(moveRq);

		// Response to the C-MOVE request.
		// The result cannot be accessed until it has been set.
		Dimse moveRsp = future.get();
		Command rspCmd = moveRsp.getCommand();
		@SuppressWarnings("unused")
		Dataset dds = moveRsp.getDataset();

		if (DEBUG) {
			StringWriter w = new StringWriter();
			w.write("C-FIND RQ Identifier:\n");
			keys.dumpDataset(w, null);
			log.log(Level.INFO, w.toString());
		}
		// Invoke active association with find request Dimse

		// Response to the C-FIND request.
		// The result cannot be accessed until it has been set.

		// Get the list of found objects
		@SuppressWarnings("unused")
		Dimse dms = future.get();

		// >>>> Extract Dataset from Dims
		Vector<Dataset> datasetVector = new Vector<Dataset>();
		// PS 3.7 - 9.3.4 C-MOVE PROTOCOL, 9.3.4.2 C-MOVE-RSP
		int status = rspCmd.getStatus();
		switch (status) {
		case 0:
			break;
		case 45056:
			log.log(Level.INFO, "One or more failures during move of " + prompt);
			break;
		default:
			log.log(Level.SEVERE, "Failed to move " + prompt + "\n\terror tstatus: " + Integer.toHexString(status));
		}

		return datasetVector;
	}

	/**
	 * Implements the ECHO service. The C-ECHO service is invoked by a
	 * DIMSE-service-user to verify end-to-end communications with a peer
	 * DIMSE-service-user. See PS 3.7 - 9.1.5 C-ECHO SERVICE
	 * 
	 * @exception ConnectException
	 * @exception InterruptedException
	 * @exception IOException
	 */
	public long ping() throws ConnectException, InterruptedException, IOException {
		long t1 = System.currentTimeMillis();
		// An association must be active
		if (this.activeAssociation == null)
			throw new ConnectException("No Association established");
		PresContext pc;

		// Test, if Presentation Context for C-ECHO is supported
		// API doc: Association.getAcceptedPresContext(String asuid, String
		// tsuid)
		if ((pc = activeAssociation.getAssociation().getAcceptedPresContext("1.2.840.10008.1.1", "1.2.840.10008.1.2")) == null) {
			throw new ConnectException("Association does not support presentation context: Verification SOP/ImplicitVRLittleEndian.");
		}

		// New Cammand Set, see: DICOM Part 7: Message Exchange, 6.3.1 Command
		// Set Structure
		Command cEchoRQ = objectFactory.newCommand();
		// API doc: Command.initCEchoRQ(int msgID
		cEchoRQ.initCEchoRQ(1);

		// API doc: AssociationFactorynewDimse(int pcid, Command cmd)
		// DIMSE (DICOM Message Service Element) ist ein Nachrichtendienst in
		// DICOM
		Dimse echoRq = associationFactory.newDimse(pc.pcid(), cEchoRQ);

		// PS 3.7 - 9.3.5 C-ECHO PROTOCOL, 9.3.5.2 C-ECHO-RSP
		// Always returns SUCESS result code.
		// Invoke active association with echo request Dim
		FutureRSP future = activeAssociation.invoke(echoRq);

		// Response to the C-ECHO request.
		// The result cannot be accessed until it has been set.
		Dimse echoRsp = future.get();
		Command rspCmd = echoRsp.getCommand();

		// PS 3.7 - 9.3.5 C-MOVE PROTOCOL, 9.3.5.2 C-ECHO-RSP
		int status = rspCmd.getStatus();
		switch (status) {
		case 0:
			break;
		default:
			log.log(Level.SEVERE, "C-ECHO failed: " + Integer.toHexString(status));
		}

		return (System.currentTimeMillis() - t1);
	}

	/**
	 * Gets the String value used for DICOM element QueryRetrieveLevel
	 * (0008,0052).
	 * <p>
	 * See PS 3.4 - C.6 SOP CLASS DEFINITIONS
	 * 
	 * @param queryRetrieveLevel
	 *            query/retrieve level as a enumerated value.
	 * @return the String value assiciated to enumerated query/retrieve level.
	 */
	public static String getQueryRetrieveLevel(int queryRetrieveLevel) {
		switch (queryRetrieveLevel) {
		case 0:
			return "PATIENT";
		case 1:
			return "STUDY";
		case 2:
			return "SERIES";
		case 3:
			return "IMAGE";
		}

		return "";
	}

	/**
	 * Constructor for the StorageSCUServiceClass object. Initializes
	 * everything.
	 * 
	 * <p>
	 * Details of how to run the server is given in another configuration
	 * property file. A sample may be found at
	 * "./resources/StorageSCUServiceClass.cfg".
	 * 
	 * @param cfg
	 *            the configuration properties for this class.
	 * @param url
	 *            the DcmURL of the communication partner.
	 * @throws ParseException
	 */
	public DCMAccessManager(DCMConfigProperties cfg, DcmURL url) throws ParseException {
		this.url = url;
		this.properties = cfg;
		priority = Integer.parseInt(cfg.getProperty("prior", "0"));
		packPDVs = "true".equalsIgnoreCase(cfg.getProperty("pack-pdvs", "false"));
		initAssocParam(cfg, url);

		initTLS(cfg);
		// Only used by query
		initKeys(cfg);
		// Only used by move
		dest = cfg.getProperty("dest");
	}

	public static Vector<PatientRecord> getPatients(int lookBack) throws Exception {
		final int tagPatientIdKey = 1048608;

		Hashtable<String, PatientRecord> patients = new Hashtable<String, PatientRecord>();

		DCMConfigProperties properties = new DCMConfigProperties(StorageService.class.getResource("/resources/DCMAccessManager.cfg"));

		if (lookBack > 0) {
			properties.put("key.StudyDate", ""+lookBack);
		}

		DcmURL url = new DcmURL(new ApplicationEntity().toString());

		DCMAccessManager pacsAccessManager = new DCMAccessManager(properties, url);

		boolean isOpen = pacsAccessManager.Connect();
		if (!(isOpen))
			throw new ConnectException("PatientInformation: unable to connect to PACS @ " + url);

		Vector<Dataset> datasetVector;

		datasetVector = pacsAccessManager.query();

		long numberofStudies = datasetVector.size();
		// System.out.println(numberofStudies);
		for (int dataSetCount = 0; dataSetCount < numberofStudies; ++dataSetCount) {

			Dataset dataSet = (Dataset) datasetVector.elementAt(dataSetCount);
			String patientID = dataSet.getString(tagPatientIdKey).trim();
			try {
				if (!patients.containsKey(patientID)) {
					patients.put(patientID, new PatientRecord(dataSet));
				}
			} catch (Exception exx) {
				Logger log = Logger.getLogger(DCMAccessManager.class.getSimpleName());
				log.log(Level.INFO, "Error occured for patient ID " + patientID + " record number " + dataSetCount);
				exx.printStackTrace();
			}

		}

		pacsAccessManager.Disconnect(true);
		Vector<PatientRecord> result = new Vector<PatientRecord>(patients.values());
		return result;
	}

	public static PatientRecord getPatient(String patientID) throws Exception {
		final int tagPatientIdKey = 1048608;
		PatientRecord patients = null;

		DCMConfigProperties properties = new DCMConfigProperties(StorageService.class.getResource("/resources/DCMAccessManager.cfg"));

		properties.put("key.PatientID", patientID.trim());

		DcmURL url = new DcmURL(new ApplicationEntity().toString());

		DCMAccessManager pacsAccessManager = new DCMAccessManager(properties, url);

		boolean isOpen = pacsAccessManager.Connect();
		if (!(isOpen))
			throw new ConnectException("PatientInformation: unable to connect to PACS @ " + url);

		Vector<Dataset> datasetVector;

		datasetVector = pacsAccessManager.query();

		long numberofStudies = datasetVector.size();
		// System.out.println("patinet id = "+patientID+"\t"+numberofStudies);
		for (int dataSetCount = 0; dataSetCount < numberofStudies; ++dataSetCount) {

			Dataset dataSet = (Dataset) datasetVector.elementAt(dataSetCount);
			String dspatientID = dataSet.getString(tagPatientIdKey).trim();
			// System.out.println(dspatientID+"\t"+patientID);
			try {
				if (dspatientID.equalsIgnoreCase(patientID)) {
					patients = new PatientRecord(dataSet);
					break;
				}
			} catch (Exception exx) {
				Logger log = Logger.getLogger(DCMAccessManager.class.getSimpleName());
				log.log(Level.INFO, "Error occured for patient ID " + patientID + " record number " + dataSetCount);
				exx.printStackTrace();
			}

		}

		pacsAccessManager.Disconnect(true);
		return patients;
	}

	public static Vector<StudyRecord> getPatientStudies(String patientID, int lookBack) throws Exception {

		Hashtable<String, StudyRecord> studies = new Hashtable<String, StudyRecord>();

		DCMConfigProperties properties = new DCMConfigProperties(StorageService.class.getResource("/resources/DCMAccessManager.cfg"));

		properties.put("key.PatientID", patientID);
		if (lookBack > 0) {
			properties.put("key.StudyDate", ""+lookBack);
		}

		DcmURL url = new DcmURL(new ApplicationEntity().toString());

		DCMAccessManager pacsAccessManager = new DCMAccessManager(properties, url);

		boolean isOpen = pacsAccessManager.Connect();
		if (!(isOpen))
			throw new ConnectException("PatientInformation: unable to connect to PACS @ " + url);

		Vector<Dataset> datasetVector;

		datasetVector = pacsAccessManager.query();

		long numberofStudies = datasetVector.size();
		// System.out.println(numberofStudies);
		for (int dataSetCount = 0; dataSetCount < numberofStudies; ++dataSetCount) {

			Dataset dataSet = (Dataset) datasetVector.elementAt(dataSetCount);
			StudyRecord record = new StudyRecord(dataSet);
			String sid = record.getStudyInstanceUID();
			try {
				if (!studies.containsKey(sid)) {
					studies.put(sid, record);
				}
			} catch (Exception exx) {
				Logger log = Logger.getLogger(DCMAccessManager.class.getSimpleName());
				log.log(Level.INFO, "Error occured for Study SOP ID " + sid + " record number " + dataSetCount);
				exx.printStackTrace();
			}

		}

		pacsAccessManager.Disconnect(true);
		Vector<StudyRecord> result = new Vector<StudyRecord>(studies.values());
		return result;
	}

	public static Vector<InstanceRecord> getStudyInstances(String studyID, int lookBack) throws Exception {

		Hashtable<String, InstanceRecord> instances = new Hashtable<String, InstanceRecord>();

		DCMConfigProperties properties = new DCMConfigProperties(StorageService.class.getResource("/resources/Image.cfg"));

		properties.put("key.StudyInstanceUID", studyID);

		if (lookBack > 0) {
			properties.put("key.StudyDate", ""+lookBack);
		}

		DcmURL url = new DcmURL(new ApplicationEntity().toString());

		DCMAccessManager pacsAccessManager = new DCMAccessManager(properties, url);

		boolean isOpen = pacsAccessManager.Connect();
		if (!(isOpen))
			throw new ConnectException("PatientInformation: unable to connect to PACS @ " + url);

		Vector<Dataset> datasetVector;

		datasetVector = pacsAccessManager.query();

		long numberofStudies = datasetVector.size();
		// System.out.println(numberofStudies);
		for (int dataSetCount = 0; dataSetCount < numberofStudies; ++dataSetCount) {

			Dataset dataSet = (Dataset) datasetVector.elementAt(dataSetCount);
			InstanceRecord record = new InstanceRecord(dataSet);
			String sid = record.getSopIuid();
			try {
				if (!instances.containsKey(sid)) {
					instances.put(sid, record);
				}
			} catch (Exception exx) {
				Logger log = Logger.getLogger(DCMAccessManager.class.getSimpleName());
				log.log(Level.INFO, "Error occured for Study SOP ID " + sid + " record number " + dataSetCount);
				exx.printStackTrace();
			}

		}

		pacsAccessManager.Disconnect(true);
		Vector<InstanceRecord> result = new Vector<InstanceRecord>(instances.values());
		return result;
	}

	public static String getInstanceWADOUrl(String studyID, String seriesID, String instanceID) throws Exception {

		// ServerXmlConfiguration sxc = new ServerXmlConfiguration();
		PACSConfiguration sxc = new PACSConfiguration();
		ServerConfiguration sc = sxc.getElementValues();
		String imageURL = "http://" + sc.getHostName() + ":" + sc.getWadoPort() + "/wado?requestType=WADO&contentType=application/dicom&studyUID=" + studyID + "&seriesUID="
				+ seriesID + "&objectUID=" + instanceID;
		return imageURL;

	}

	public static String getImageWADOUrl(String studyID, String seriesID, String instanceID, String imageNo) throws Exception {

		// ServerXmlConfiguration sxc = new ServerXmlConfiguration();
		PACSConfiguration sxc = new PACSConfiguration();
		ServerConfiguration sc = sxc.getElementValues();
		String imageURL = "http://" + sc.getHostName() + ":" + sc.getWadoPort() + "/wado?requestType=WADO&studyUID=" + studyID + "&seriesUID=" + seriesID + "&objectUID="
				+ instanceID + "&frameNumber=" + imageNo;
		return imageURL;

	}

	public static JSONObject storeDataSets(ArrayList<File> datasets) throws Exception {
		JSONObject result = new JSONObject();
		DcmURL url = new DcmURL(new ApplicationEntity().toString());

		DcmSnd dcmsnd = new org.dcm4che2.tool.dcmsnd.DcmSnd();

		dcmsnd.setCalledAET(url.getCalledAET());
		dcmsnd.setRemoteHost(url.getHost());
		dcmsnd.setRemotePort(url.getPort());
		dcmsnd.setOfferDefaultTransferSyntaxInSeparatePresentationContext(false);
		//dcmsnd.setStorageCommitment(true);
		dcmsnd.setPackPDV(true);
		dcmsnd.setTcpNoDelay(true);
		for (File ds : datasets)
			dcmsnd.addFile(ds);
		dcmsnd.configureTransferCapability();
		try {
			dcmsnd.start();
			dcmsnd.open();
			dcmsnd.send();
/*			if (dcmsnd.isStorageCommitment()) {
				if (dcmsnd.commit()) {
					result.put("commit", "success");
				}else{
					result.put("commit", "failed");
				}
			}*/
			result.put("commit", "success");
			dcmsnd.close();
		} finally {
			dcmsnd.stop();
		}
		
		return result;
	}

}
