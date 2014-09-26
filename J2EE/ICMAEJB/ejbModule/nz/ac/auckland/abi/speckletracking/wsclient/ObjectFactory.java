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

package nz.ac.auckland.abi.speckletracking.wsclient;

import javax.xml.bind.JAXBElement;
import javax.xml.bind.annotation.XmlElementDecl;
import javax.xml.bind.annotation.XmlRegistry;
import javax.xml.namespace.QName;


/**
 * This object contains factory methods for each 
 * Java content interface and Java element interface 
 * generated in the nz.ac.auckland.abi.speckletracking.wsclient package. 
 * <p>An ObjectFactory allows you to programatically 
 * construct new instances of the Java representation 
 * for XML content. The Java representation of XML 
 * content can consist of schema derived interfaces 
 * and classes representing the binding of schema 
 * type definitions, element declarations and model 
 * groups.  Factory methods for each of these are 
 * provided in this class.
 * 
 */
@XmlRegistry
public class ObjectFactory {

    private final static QName _GetSpeckleTrackingXMLResponse_QNAME = new QName("www.abi.auckland.ac.nz/CCF", "getSpeckleTrackingXMLResponse");
    private final static QName _GetDicom_QNAME = new QName("www.abi.auckland.ac.nz/CCF", "getDicom");
    private final static QName _CleanUpResponse_QNAME = new QName("www.abi.auckland.ac.nz/CCF", "cleanUpResponse");
    private final static QName _GetImageArchive_QNAME = new QName("www.abi.auckland.ac.nz/CCF", "getImageArchive");
    private final static QName _GetImageArchiveResponse_QNAME = new QName("www.abi.auckland.ac.nz/CCF", "getImageArchiveResponse");
    private final static QName _GetCompletion_QNAME = new QName("www.abi.auckland.ac.nz/CCF", "getCompletion");
    private final static QName _SubmitResponse_QNAME = new QName("www.abi.auckland.ac.nz/CCF", "submitResponse");
    private final static QName _GetCompletionResponse_QNAME = new QName("www.abi.auckland.ac.nz/CCF", "getCompletionResponse");
    private final static QName _SubmitWithDisplacementsResponse_QNAME = new QName("www.abi.auckland.ac.nz/CCF", "submitWithDisplacementsResponse");
    private final static QName _Submit_QNAME = new QName("www.abi.auckland.ac.nz/CCF", "submit");
    private final static QName _SubmitWithDisplacements_QNAME = new QName("www.abi.auckland.ac.nz/CCF", "submitWithDisplacements");
    private final static QName _CleanUp_QNAME = new QName("www.abi.auckland.ac.nz/CCF", "cleanUp");
    private final static QName _GetDicomResponse_QNAME = new QName("www.abi.auckland.ac.nz/CCF", "getDicomResponse");
    private final static QName _GetSpeckleTrackingXML_QNAME = new QName("www.abi.auckland.ac.nz/CCF", "getSpeckleTrackingXML");

    /**
     * Create a new ObjectFactory that can be used to create new instances of schema derived classes for package: nz.ac.auckland.abi.speckletracking.wsclient
     * 
     */
    public ObjectFactory() {
    }

    /**
     * Create an instance of {@link GetSpeckleTrackingXMLResponse }
     * 
     */
    public GetSpeckleTrackingXMLResponse createGetSpeckleTrackingXMLResponse() {
        return new GetSpeckleTrackingXMLResponse();
    }

    /**
     * Create an instance of {@link GetDicom }
     * 
     */
    public GetDicom createGetDicom() {
        return new GetDicom();
    }

    /**
     * Create an instance of {@link Submit }
     * 
     */
    public Submit createSubmit() {
        return new Submit();
    }

    /**
     * Create an instance of {@link CleanUpResponse }
     * 
     */
    public CleanUpResponse createCleanUpResponse() {
        return new CleanUpResponse();
    }

    /**
     * Create an instance of {@link SubmitWithDisplacementsResponse }
     * 
     */
    public SubmitWithDisplacementsResponse createSubmitWithDisplacementsResponse() {
        return new SubmitWithDisplacementsResponse();
    }

    /**
     * Create an instance of {@link GetImageArchive }
     * 
     */
    public GetImageArchive createGetImageArchive() {
        return new GetImageArchive();
    }

    /**
     * Create an instance of {@link SubmitWithDisplacements }
     * 
     */
    public SubmitWithDisplacements createSubmitWithDisplacements() {
        return new SubmitWithDisplacements();
    }

    /**
     * Create an instance of {@link GetImageArchiveResponse }
     * 
     */
    public GetImageArchiveResponse createGetImageArchiveResponse() {
        return new GetImageArchiveResponse();
    }

    /**
     * Create an instance of {@link SubmitResponse }
     * 
     */
    public SubmitResponse createSubmitResponse() {
        return new SubmitResponse();
    }

    /**
     * Create an instance of {@link GetCompletion }
     * 
     */
    public GetCompletion createGetCompletion() {
        return new GetCompletion();
    }

    /**
     * Create an instance of {@link GetCompletionResponse }
     * 
     */
    public GetCompletionResponse createGetCompletionResponse() {
        return new GetCompletionResponse();
    }

    /**
     * Create an instance of {@link GetSpeckleTrackingXML }
     * 
     */
    public GetSpeckleTrackingXML createGetSpeckleTrackingXML() {
        return new GetSpeckleTrackingXML();
    }

    /**
     * Create an instance of {@link GetDicomResponse }
     * 
     */
    public GetDicomResponse createGetDicomResponse() {
        return new GetDicomResponse();
    }

    /**
     * Create an instance of {@link CleanUp }
     * 
     */
    public CleanUp createCleanUp() {
        return new CleanUp();
    }

    /**
     * Create an instance of {@link ArrayList }
     * 
     */
    public ArrayList createArrayList() {
        return new ArrayList();
    }

    /**
     * Create an instance of {@link JAXBElement }{@code <}{@link GetSpeckleTrackingXMLResponse }{@code >}}
     * 
     */
    @XmlElementDecl(namespace = "www.abi.auckland.ac.nz/CCF", name = "getSpeckleTrackingXMLResponse")
    public JAXBElement<GetSpeckleTrackingXMLResponse> createGetSpeckleTrackingXMLResponse(GetSpeckleTrackingXMLResponse value) {
        return new JAXBElement<GetSpeckleTrackingXMLResponse>(_GetSpeckleTrackingXMLResponse_QNAME, GetSpeckleTrackingXMLResponse.class, null, value);
    }

    /**
     * Create an instance of {@link JAXBElement }{@code <}{@link GetDicom }{@code >}}
     * 
     */
    @XmlElementDecl(namespace = "www.abi.auckland.ac.nz/CCF", name = "getDicom")
    public JAXBElement<GetDicom> createGetDicom(GetDicom value) {
        return new JAXBElement<GetDicom>(_GetDicom_QNAME, GetDicom.class, null, value);
    }

    /**
     * Create an instance of {@link JAXBElement }{@code <}{@link CleanUpResponse }{@code >}}
     * 
     */
    @XmlElementDecl(namespace = "www.abi.auckland.ac.nz/CCF", name = "cleanUpResponse")
    public JAXBElement<CleanUpResponse> createCleanUpResponse(CleanUpResponse value) {
        return new JAXBElement<CleanUpResponse>(_CleanUpResponse_QNAME, CleanUpResponse.class, null, value);
    }

    /**
     * Create an instance of {@link JAXBElement }{@code <}{@link GetImageArchive }{@code >}}
     * 
     */
    @XmlElementDecl(namespace = "www.abi.auckland.ac.nz/CCF", name = "getImageArchive")
    public JAXBElement<GetImageArchive> createGetImageArchive(GetImageArchive value) {
        return new JAXBElement<GetImageArchive>(_GetImageArchive_QNAME, GetImageArchive.class, null, value);
    }

    /**
     * Create an instance of {@link JAXBElement }{@code <}{@link GetImageArchiveResponse }{@code >}}
     * 
     */
    @XmlElementDecl(namespace = "www.abi.auckland.ac.nz/CCF", name = "getImageArchiveResponse")
    public JAXBElement<GetImageArchiveResponse> createGetImageArchiveResponse(GetImageArchiveResponse value) {
        return new JAXBElement<GetImageArchiveResponse>(_GetImageArchiveResponse_QNAME, GetImageArchiveResponse.class, null, value);
    }

    /**
     * Create an instance of {@link JAXBElement }{@code <}{@link GetCompletion }{@code >}}
     * 
     */
    @XmlElementDecl(namespace = "www.abi.auckland.ac.nz/CCF", name = "getCompletion")
    public JAXBElement<GetCompletion> createGetCompletion(GetCompletion value) {
        return new JAXBElement<GetCompletion>(_GetCompletion_QNAME, GetCompletion.class, null, value);
    }

    /**
     * Create an instance of {@link JAXBElement }{@code <}{@link SubmitResponse }{@code >}}
     * 
     */
    @XmlElementDecl(namespace = "www.abi.auckland.ac.nz/CCF", name = "submitResponse")
    public JAXBElement<SubmitResponse> createSubmitResponse(SubmitResponse value) {
        return new JAXBElement<SubmitResponse>(_SubmitResponse_QNAME, SubmitResponse.class, null, value);
    }

    /**
     * Create an instance of {@link JAXBElement }{@code <}{@link GetCompletionResponse }{@code >}}
     * 
     */
    @XmlElementDecl(namespace = "www.abi.auckland.ac.nz/CCF", name = "getCompletionResponse")
    public JAXBElement<GetCompletionResponse> createGetCompletionResponse(GetCompletionResponse value) {
        return new JAXBElement<GetCompletionResponse>(_GetCompletionResponse_QNAME, GetCompletionResponse.class, null, value);
    }

    /**
     * Create an instance of {@link JAXBElement }{@code <}{@link SubmitWithDisplacementsResponse }{@code >}}
     * 
     */
    @XmlElementDecl(namespace = "www.abi.auckland.ac.nz/CCF", name = "submitWithDisplacementsResponse")
    public JAXBElement<SubmitWithDisplacementsResponse> createSubmitWithDisplacementsResponse(SubmitWithDisplacementsResponse value) {
        return new JAXBElement<SubmitWithDisplacementsResponse>(_SubmitWithDisplacementsResponse_QNAME, SubmitWithDisplacementsResponse.class, null, value);
    }

    /**
     * Create an instance of {@link JAXBElement }{@code <}{@link Submit }{@code >}}
     * 
     */
    @XmlElementDecl(namespace = "www.abi.auckland.ac.nz/CCF", name = "submit")
    public JAXBElement<Submit> createSubmit(Submit value) {
        return new JAXBElement<Submit>(_Submit_QNAME, Submit.class, null, value);
    }

    /**
     * Create an instance of {@link JAXBElement }{@code <}{@link SubmitWithDisplacements }{@code >}}
     * 
     */
    @XmlElementDecl(namespace = "www.abi.auckland.ac.nz/CCF", name = "submitWithDisplacements")
    public JAXBElement<SubmitWithDisplacements> createSubmitWithDisplacements(SubmitWithDisplacements value) {
        return new JAXBElement<SubmitWithDisplacements>(_SubmitWithDisplacements_QNAME, SubmitWithDisplacements.class, null, value);
    }

    /**
     * Create an instance of {@link JAXBElement }{@code <}{@link CleanUp }{@code >}}
     * 
     */
    @XmlElementDecl(namespace = "www.abi.auckland.ac.nz/CCF", name = "cleanUp")
    public JAXBElement<CleanUp> createCleanUp(CleanUp value) {
        return new JAXBElement<CleanUp>(_CleanUp_QNAME, CleanUp.class, null, value);
    }

    /**
     * Create an instance of {@link JAXBElement }{@code <}{@link GetDicomResponse }{@code >}}
     * 
     */
    @XmlElementDecl(namespace = "www.abi.auckland.ac.nz/CCF", name = "getDicomResponse")
    public JAXBElement<GetDicomResponse> createGetDicomResponse(GetDicomResponse value) {
        return new JAXBElement<GetDicomResponse>(_GetDicomResponse_QNAME, GetDicomResponse.class, null, value);
    }

    /**
     * Create an instance of {@link JAXBElement }{@code <}{@link GetSpeckleTrackingXML }{@code >}}
     * 
     */
    @XmlElementDecl(namespace = "www.abi.auckland.ac.nz/CCF", name = "getSpeckleTrackingXML")
    public JAXBElement<GetSpeckleTrackingXML> createGetSpeckleTrackingXML(GetSpeckleTrackingXML value) {
        return new JAXBElement<GetSpeckleTrackingXML>(_GetSpeckleTrackingXML_QNAME, GetSpeckleTrackingXML.class, null, value);
    }

}
