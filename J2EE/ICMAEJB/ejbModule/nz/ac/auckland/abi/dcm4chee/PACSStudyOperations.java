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

import java.io.File;
import java.util.Properties;

import javax.management.MBeanServerConnection;
import javax.management.ObjectName;
import javax.naming.Context;
import javax.naming.InitialContext;
import javax.security.auth.login.AppConfigurationEntry;
import javax.security.auth.login.Configuration;
import javax.security.auth.login.LoginContext;

import org.dcm4che2.tool.dcmsnd.DcmSnd;

//Requires JBOSSALL-client.jar to work

public class PACSStudyOperations {
	PACSCallBackHandler pacsHandler = null;
	Configuration conf = null;
	Properties env = null;
	//private ServerXmlConfiguration sxc;
	private PACSConfiguration sxc;
	private ServerConfiguration serverConfiguration;
	
	public PACSStudyOperations(String username, String password) throws Exception{
		sxc = new PACSConfiguration();		
		serverConfiguration = sxc.getElementValues();
		
		env = new Properties();
		env.setProperty(Context.INITIAL_CONTEXT_FACTORY,
				"org.jnp.interfaces.NamingContextFactory");
		env.setProperty(Context.URL_PKG_PREFIXES,
				"org.jnp.interfaces.NamingContextFactory");

		env.setProperty(Context.PROVIDER_URL, "jnp://"+serverConfiguration.getHostName()+":1099/");
		pacsHandler = new PACSCallBackHandler(username, password);

		conf = new Configuration() {
			@Override
			public AppConfigurationEntry[] getAppConfigurationEntry(String name) {
				@SuppressWarnings({ "unchecked", "rawtypes" })
				AppConfigurationEntry entry = new AppConfigurationEntry(
						"org.jboss.security.ClientLoginModule",
						AppConfigurationEntry.LoginModuleControlFlag.REQUIRED,
						new java.util.HashMap());
				return new AppConfigurationEntry[] { entry };
			}

		};

	}

	public void deleteInstance(String instanceUID) throws Exception {
		Context context = new InitialContext();
		String icmaaetitle = (String)context.lookup("java:global/ICMAAETITLE");
		LoginContext lctx = new LoginContext(icmaaetitle, null, pacsHandler, conf);
		lctx.login();
		InitialContext ctx = new InitialContext(env);
		MBeanServerConnection mconn = (MBeanServerConnection) ctx
				.lookup("jmx/invoker/RMIAdaptor");
		ObjectName contentEditName = new ObjectName(
				"dcm4chee.archive:service=ContentEditService");
		String[] inst = { instanceUID };
		// mconn.invoke(contentEditName, "moveInstanceToTrash", new Object[] {
		// instanceUID }, new String[] { String.class.getName() });
		mconn.invoke(contentEditName, "moveInstancesToTrash",
				new Object[] { inst },
				new String[] { String[].class.getName() });
		mconn.invoke(contentEditName, "emptyTrash", null, null);
		lctx.logout();
		// System.out.println("Successfully deleted image "+instanceUID);
	}


	public static void dcmSND(String filename) throws Exception {
		//ServerXmlConfiguration sxc = new ServerXmlConfiguration();
		Context context = new InitialContext();
		String icmaaetitle = (String)context.lookup("java:global/ICMAAETITLE");
		PACSConfiguration sxc = new PACSConfiguration();
		ServerConfiguration sc = sxc.getElementValues();
		DcmSnd dcmsnd = new DcmSnd(icmaaetitle); // Register the calling AE title
		dcmsnd.setCalledAET(sc.getAeTitle());
		dcmsnd.setRemoteHost(sc.getHostName());
		dcmsnd.setRemotePort(Integer.parseInt((sc.getPort())));
		dcmsnd.setOfferDefaultTransferSyntaxInSeparatePresentationContext(false);
		dcmsnd.setSendFileRef(false);

		// Username password etc
		/*
		 * if (cl.hasOption("username")) { String username =
		 * cl.getOptionValue("username"); UserIdentity userId; UserIdentity
		 * userId; if (cl.hasOption("passcode")) { String passcode =
		 * cl.getOptionValue("passcode"); userId = new
		 * UserIdentity.UsernamePasscode(username, passcode.toCharArray()); }
		 * else { userId = new UserIdentity.Username(username); }
		 * userId.setPositiveResponseRequested(cl.hasOption("uidnegrsp"));
		 * dcmsnd.setUserIdentity(userId); }
		 */

		dcmsnd.setStorageCommitment(false);
		dcmsnd.setPackPDV(true);
		dcmsnd.setTcpNoDelay(true);
		// Setup the file
		dcmsnd.addFile(new File(filename));
		dcmsnd.configureTransferCapability();
		try {
			// Start the sending process
			dcmsnd.start();
			// Open connections
			dcmsnd.open();
			dcmsnd.send();
			dcmsnd.close();
		} catch (Exception exx) {
			throw exx;
		} finally {
			dcmsnd.stop();
		}
	}

	// http://community.jboss.org/thread/52375?decorator=print&displayFullThread=true
	public static void main(String[] args) throws Exception {
		PACSStudyOperations pacs = new PACSStudyOperations("admin", "admin");
		pacs.deleteInstance("1.2.826.0.1.3680043.2.1125.1.73552776556877376081276676778187495");
		// read in a dicom file
/*		DataInputStream in = new DataInputStream(new BufferedInputStream(
				new FileInputStream(
						"/people/rjag008/Desktop/Testdir/TestCMISS.dcm")));
		Dataset ds = DcmObjectFactory.getInstance().newDataset();
		try {
			ds.readFile(in, null, -1);
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			in.close();
		}
		System.out.println("Creating...");
		System.out.println(ds.getString(Tags.PatientID));
		System.out.println(ds.getString(Tags.SOPInstanceUID));
		// pacs.deleteInstance(ds.getString(Tags.SOPInstanceUID));
		pacs.dcmSND("/people/rjag008/Desktop/Testdir/TestCMISS.dcm");*/

	}
}
