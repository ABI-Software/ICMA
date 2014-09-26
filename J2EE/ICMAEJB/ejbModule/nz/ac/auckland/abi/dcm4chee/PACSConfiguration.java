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

import java.util.logging.Level;
import java.util.logging.Logger;

import javax.naming.Context;
import javax.naming.InitialContext;

public class PACSConfiguration {
	private static String aetitle = null;
	
	private static String hostname = null;
	
	private static String port = null;
	
	private static String wadoPort = null;
	
	private static String dcmprotocol = null;
	
	
	public PACSConfiguration() throws Exception{
		if(aetitle==null){
			//Use JNDI to get the deployment values as this class is a POJO
			Context context = new InitialContext();
			aetitle = (String)context.lookup("java:global/PACSAETITLE");
			hostname = (String)context.lookup("java:global/PACSHOSTNAME");
			port = (String)context.lookup("java:global/PACSPORT");
			wadoPort = (String)context.lookup("java:global/PACSWADOPORT");
			dcmprotocol = (String)context.lookup("java:global/PACSDCMPROTOCOL");
			
			Logger log = Logger.getLogger(DCMAccessManager.class.getSimpleName());
			log.log(Level.INFO,"Configuring PACS with "+aetitle+"\t"+hostname+"\t"+port+"\t"+wadoPort+"\t"+dcmprotocol);
		}
	}
	
	/**
	 * Getter for ServerConfiguration object.It reads the element values
	 * from the xml document.Creates a new ServerConfiguration object, sets
	 * the properties of serverConfiguration and returns the serverConfiguration.
	 * @return The ServerConfiguration object.
	 */
	public ServerConfiguration getElementValues(){
		ServerConfiguration serverConfiguration = new ServerConfiguration();
		serverConfiguration.setAeTitle(aetitle);
		serverConfiguration.setHostName(hostname);
		serverConfiguration.setPort(port);
		serverConfiguration.setWadoPort(wadoPort);
		serverConfiguration.setDcmProtocol(dcmprotocol);
		return serverConfiguration;
	}
}
