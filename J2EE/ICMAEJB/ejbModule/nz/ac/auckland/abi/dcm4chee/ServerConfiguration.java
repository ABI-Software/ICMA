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

public class ServerConfiguration {

	private String aeTitle;
	private String hostName;
	private String port;
	private String wadoPort;
	private String dcmProtocol;
	
	public ServerConfiguration() {		
		aeTitle=new String();
		hostName=new String();
		port=new String();
		wadoPort=new String();
		dcmProtocol = new String();
	}

	/**
	 * @return the aeTitle
	 */
	public String getAeTitle() {
		return aeTitle;
	}

	/**
	 * @param aeTitle the aeTitle to set
	 */
	public void setAeTitle(String aeTitle) {
		this.aeTitle = aeTitle;
	}

	/**
	 * @return the hostName
	 */
	public String getHostName() {
		return hostName;
	}

	/**
	 * @param hostName the hostName to set
	 */
	public void setHostName(String hostName) {
		this.hostName = hostName;
	}

	/**
	 * @return the port
	 */
	public String getPort() {
		return port;
	}

	/**
	 * @param port the port to set
	 */
	public void setPort(String port) {
		this.port = port;
	}

	/**
	 * @return the wadoPort
	 */
	public String getWadoPort() {
		return wadoPort;
	}

	/**
	 * @param wadoPort the wadoPort to set
	 */
	public void setWadoPort(String wadoPort) {
		this.wadoPort = wadoPort;
	}
	
	/**
	 * 
	 * @return the dcmProtocol;
	 */
	public String getDcmProtocol(){
		return this.dcmProtocol;
	}
	
	/**
	 * 
	 * @param dcmProtocol the dcmProtocol to set
	 */
	public void setDcmProtocol(String dcmProtocol){
		this.dcmProtocol = dcmProtocol;
	}

}