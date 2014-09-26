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
package nz.ac.auckland.abi.webapp.consultant;

import java.io.Closeable;
import java.io.IOException;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Calendar;
import java.util.GregorianCalendar;
import java.util.List;
import java.util.zip.GZIPOutputStream;

import javax.ejb.EJB;
import javax.servlet.ServletException;
import javax.servlet.ServletOutputStream;
import javax.servlet.annotation.WebServlet;
import javax.servlet.http.Cookie;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import nz.ac.auckland.abi.icmaconfiguration.CMSContent;
import nz.ac.auckland.abi.icmaconfiguration.ResourceConfigurationManager;

/**
 * Servlet implementation class ICMAUSERDOCS
 */
@WebServlet(description = "Content delivary", urlPatterns = { "/ICMAWORKFLOWDOCS/*" })
public class ICMAWORKFLOWDOCS extends HttpServlet {
	private static final long serialVersionUID = 1L;
    
	private static final int DEFAULT_BUFFER_SIZE = 10240; // ..bytes = 10KB.
    private static final long DEFAULT_EXPIRE_TIME = 604800000L; // ..ms = 1 week.
    private static final String MULTIPART_BOUNDARY = "MULTIPART_BYTERANGES";
	
	@EJB
	ResourceConfigurationManager resourceManager;
	
    /**
     * @see HttpServlet#HttpServlet()
     */
    public ICMAWORKFLOWDOCS() {
        super();
    }
    
    /**
     * Process HEAD request. This returns the same headers as GET request, but without content.
     * @see HttpServlet#doHead(HttpServletRequest, HttpServletResponse).
     */
    protected void doHead(HttpServletRequest request, HttpServletResponse response)
        throws ServletException, IOException
    {
        // Process request without content.
        processRequest(request, response, false);
    }

	/**
	 * @see HttpServlet#doGet(HttpServletRequest request, HttpServletResponse response)
	 */
	protected void doGet(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
		processRequest(request, response, true);
	}

	
	private void processRequest(HttpServletRequest request, HttpServletResponse response, boolean content)   throws IOException{
		String uri = request.getRequestURI(); // has application name/ICMAUSERDOCS/request
		String nodepath = uri.substring(uri.indexOf("/ICMAWORKFLOWDOCS")); //Remove /ICMAUSERDOCS/
		try{
			String filename = uri.substring(uri.lastIndexOf("/"));
			CMSContent myContent = resourceManager.getCMSNodeAt(nodepath);
			
			Calendar cal = new GregorianCalendar(2013, 01, 01); //Since content is static
		    int length = myContent.contentLength;
		    long lastModified = cal.getTimeInMillis();
		    String eTag = filename + "_" + length + "_" + lastModified;
			
		    // Validate request headers for caching ---------------------------------------------------

	        // If-None-Match header should contain "*" or ETag. If so, then return 304.
	        String ifNoneMatch = request.getHeader("If-None-Match");
	        if (ifNoneMatch != null && matches(ifNoneMatch, eTag)) {
	            response.setHeader("ETag", eTag); // Required in 304.
	            response.sendError(HttpServletResponse.SC_NOT_MODIFIED);
	            return;
	        }

	        // If-Modified-Since header should be greater than LastModified. If so, then return 304.
	        // This header is ignored if any If-None-Match header is specified.
	        long ifModifiedSince = request.getDateHeader("If-Modified-Since");
	        if (ifNoneMatch == null && ifModifiedSince != -1 && ifModifiedSince + 1000 > lastModified) {
	            response.setHeader("ETag", eTag); // Required in 304.
	            response.sendError(HttpServletResponse.SC_NOT_MODIFIED);
	            return;
	        }


	        // Validate request headers for resume ----------------------------------------------------

	        // If-Match header should contain "*" or ETag. If not, then return 412.
	        String ifMatch = request.getHeader("If-Match");
	        if (ifMatch != null && !matches(ifMatch, eTag)) {
	            response.sendError(HttpServletResponse.SC_PRECONDITION_FAILED);
	            return;
	        }

	        // If-Unmodified-Since header should be greater than LastModified. If not, then return 412.
	        long ifUnmodifiedSince = request.getDateHeader("If-Unmodified-Since");
	        if (ifUnmodifiedSince != -1 && ifUnmodifiedSince + 1000 <= lastModified) {
	            response.sendError(HttpServletResponse.SC_PRECONDITION_FAILED);
	            return;
	        }


	        // Validate and process range -------------------------------------------------------------

	        // Prepare some variables. The full Range represents the complete file.
	        Range full = new Range(0, length - 1, length);
	        List<Range> ranges = new ArrayList<Range>();

	        // Validate and process Range and If-Range headers.
	        String range = request.getHeader("Range");
	        if (range != null) {

	            // Range header should match format "bytes=n-n,n-n,n-n...". If not, then return 416.
	            if (!range.matches("^bytes=\\d*-\\d*(,\\d*-\\d*)*$")) {
	                response.setHeader("Content-Range", "bytes */" + length); // Required in 416.
	                response.sendError(HttpServletResponse.SC_REQUESTED_RANGE_NOT_SATISFIABLE);
	                return;
	            }

	            // If-Range header should either match ETag or be greater then LastModified. If not,
	            // then return full file.
	            String ifRange = request.getHeader("If-Range");
	            if (ifRange != null && !ifRange.equals(eTag)) {
	                try {
	                    long ifRangeTime = request.getDateHeader("If-Range"); // Throws IAE if invalid.
	                    if (ifRangeTime != -1 && ifRangeTime + 1000 < lastModified) {
	                        ranges.add(full);
	                    }
	                } catch (IllegalArgumentException ignore) {
	                    ranges.add(full);
	                }
	            }

	            // If any valid If-Range header, then process each part of byte range.
	            if (ranges.isEmpty()) {
	                for (String part : range.substring(6).split(",")) {
	                    // Assuming a file with length of 100, the following examples returns bytes at:
	                    // 50-80 (50 to 80), 40- (40 to length=100), -20 (length-20=80 to length=100).
	                    int start = sublong(part, 0, part.indexOf("-"));
	                    int end = sublong(part, part.indexOf("-") + 1, part.length());

	                    if (start == -1) {
	                        start = length - end;
	                        end = length - 1;
	                    } else if (end == -1 || end > length - 1) {
	                        end = length - 1;
	                    }

	                    // Check if Range is syntactically valid. If not, then return 416.
	                    if (start > end) {
	                        response.setHeader("Content-Range", "bytes */" + length); // Required in 416.
	                        response.sendError(HttpServletResponse.SC_REQUESTED_RANGE_NOT_SATISFIABLE);
	                        return;
	                    }

	                    // Add range.
	                    ranges.add(new Range(start, end, length));
	                }
	            }
	        }


	        // Prepare and initialize response --------------------------------------------------------

	        // Get content type by file name and set default GZIP support and content disposition.
	        String contentType = myContent.mimeType;
	        boolean acceptsGzip = false;
	        String disposition = "inline";

	        // If content type is unknown, then set the default value.
	        // For all content types, see: http://www.w3schools.com/media/media_mimeref.asp
	        // To add new content types, add new mime-mapping entry in web.xml.
	        if (contentType == null) {
	            contentType = "application/octet-stream";
	        }

	        // If content type is text, then determine whether GZIP content encoding is supported by
	        // the browser and expand content type with the one and right character encoding.
	        if (contentType.startsWith("text")) {
	            String acceptEncoding = request.getHeader("Accept-Encoding");
	            acceptsGzip = acceptEncoding != null && accepts(acceptEncoding, "gzip");
	            contentType += ";charset=UTF-8";
	        } 

	        // Else, expect for images, determine content disposition. If content type is supported by
	        // the browser, then set to inline, else attachment which will pop a 'save as' dialogue.
	        else if (!contentType.startsWith("image")) {
	            String accept = request.getHeader("Accept");
	            disposition = accept != null && accepts(accept, contentType) ? "inline" : "attachment";
	        }

	        // Initialize response.
	        response.reset();
	        response.setBufferSize(DEFAULT_BUFFER_SIZE);
	        response.setHeader("Content-Disposition", disposition + ";filename=\"" + filename + "\"");
	        response.setHeader("Accept-Ranges", "bytes");
	        response.setHeader("ETag", eTag);
	        response.setDateHeader("Last-Modified", lastModified);
	        response.setDateHeader("Expires", System.currentTimeMillis() + DEFAULT_EXPIRE_TIME);


	        response.addCookie(new Cookie("analysisfileDownload", "true"));
	        
	        // Send requested file (part(s)) to client ------------------------------------------------

	        // Prepare streams.
	        OutputStream output = null;

	        try {
	            // Open streams.
	            output = response.getOutputStream();

	            if (ranges.isEmpty() || ranges.get(0) == full) {

	                // Return full file.
	                Range r = full;
	                response.setContentType(contentType);
	                response.setHeader("Content-Range", "bytes " + r.start + "-" + r.end + "/" + r.total);

	                if (content) {
	                    if (acceptsGzip) {
	                        // The browser accepts GZIP, so GZIP the content.
	                        response.setHeader("Content-Encoding", "gzip");
	                        output = new GZIPOutputStream(output, DEFAULT_BUFFER_SIZE);
	                    } else {
	                        // Content length is not directly predictable in case of GZIP.
	                        // So only add it if there is no means of GZIP, else browser will hang.
	                        response.setHeader("Content-Length", String.valueOf(r.length));
	                    }

	                    // Copy full range.
	                    copy(myContent.output.toByteArray(), output, r.start, r.length);
	                }

	            } else if (ranges.size() == 1) {

	                // Return single part of file.
	                Range r = ranges.get(0);
	                response.setContentType(contentType);
	                response.setHeader("Content-Range", "bytes " + r.start + "-" + r.end + "/" + r.total);
	                response.setHeader("Content-Length", String.valueOf(r.length));
	                response.setStatus(HttpServletResponse.SC_PARTIAL_CONTENT); // 206.

	                if (content) {
	                    // Copy single part range.
	                    copy(myContent.output.toByteArray(), output, r.start, r.length);
	                }

	            } else {

	                // Return multiple parts of file.
	                response.setContentType("multipart/byteranges; boundary=" + MULTIPART_BOUNDARY);
	                response.setStatus(HttpServletResponse.SC_PARTIAL_CONTENT); // 206.

	                if (content) {
	                    // Cast back to ServletOutputStream to get the easy println methods.
	                    ServletOutputStream sos = (ServletOutputStream) output;

	                    // Copy multi part range.
	                    for (Range r : ranges) {
	                        // Add multipart boundary and header fields for every range.
	                        sos.println();
	                        sos.println("--" + MULTIPART_BOUNDARY);
	                        sos.println("Content-Type: " + contentType);
	                        sos.println("Content-Range: bytes " + r.start + "-" + r.end + "/" + r.total);

	                        // Copy single part range of multi part range.
	                        copy(myContent.output.toByteArray(), output, r.start, r.length);
	                    }

	                    // End with multipart boundary.
	                    sos.println();
	                    sos.println("--" + MULTIPART_BOUNDARY + "--");
	                }
	            }
	        } finally {
	            // Gently close streams.
	            close(output);
	        }
			
			
		}catch(Exception exx){
			String error = "Unable to process request: Exception "+exx+" occured for "+nodepath;
			
			
			response.getOutputStream().write(error.getBytes());
		}
	}

	  /**
     * Returns true if the given accept header accepts the given value.
     * @param acceptHeader The accept header.
     * @param toAccept The value to be accepted.
     * @return True if the given accept header accepts the given value.
     */
    private static boolean accepts(String acceptHeader, String toAccept) {
        String[] acceptValues = acceptHeader.split("\\s*(,|;)\\s*");
        Arrays.sort(acceptValues);
        return Arrays.binarySearch(acceptValues, toAccept) > -1
            || Arrays.binarySearch(acceptValues, toAccept.replaceAll("/.*$", "/*")) > -1
            || Arrays.binarySearch(acceptValues, "*/*") > -1;
    }

    /**
     * Returns true if the given match header matches the given value.
     * @param matchHeader The match header.
     * @param toMatch The value to be matched.
     * @return True if the given match header matches the given value.
     */
    private static boolean matches(String matchHeader, String toMatch) {
        String[] matchValues = matchHeader.split("\\s*,\\s*");
        Arrays.sort(matchValues);
        return Arrays.binarySearch(matchValues, toMatch) > -1
            || Arrays.binarySearch(matchValues, "*") > -1;
    }

    /**
     * Returns a substring of the given string value from the given begin index to the given end
     * index as a long. If the substring is empty, then -1 will be returned
     * @param value The string value to return a substring as long for.
     * @param beginIndex The begin index of the substring to be returned as long.
     * @param endIndex The end index of the substring to be returned as long.
     * @return A substring of the given string value as long or -1 if substring is empty.
     */
    private static int sublong(String value, int beginIndex, int endIndex) {
        String substring = value.substring(beginIndex, endIndex);
        return (substring.length() > 0) ? Integer.parseInt(substring) : -1;
    }

    /**
     * Copy the given byte range of the given input to the given output.
     * @param input The input to copy the given range to the given output for.
     * @param output The output to copy the given range from the given input for.
     * @param start Start of the byte range.
     * @param length Length of the byte range.
     * @throws IOException If something fails at I/O level.
     */
    private static void copy(byte[] input, OutputStream output, int start, int length)
        throws IOException
    {
        if (input.length == length) {
            // Write full range.
        	output.write(input);
        } else {
            // Write partial range.
        	output.write(input, start, length);
        }
    }

    /**
     * Close the given resource.
     * @param resource The resource to be closed.
     */
    private static void close(Closeable resource) {
        if (resource != null) {
            try {
                resource.close();
            } catch (IOException ignore) {
                // Ignore IOException. If you want to handle this anyway, it might be useful to know
                // that this will generally only be thrown when the client aborted the request.
            }
        }
    }

    // Inner classes ------------------------------------------------------------------------------

    /**
     * This class represents a byte range.
     */
    protected class Range {
        int start;
        int end;
        int length;
        int total;

        /**
         * Construct a byte range.
         * @param start Start of the byte range.
         * @param end End of the byte range.
         * @param total Total length of the byte source.
         */
        public Range(int start, int end, int total) {
            this.start = start;
            this.end = end;
            this.length = end - start + 1;
            this.total = total;
        }

    }

	
}
