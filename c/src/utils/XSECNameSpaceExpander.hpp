/*
 * The Apache Software License, Version 1.1
 *
 *
 * Copyright (c) 1999 The Apache Software Foundation.  All rights 
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:  
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "<WebSig>" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written 
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation and was
 * originally based on software copyright (c) 2001, Institute for
 * Data Communications Systems, <http://www.nue.et-inf.uni-siegen.de/>.
 * The development of this software was partly funded by the European 
 * Commission in the <WebSig> project in the ISIS Programme. 
 * For more information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

/*
 * XSEC
 *
 * XSECNameSpaceHolder := Container class for holding and managing the name space stack
 *						  Used when running through a DOM document
 *
 * Author(s): Berin Lautenbach
 *
 * $ID$
 *
 * $LOG$
 *
 */

#ifndef XSECNAMESPACEEXPANDER_HEADER
#define XSECNAMESPACEEXPANDER_HEADER

// XSEC Includes
#include <xsec/framework/XSECDefs.hpp>
#include <xsec/utils/XSECSafeBuffer.hpp>
#include <xsec/utils/XSECSafeBufferFormatter.hpp>

// Xerces Includes
XSEC_DECLARE_XERCES_CLASS(DOMDocument);
XSEC_DECLARE_XERCES_CLASS(DOMNode);
XSEC_DECLARE_XERCES_CLASS(DOMElement);

#include <vector>

// --------------------------------------------------------------------------------
//           Structure Definition for the nodes within the list of nodes
// --------------------------------------------------------------------------------

struct XSECNameSpaceEntry {

	// Variables
	safeBuffer							m_name;			// The name for this name space
	DOMElement							* mp_node;		// The Element Node owner
	DOMNode								* mp_att;		// The added attribute node
			
};

// --------------------------------------------------------------------------------
//           Class definition for the list
// --------------------------------------------------------------------------------

class CANON_EXPORT XSECNameSpaceExpander {


#if defined(XALAN_NO_NAMESPACES)
	typedef vector<XSECNameSpaceEntry *>			NameSpaceEntryListVectorType;
#else
	typedef std::vector<XSECNameSpaceEntry *>		NameSpaceEntryListVectorType;
#endif


public:

	XSECNameSpaceExpander(DOMDocument *d);			// Constructor
	~XSECNameSpaceExpander();						// Default destructor

	// Operate 
	void expandNameSpaces(void);
	void deleteAddedNamespaces(void);

	// Check if a node is an added node
	bool nodeWasAdded(DOMNode *n);

private:  // Functions

	XSECNameSpaceExpander(void);					// No default constructor
	void recurse(DOMElement *n);

	// data
	
	NameSpaceEntryListVectorType	m_lst;			// List of added name spaces
	DOMDocument						* mp_doc;		// The owner document
	bool							m_expanded;		// Have we expanded already?
	XSECSafeBufferFormatter			* mp_formatter;

};

#endif /* XSECNAMESPACEEXPANDER_HEADER */