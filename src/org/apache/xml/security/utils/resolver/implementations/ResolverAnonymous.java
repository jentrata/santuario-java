package org.apache.xml.security.utils.resolver.implementations;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;

import org.apache.xml.security.signature.XMLSignatureInput;
import org.apache.xml.security.utils.resolver.ResourceResolverSpi;
import org.w3c.dom.Attr;

/**
 *
 * @author $Author$
 */
public class ResolverAnonymous extends ResourceResolverSpi {
   /** {@link org.apache.log4j} logging facility */
   static org.apache.log4j.Category cat =
      org.apache.log4j.Category.getInstance(ResolverAnonymous.class.getName());

   private XMLSignatureInput _input = null;

   public ResolverAnonymous(String filename) throws FileNotFoundException, IOException {
      this._input = new XMLSignatureInput(new FileInputStream(filename));
   }

   public ResolverAnonymous(InputStream is) throws IOException {
      this._input = new XMLSignatureInput(is);
   }

   public XMLSignatureInput engineResolve(Attr uri, String BaseURI) {
      return this._input;
   }

   /**
    * We resolve anonymous (unspecified) URIs
    *
    * @param uri
    * @param BaseURI
    *
    */
   public boolean engineCanResolve(Attr uri, String BaseURI) {
      if (uri == null) {
         return true;
      }
      return false;
   }

   /**
    * Method engineGetPropertyKeys
    *
    *
    */
   public String[] engineGetPropertyKeys() {
      return new String[0];
   }
}