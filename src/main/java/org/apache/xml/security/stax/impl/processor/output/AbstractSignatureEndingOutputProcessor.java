/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
package org.apache.xml.security.stax.impl.processor.output;

import org.apache.commons.codec.binary.Base64;
import org.apache.xml.security.exceptions.XMLSecurityException;
import org.apache.xml.security.stax.ext.*;
import org.apache.xml.security.stax.impl.transformer.canonicalizer.Canonicalizer20010315_Excl;
import org.apache.xml.security.stax.securityToken.SecurityTokenProvider;
import org.apache.xml.security.stax.ext.stax.XMLSecAttribute;
import org.apache.xml.security.stax.ext.stax.XMLSecEvent;
import org.apache.xml.security.stax.ext.stax.XMLSecStartElement;
import org.apache.xml.security.stax.impl.SignaturePartDef;
import org.apache.xml.security.stax.impl.algorithms.SignatureAlgorithm;
import org.apache.xml.security.stax.impl.algorithms.SignatureAlgorithmFactory;
import org.apache.xml.security.stax.securityToken.OutboundSecurityToken;
import org.apache.xml.security.stax.impl.util.IDGenerator;
import org.apache.xml.security.stax.impl.util.SignerOutputStream;
import org.apache.xml.security.stax.impl.util.UnsynchronizedBufferedOutputStream;

import javax.xml.stream.XMLStreamException;
import java.io.IOException;
import java.io.OutputStream;
import java.security.NoSuchAlgorithmException;
import java.security.NoSuchProviderException;
import java.util.*;

/**
 * @author $Author$
 * @version $Revision$ $Date$
 */
public abstract class AbstractSignatureEndingOutputProcessor extends AbstractBufferingOutputProcessor {

    private List<SignaturePartDef> signaturePartDefList;

    public AbstractSignatureEndingOutputProcessor(AbstractSignatureOutputProcessor signatureOutputProcessor)
            throws XMLSecurityException {
        super();
        signaturePartDefList = signatureOutputProcessor.getSignaturePartDefList();
    }

    /*
        <ds:Signature xmlns:ds="http://www.w3.org/2000/09/xmldsig#" Id="Signature-1022834285">
            <ds:SignedInfo>
                <ds:CanonicalizationMethod Algorithm="http://www.w3.org/2001/10/xml-exc-c14n#" />
                <ds:SignatureMethod Algorithm="http://www.w3.org/2000/09/xmldsig#rsa-sha1" />
                <ds:Reference URI="#id-1612925417">
                    <ds:Transforms>
                        <ds:Transform Algorithm="http://www.w3.org/2001/10/xml-exc-c14n#" />
                    </ds:Transforms>
                    <ds:DigestMethod Algorithm="http://www.w3.org/2000/09/xmldsig#sha1" />
                    <ds:DigestValue>cy/khx5N6UobCJ1EbX+qnrGID2U=</ds:DigestValue>
                </ds:Reference>
                <ds:Reference URI="#Timestamp-1106985890">
                    <ds:Transforms>
                        <ds:Transform Algorithm="http://www.w3.org/2001/10/xml-exc-c14n#" />
                    </ds:Transforms>
                    <ds:DigestMethod Algorithm="http://www.w3.org/2000/09/xmldsig#sha1" />
                    <ds:DigestValue>+p5YRII6uvUdsJ7XLKkWx1CBewE=</ds:DigestValue>
                </ds:Reference>
            </ds:SignedInfo>
            <ds:SignatureValue>
                Izg1FlI9oa4gOon2vTXi7V0EpiyCUazECVGYflbXq7/3GF8ThKGDMpush/fo1I2NVjEFTfmT2WP/
                +ZG5N2jASFptrcGbsqmuLE5JbxUP1TVKb9SigKYcOQJJ8klzmVfPXnSiRZmIU+DUT2UXopWnGNFL
                TwY0Uxja4ZuI6U8m8Tg=
            </ds:SignatureValue>
            <ds:KeyInfo Id="KeyId-1043455692">
                <wsse:SecurityTokenReference xmlns:wsse="http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd"
                    xmlns:wsu="http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd" wsu:Id="STRId-1008354042">
                    <wsse:Reference xmlns:wsse="http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd"
                        URI="#CertId-3458500" ValueType="http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-x509-token-profile-1.0#X509v3" />
                </wsse:SecurityTokenReference>
            </ds:KeyInfo>
        </ds:Signature>
    */

    @Override
    public void processHeaderEvent(OutputProcessorChain outputProcessorChain) throws XMLStreamException, XMLSecurityException {

        OutputProcessorChain subOutputProcessorChain = outputProcessorChain.createSubChain(this);

        List<XMLSecAttribute> attributes = new ArrayList<XMLSecAttribute>(1);
        attributes.add(createAttribute(XMLSecurityConstants.ATT_NULL_Id, IDGenerator.generateID(null)));
        XMLSecStartElement signatureElement = createStartElementAndOutputAsEvent(subOutputProcessorChain,
                XMLSecurityConstants.TAG_dsig_Signature, true, attributes);

        SignatureAlgorithm signatureAlgorithm;
        try {
            signatureAlgorithm = SignatureAlgorithmFactory.getInstance().getSignatureAlgorithm(
                    getSecurityProperties().getSignatureAlgorithm());
        } catch (NoSuchAlgorithmException e) {
            throw new XMLSecurityException(e);
        } catch (NoSuchProviderException e) {
            throw new XMLSecurityException(e);
        }

        String tokenId = outputProcessorChain.getSecurityContext().get(XMLSecurityConstants.PROP_USE_THIS_TOKEN_ID_FOR_SIGNATURE);
        if (tokenId == null) {
            throw new XMLSecurityException("stax.keyNotFound");
        }
        SecurityTokenProvider<OutboundSecurityToken> wrappingSecurityTokenProvider =
                outputProcessorChain.getSecurityContext().getSecurityTokenProvider(tokenId);
        if (wrappingSecurityTokenProvider == null) {
            throw new XMLSecurityException("stax.keyNotFound");
        }
        final OutboundSecurityToken wrappingSecurityToken = wrappingSecurityTokenProvider.getSecurityToken();
        if (wrappingSecurityToken == null) {
            throw new XMLSecurityException("stax.keyNotFound");
        }

        signatureAlgorithm.engineInitSign(wrappingSecurityToken.getSecretKey(getSecurityProperties().getSignatureAlgorithm()));

        SignedInfoProcessor signedInfoProcessor = newSignedInfoProcessor(signatureAlgorithm, signatureElement, subOutputProcessorChain);
        createStartElementAndOutputAsEvent(subOutputProcessorChain, XMLSecurityConstants.TAG_dsig_SignedInfo, false, null);

        attributes = new ArrayList<XMLSecAttribute>(1);
        final String signatureCanonicalizationAlgorithm = getSecurityProperties().getSignatureCanonicalizationAlgorithm();
        attributes.add(createAttribute(XMLSecurityConstants.ATT_NULL_Algorithm, signatureCanonicalizationAlgorithm));
        createStartElementAndOutputAsEvent(subOutputProcessorChain, XMLSecurityConstants.TAG_dsig_CanonicalizationMethod, false, attributes);

        if (getSecurityProperties().isAddExcC14NInclusivePrefixes() && XMLSecurityConstants.NS_C14N_EXCL.equals(signatureCanonicalizationAlgorithm)) {
            attributes = new ArrayList<XMLSecAttribute>(1);
            attributes.add(createAttribute(XMLSecurityConstants.ATT_NULL_PrefixList, signedInfoProcessor.getInclusiveNamespacePrefixes()));
            createStartElementAndOutputAsEvent(subOutputProcessorChain, XMLSecurityConstants.TAG_c14nExcl_InclusiveNamespaces, true, attributes);
            createEndElementAndOutputAsEvent(subOutputProcessorChain, XMLSecurityConstants.TAG_c14nExcl_InclusiveNamespaces);
        }

        createEndElementAndOutputAsEvent(subOutputProcessorChain, XMLSecurityConstants.TAG_dsig_CanonicalizationMethod);

        attributes = new ArrayList<XMLSecAttribute>(1);
        attributes.add(createAttribute(XMLSecurityConstants.ATT_NULL_Algorithm, getSecurityProperties().getSignatureAlgorithm()));
        createStartElementAndOutputAsEvent(subOutputProcessorChain, XMLSecurityConstants.TAG_dsig_SignatureMethod, false, attributes);
        createEndElementAndOutputAsEvent(subOutputProcessorChain, XMLSecurityConstants.TAG_dsig_SignatureMethod);

        Iterator<SignaturePartDef> signaturePartDefIterator = signaturePartDefList.iterator();
        while (signaturePartDefIterator.hasNext()) {
            SignaturePartDef signaturePartDef = signaturePartDefIterator.next();
            String uriString;
            if (signaturePartDef.isExternalResource()) {
                uriString = signaturePartDef.getSigRefId();
            } else if (signaturePartDef.isGenerateXPointer()) {
                uriString = "#xpointer(id('" + signaturePartDef.getSigRefId() + "'))";
            } else {
                uriString = "#" + signaturePartDef.getSigRefId();
            }
            attributes = new ArrayList<XMLSecAttribute>(1);
            attributes.add(createAttribute(XMLSecurityConstants.ATT_NULL_URI, uriString));
            createStartElementAndOutputAsEvent(subOutputProcessorChain, XMLSecurityConstants.TAG_dsig_Reference, false, attributes);
            createTransformsStructureForSignature(subOutputProcessorChain, signaturePartDef);

            attributes = new ArrayList<XMLSecAttribute>(1);
            attributes.add(createAttribute(XMLSecurityConstants.ATT_NULL_Algorithm, signaturePartDef.getDigestAlgo()));
            createStartElementAndOutputAsEvent(subOutputProcessorChain, XMLSecurityConstants.TAG_dsig_DigestMethod, false, attributes);
            createEndElementAndOutputAsEvent(subOutputProcessorChain, XMLSecurityConstants.TAG_dsig_DigestMethod);
            createStartElementAndOutputAsEvent(subOutputProcessorChain, XMLSecurityConstants.TAG_dsig_DigestValue, false, null);
            createCharactersAndOutputAsEvent(subOutputProcessorChain, signaturePartDef.getDigestValue());
            createEndElementAndOutputAsEvent(subOutputProcessorChain, XMLSecurityConstants.TAG_dsig_DigestValue);
            createEndElementAndOutputAsEvent(subOutputProcessorChain, XMLSecurityConstants.TAG_dsig_Reference);
        }

        createEndElementAndOutputAsEvent(subOutputProcessorChain, XMLSecurityConstants.TAG_dsig_SignedInfo);
        subOutputProcessorChain.removeProcessor(signedInfoProcessor);

        createStartElementAndOutputAsEvent(subOutputProcessorChain, XMLSecurityConstants.TAG_dsig_SignatureValue, false, null);
        final byte[] signatureValue = signedInfoProcessor.getSignatureValue();
        createCharactersAndOutputAsEvent(subOutputProcessorChain, new Base64(76, new byte[]{'\n'}).encodeToString(signatureValue));
        createEndElementAndOutputAsEvent(subOutputProcessorChain, XMLSecurityConstants.TAG_dsig_SignatureValue);

        attributes = new ArrayList<XMLSecAttribute>(1);
        attributes.add(createAttribute(XMLSecurityConstants.ATT_NULL_Id, IDGenerator.generateID(null)));
        createStartElementAndOutputAsEvent(subOutputProcessorChain, XMLSecurityConstants.TAG_dsig_KeyInfo, false, attributes);
        createKeyInfoStructureForSignature(subOutputProcessorChain, wrappingSecurityToken, getSecurityProperties().isUseSingleCert());
        createEndElementAndOutputAsEvent(subOutputProcessorChain, XMLSecurityConstants.TAG_dsig_KeyInfo);
        createEndElementAndOutputAsEvent(subOutputProcessorChain, XMLSecurityConstants.TAG_dsig_Signature);
    }

    protected abstract SignedInfoProcessor newSignedInfoProcessor(
            SignatureAlgorithm signatureAlgorithm, XMLSecStartElement xmlSecStartElement, OutputProcessorChain outputProcessorChain)
            throws XMLSecurityException;

    protected abstract void createTransformsStructureForSignature(
            OutputProcessorChain subOutputProcessorChain,
            SignaturePartDef signaturePartDef) throws XMLStreamException, XMLSecurityException;

    protected abstract void createKeyInfoStructureForSignature(
            OutputProcessorChain outputProcessorChain,
            OutboundSecurityToken securityToken,
            boolean useSingleCertificate) throws XMLStreamException, XMLSecurityException;


    public class SignedInfoProcessor extends AbstractOutputProcessor {

        private SignerOutputStream signerOutputStream;
        private OutputStream bufferedSignerOutputStream;
        private Transformer transformer;
        private byte[] signatureValue = null;
        private String inclusiveNamespacePrefixes = null;
        private SignatureAlgorithm signatureAlgorithm;
        private XMLSecStartElement xmlSecStartElement;

        public SignedInfoProcessor(SignatureAlgorithm signatureAlgorithm, XMLSecStartElement xmlSecStartElement)
                throws XMLSecurityException {
            super();
            this.signatureAlgorithm = signatureAlgorithm;
            this.xmlSecStartElement = xmlSecStartElement;
        }

        @Override
        public void init(OutputProcessorChain outputProcessorChain) throws XMLSecurityException {

            this.signerOutputStream = new SignerOutputStream(this.signatureAlgorithm);
            this.bufferedSignerOutputStream = new UnsynchronizedBufferedOutputStream(this.signerOutputStream);

            final String canonicalizationAlgorithm = getSecurityProperties().getSignatureCanonicalizationAlgorithm();

            Map<String, Object> transformerProperties = null;
            if (getSecurityProperties().isAddExcC14NInclusivePrefixes() &&
                    XMLSecurityConstants.NS_C14N_EXCL.equals(canonicalizationAlgorithm)) {

                Set<String> prefixSet = XMLSecurityUtils.getExcC14NInclusiveNamespacePrefixes(xmlSecStartElement, false);
                StringBuilder prefixes = new StringBuilder();
                for (Iterator<String> iterator = prefixSet.iterator(); iterator.hasNext(); ) {
                    String prefix = iterator.next();
                    if (prefixes.length() != 0) {
                        prefixes.append(" ");
                    }
                    prefixes.append(prefix);
                }
                this.inclusiveNamespacePrefixes = prefixes.toString();

                transformerProperties = new HashMap<String, Object>(2);
                transformerProperties.put(
                        Canonicalizer20010315_Excl.INCLUSIVE_NAMESPACES_PREFIX_LIST, new ArrayList<String>(prefixSet));
            }

            this.transformer = XMLSecurityUtils.getTransformer(null, this.bufferedSignerOutputStream,
                    transformerProperties, canonicalizationAlgorithm, XMLSecurityConstants.DIRECTION.OUT);

            super.init(outputProcessorChain);
        }

        public byte[] getSignatureValue() throws XMLSecurityException {
            if (signatureValue != null) {
                return signatureValue;
            }
            try {
                transformer.doFinal();
                bufferedSignerOutputStream.close();
                signatureValue = signerOutputStream.sign();
                return signatureValue;
            } catch (IOException e) {
                throw new XMLSecurityException(e);
            } catch (XMLStreamException e) {
                throw new XMLSecurityException(e);
            }
        }

        public String getInclusiveNamespacePrefixes() {
            return inclusiveNamespacePrefixes;
        }

        @Override
        public void processEvent(XMLSecEvent xmlSecEvent, OutputProcessorChain outputProcessorChain)
                throws XMLStreamException, XMLSecurityException {
            transformer.transform(xmlSecEvent);
            outputProcessorChain.processEvent(xmlSecEvent);
        }
    }
}
