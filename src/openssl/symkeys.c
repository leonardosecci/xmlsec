/** 
 *
 * XMLSec library
 * 
 * DES Algorithm support
 * 
 * See Copyright for the status of this software.
 * 
 * Author: Aleksey Sanin <aleksey@aleksey.com>
 */
#include "globals.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <openssl/rand.h>

#include <xmlsec/xmlsec.h>
#include <xmlsec/xmltree.h>
#include <xmlsec/keys.h>
#include <xmlsec/keyinfo.h>
#include <xmlsec/transforms.h>
#include <xmlsec/transformsInternal.h>
#include <xmlsec/errors.h>

#include <xmlsec/openssl/crypto.h>

/*****************************************************************************
 * 
 * Symmetic (binary) keys - just a wrapper for xmlSecKeyDataBinary
 *
 ****************************************************************************/
static int	xmlSecOpenSSLSymKeyDataInitialize	(xmlSecKeyDataPtr data);
static int	xmlSecOpenSSLSymKeyDataDuplicate	(xmlSecKeyDataPtr dst,
							 xmlSecKeyDataPtr src);
static void	xmlSecOpenSSLSymKeyDataFinalize		(xmlSecKeyDataPtr data);
static int	xmlSecOpenSSLSymKeyDataXmlRead		(xmlSecKeyDataId id,
							 xmlSecKeyPtr key,
							 xmlNodePtr node,
							 xmlSecKeyInfoCtxPtr keyInfoCtx);
static int	xmlSecOpenSSLSymKeyDataXmlWrite		(xmlSecKeyDataId id,
							 xmlSecKeyPtr key,
							 xmlNodePtr node,
							 xmlSecKeyInfoCtxPtr keyInfoCtx);
static int	xmlSecOpenSSLSymKeyDataBinRead		(xmlSecKeyDataId id,
							 xmlSecKeyPtr key,
							 const unsigned char* buf,
							 size_t bufSize,
							 xmlSecKeyInfoCtxPtr keyInfoCtx);
static int	xmlSecOpenSSLSymKeyDataBinWrite		(xmlSecKeyDataId id,
							 xmlSecKeyPtr key,
							 unsigned char** buf,
							 size_t* bufSize,
							 xmlSecKeyInfoCtxPtr keyInfoCtx);
static int	xmlSecOpenSSLSymKeyDataGenerate		(xmlSecKeyDataPtr data,
							 size_t sizeBits,
							 xmlSecKeyDataType type);

static xmlSecKeyDataType xmlSecOpenSSLSymKeyDataGetType	(xmlSecKeyDataPtr data);
static size_t	xmlSecOpenSSLSymKeyDataGetSize		(xmlSecKeyDataPtr data);
static void	xmlSecOpenSSLSymKeyDataDebugDump	(xmlSecKeyDataPtr data,
							 FILE* output);
static void	xmlSecOpenSSLSymKeyDataDebugXmlDump	(xmlSecKeyDataPtr data,
							 FILE* output);
static int	xmlSecOpenSSLSymKeyDataKlassCheck	(xmlSecKeyDataKlass* klass);

#define xmlSecOpenSSLSymKeyDataCheckId(data) \
    (xmlSecKeyDataIsValid((data)) && \
     xmlSecOpenSSLSymKeyDataKlassCheck((data)->id))

static int
xmlSecOpenSSLSymKeyDataInitialize(xmlSecKeyDataPtr data) {
    xmlSecAssert2(xmlSecOpenSSLSymKeyDataCheckId(data), -1);
    
    return(xmlSecKeyDataBinaryValueInitialize(data));
}

static int
xmlSecOpenSSLSymKeyDataDuplicate(xmlSecKeyDataPtr dst, xmlSecKeyDataPtr src) {
    xmlSecAssert2(xmlSecOpenSSLSymKeyDataCheckId(dst), -1);
    xmlSecAssert2(xmlSecOpenSSLSymKeyDataCheckId(src), -1);
    xmlSecAssert2(dst->id == src->id, -1);
        
    return(xmlSecKeyDataBinaryValueDuplicate(dst, src));
}

static void
xmlSecOpenSSLSymKeyDataFinalize(xmlSecKeyDataPtr data) {
    xmlSecAssert(xmlSecOpenSSLSymKeyDataCheckId(data));
    
    xmlSecKeyDataBinaryValueFinalize(data);
}

static int
xmlSecOpenSSLSymKeyDataXmlRead(xmlSecKeyDataId id, xmlSecKeyPtr key,
			       xmlNodePtr node, xmlSecKeyInfoCtxPtr keyInfoCtx) {
    xmlSecAssert2(xmlSecOpenSSLSymKeyDataKlassCheck(id), -1);
    
    return(xmlSecKeyDataBinaryValueXmlRead(id, key, node, keyInfoCtx));
}

static int 
xmlSecOpenSSLSymKeyDataXmlWrite(xmlSecKeyDataId id, xmlSecKeyPtr key,
				    xmlNodePtr node, xmlSecKeyInfoCtxPtr keyInfoCtx) {
    xmlSecAssert2(xmlSecOpenSSLSymKeyDataKlassCheck(id), -1);
    
    return(xmlSecKeyDataBinaryValueXmlWrite(id, key, node, keyInfoCtx));
}

static int
xmlSecOpenSSLSymKeyDataBinRead(xmlSecKeyDataId id, xmlSecKeyPtr key,
				    const unsigned char* buf, size_t bufSize,
				    xmlSecKeyInfoCtxPtr keyInfoCtx) {
    xmlSecAssert2(xmlSecOpenSSLSymKeyDataKlassCheck(id), -1);
    
    return(xmlSecKeyDataBinaryValueBinRead(id, key, buf, bufSize, keyInfoCtx));
}

static int
xmlSecOpenSSLSymKeyDataBinWrite(xmlSecKeyDataId id, xmlSecKeyPtr key,
				    unsigned char** buf, size_t* bufSize,
				    xmlSecKeyInfoCtxPtr keyInfoCtx) {
    xmlSecAssert2(xmlSecOpenSSLSymKeyDataKlassCheck(id), -1);
    
    return(xmlSecKeyDataBinaryValueBinWrite(id, key, buf, bufSize, keyInfoCtx));
}

static int
xmlSecOpenSSLSymKeyDataGenerate(xmlSecKeyDataPtr data, size_t sizeBits, xmlSecKeyDataType type ATTRIBUTE_UNUSED) {
    xmlSecBufferPtr buffer;

    xmlSecAssert2(xmlSecOpenSSLSymKeyDataCheckId(data), -1);
    xmlSecAssert2(sizeBits > 0, -1);

    buffer = xmlSecKeyDataBinaryValueGetBuffer(data);
    xmlSecAssert2(buffer != NULL, -1);
    
    return(xmlSecOpenSSLGenerateRandom(buffer, (sizeBits + 7) / 8));
}

static xmlSecKeyDataType
xmlSecOpenSSLSymKeyDataGetType(xmlSecKeyDataPtr data) {
    xmlSecBufferPtr buffer;

    xmlSecAssert2(xmlSecOpenSSLSymKeyDataCheckId(data), xmlSecKeyDataTypeUnknown);

    buffer = xmlSecKeyDataBinaryValueGetBuffer(data);
    xmlSecAssert2(buffer != NULL, xmlSecKeyDataTypeUnknown);

    return((xmlSecBufferGetSize(buffer) > 0) ? xmlSecKeyDataTypeSymmetric : xmlSecKeyDataTypeUnknown);
}

static size_t 
xmlSecOpenSSLSymKeyDataGetSize(xmlSecKeyDataPtr data) {
    xmlSecAssert2(xmlSecOpenSSLSymKeyDataCheckId(data), 0);
    
    return(xmlSecKeyDataBinaryValueGetSize(data));
}

static void 
xmlSecOpenSSLSymKeyDataDebugDump(xmlSecKeyDataPtr data, FILE* output) {
    xmlSecAssert(xmlSecOpenSSLSymKeyDataCheckId(data));
    
    xmlSecKeyDataBinaryValueDebugDump(data, output);    
}

static void
xmlSecOpenSSLSymKeyDataDebugXmlDump(xmlSecKeyDataPtr data, FILE* output) {
    xmlSecAssert(xmlSecOpenSSLSymKeyDataCheckId(data));
    
    xmlSecKeyDataBinaryValueDebugXmlDump(data, output);    
}

static int 
xmlSecOpenSSLSymKeyDataKlassCheck(xmlSecKeyDataKlass* klass) {    
#ifndef XMLSEC_NO_DES
    if(klass == xmlSecOpenSSLKeyDataDesId) {
	return(1);
    }
#endif /* XMLSEC_NO_DES */

#ifndef XMLSEC_NO_AES
#ifndef XMLSEC_OPENSSL_096
    if(klass == xmlSecOpenSSLKeyDataAesId) {
	return(1);
    }
#endif /* XMLSEC_OPENSSL_096 */
#endif /* XMLSEC_NO_AES */

#ifndef XMLSEC_NO_HMAC
    if(klass == xmlSecOpenSSLKeyDataHmacId) {
	return(1);
    }
#endif /* XMLSEC_NO_HMAC */

    return(0);
}

#ifndef XMLSEC_NO_AES
#ifndef XMLSEC_OPENSSL_096
/**************************************************************************
 *
 * <xmlsec:AESKeyValue> processing
 *
 *************************************************************************/
static xmlSecKeyDataKlass xmlSecOpenSSLKeyDataAesKlass = {
    sizeof(xmlSecKeyDataKlass),
    xmlSecKeyDataBinarySize,

    /* data */
    xmlSecNameAESKeyValue,
    xmlSecKeyDataUsageKeyValueNode | xmlSecKeyDataUsageRetrievalMethodNodeXml, 
						/* xmlSecKeyDataUsage usage; */
    xmlSecHrefAESKeyValue,			/* const xmlChar* href; */
    xmlSecNodeAESKeyValue,			/* const xmlChar* dataNodeName; */
    xmlSecNs,					/* const xmlChar* dataNodeNs; */
    
    /* constructors/destructor */
    xmlSecOpenSSLSymKeyDataInitialize,		/* xmlSecKeyDataInitializeMethod initialize; */
    xmlSecOpenSSLSymKeyDataDuplicate,		/* xmlSecKeyDataDuplicateMethod duplicate; */
    xmlSecOpenSSLSymKeyDataFinalize,		/* xmlSecKeyDataFinalizeMethod finalize; */
    xmlSecOpenSSLSymKeyDataGenerate,		/* xmlSecKeyDataGenerateMethod generate; */
    
    /* get info */
    xmlSecOpenSSLSymKeyDataGetType, 		/* xmlSecKeyDataGetTypeMethod getType; */
    xmlSecOpenSSLSymKeyDataGetSize,		/* xmlSecKeyDataGetSizeMethod getSize; */
    NULL,					/* xmlSecKeyDataGetIdentifier getIdentifier; */

    /* read/write */
    xmlSecOpenSSLSymKeyDataXmlRead,		/* xmlSecKeyDataXmlReadMethod xmlRead; */
    xmlSecOpenSSLSymKeyDataXmlWrite,		/* xmlSecKeyDataXmlWriteMethod xmlWrite; */
    xmlSecOpenSSLSymKeyDataBinRead,		/* xmlSecKeyDataBinReadMethod binRead; */
    xmlSecOpenSSLSymKeyDataBinWrite,		/* xmlSecKeyDataBinWriteMethod binWrite; */

    /* debug */
    xmlSecOpenSSLSymKeyDataDebugDump,		/* xmlSecKeyDataDebugDumpMethod debugDump; */
    xmlSecOpenSSLSymKeyDataDebugXmlDump, 	/* xmlSecKeyDataDebugDumpMethod debugXmlDump; */
};

xmlSecKeyDataId 
xmlSecOpenSSLKeyDataAesGetKlass(void) {
    return(&xmlSecOpenSSLKeyDataAesKlass);
}

int
xmlSecOpenSSLKeyDataAesSet(xmlSecKeyDataPtr data, const unsigned char* buf, size_t bufSize) {
    xmlSecBufferPtr buffer;
    
    xmlSecAssert2(xmlSecKeyDataCheckId(data, xmlSecOpenSSLKeyDataAesId), -1);
    xmlSecAssert2(buf != NULL, -1);
    xmlSecAssert2(bufSize > 0, -1);
    
    buffer = xmlSecKeyDataBinaryValueGetBuffer(data);
    xmlSecAssert2(buffer != NULL, -1);
    
    return(xmlSecBufferSetData(buffer, buf, bufSize));
}

#endif /* XMLSEC_OPENSSL_096 */
#endif /* XMLSEC_NO_AES */

#ifndef XMLSEC_NO_DES
/**************************************************************************
 *
 * <xmlsec:DESKeyValue> processing
 *
 *************************************************************************/
static xmlSecKeyDataKlass xmlSecOpenSSLKeyDataDesKlass = {
    sizeof(xmlSecKeyDataKlass),
    xmlSecKeyDataBinarySize,

    /* data */
    xmlSecNameDESKeyValue,
    xmlSecKeyDataUsageKeyValueNode | xmlSecKeyDataUsageRetrievalMethodNodeXml, 
						/* xmlSecKeyDataUsage usage; */
    xmlSecHrefDESKeyValue,			/* const xmlChar* href; */
    xmlSecNodeDESKeyValue,			/* const xmlChar* dataNodeName; */
    xmlSecNs,					/* const xmlChar* dataNodeNs; */
    
    /* constructors/destructor */
    xmlSecOpenSSLSymKeyDataInitialize,		/* xmlSecKeyDataInitializeMethod initialize; */
    xmlSecOpenSSLSymKeyDataDuplicate,		/* xmlSecKeyDataDuplicateMethod duplicate; */
    xmlSecOpenSSLSymKeyDataFinalize,		/* xmlSecKeyDataFinalizeMethod finalize; */
    xmlSecOpenSSLSymKeyDataGenerate,		/* xmlSecKeyDataGenerateMethod generate; */
    
    /* get info */
    xmlSecOpenSSLSymKeyDataGetType, 		/* xmlSecKeyDataGetTypeMethod getType; */
    xmlSecOpenSSLSymKeyDataGetSize,		/* xmlSecKeyDataGetSizeMethod getSize; */
    NULL,					/* xmlSecKeyDataGetIdentifier getIdentifier; */

    /* read/write */
    xmlSecOpenSSLSymKeyDataXmlRead,		/* xmlSecKeyDataXmlReadMethod xmlRead; */
    xmlSecOpenSSLSymKeyDataXmlWrite,		/* xmlSecKeyDataXmlWriteMethod xmlWrite; */
    xmlSecOpenSSLSymKeyDataBinRead,		/* xmlSecKeyDataBinReadMethod binRead; */
    xmlSecOpenSSLSymKeyDataBinWrite,		/* xmlSecKeyDataBinWriteMethod binWrite; */

    /* debug */
    xmlSecOpenSSLSymKeyDataDebugDump,		/* xmlSecKeyDataDebugDumpMethod debugDump; */
    xmlSecOpenSSLSymKeyDataDebugXmlDump, 	/* xmlSecKeyDataDebugDumpMethod debugXmlDump; */
};

xmlSecKeyDataId 
xmlSecOpenSSLKeyDataDesGetKlass(void) {
    return(&xmlSecOpenSSLKeyDataDesKlass);
}

int
xmlSecOpenSSLKeyDataDesSet(xmlSecKeyDataPtr data, const unsigned char* buf, size_t bufSize) {
    xmlSecBufferPtr buffer;
    
    xmlSecAssert2(xmlSecKeyDataCheckId(data, xmlSecOpenSSLKeyDataDesId), -1);
    xmlSecAssert2(buf != NULL, -1);
    xmlSecAssert2(bufSize > 0, -1);
    
    buffer = xmlSecKeyDataBinaryValueGetBuffer(data);
    xmlSecAssert2(buffer != NULL, -1);
    
    return(xmlSecBufferSetData(buffer, buf, bufSize));
}

#endif /* XMLSEC_NO_DES */

#ifndef XMLSEC_NO_HMAC
/**************************************************************************
 *
 * <xmlsec:HMACKeyValue> processing
 *
 *************************************************************************/
static xmlSecKeyDataKlass xmlSecOpenSSLKeyDataHmacKlass = {
    sizeof(xmlSecKeyDataKlass),
    xmlSecKeyDataBinarySize,

    /* data */
    xmlSecNameHMACKeyValue,
    xmlSecKeyDataUsageKeyValueNode | xmlSecKeyDataUsageRetrievalMethodNodeXml, 
						/* xmlSecKeyDataUsage usage; */
    xmlSecHrefHMACKeyValue,			/* const xmlChar* href; */
    xmlSecNodeHMACKeyValue,			/* const xmlChar* dataNodeName; */
    xmlSecNs,					/* const xmlChar* dataNodeNs; */
    
    /* constructors/destructor */
    xmlSecOpenSSLSymKeyDataInitialize,		/* xmlSecKeyDataInitializeMethod initialize; */
    xmlSecOpenSSLSymKeyDataDuplicate,		/* xmlSecKeyDataDuplicateMethod duplicate; */
    xmlSecOpenSSLSymKeyDataFinalize,		/* xmlSecKeyDataFinalizeMethod finalize; */
    xmlSecOpenSSLSymKeyDataGenerate,		/* xmlSecKeyDataGenerateMethod generate; */
    
    /* get info */
    xmlSecOpenSSLSymKeyDataGetType, 		/* xmlSecKeyDataGetTypeMethod getType; */
    xmlSecOpenSSLSymKeyDataGetSize,		/* xmlSecKeyDataGetSizeMethod getSize; */
    NULL,					/* xmlSecKeyDataGetIdentifier getIdentifier; */

    /* read/write */
    xmlSecOpenSSLSymKeyDataXmlRead,		/* xmlSecKeyDataXmlReadMethod xmlRead; */
    xmlSecOpenSSLSymKeyDataXmlWrite,		/* xmlSecKeyDataXmlWriteMethod xmlWrite; */
    xmlSecOpenSSLSymKeyDataBinRead,		/* xmlSecKeyDataBinReadMethod binRead; */
    xmlSecOpenSSLSymKeyDataBinWrite,		/* xmlSecKeyDataBinWriteMethod binWrite; */

    /* debug */
    xmlSecOpenSSLSymKeyDataDebugDump,		/* xmlSecKeyDataDebugDumpMethod debugDump; */
    xmlSecOpenSSLSymKeyDataDebugXmlDump, 	/* xmlSecKeyDataDebugDumpMethod debugXmlDump; */
};

xmlSecKeyDataId 
xmlSecOpenSSLKeyDataHmacGetKlass(void) {
    return(&xmlSecOpenSSLKeyDataHmacKlass);
}

int
xmlSecOpenSSLKeyDataHmacSet(xmlSecKeyDataPtr data, const unsigned char* buf, size_t bufSize) {
    xmlSecBufferPtr buffer;
    
    xmlSecAssert2(xmlSecKeyDataCheckId(data, xmlSecOpenSSLKeyDataHmacId), -1);
    xmlSecAssert2(buf != NULL, -1);
    xmlSecAssert2(bufSize > 0, -1);
    
    buffer = xmlSecKeyDataBinaryValueGetBuffer(data);
    xmlSecAssert2(buffer != NULL, -1);
    
    return(xmlSecBufferSetData(buffer, buf, bufSize));
}

#endif /* XMLSEC_NO_HMAC */

