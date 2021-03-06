/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "_jni/org_mozilla_jss_pkcs11_PK11Cipher.h"

#include <nspr.h>
#include <plarena.h>
#include <seccomon.h>
#include <pk11func.h>
#include <secitem.h>

/* JSS includes */
#include <java_ids.h>
#include <jss_exceptions.h>
#include <jssutil.h>
#include <pk11util.h>
#include <Algorithm.h>

/***********************************************************************
 *
 * PK11Cipher.initContext
 */
JNIEXPORT jobject JNICALL
Java_org_mozilla_jss_pkcs11_PK11Cipher_initContext
    (JNIEnv *env, jclass clazz, jboolean encrypt, jobject keyObj,
        jobject algObj, jbyteArray ivBA, jboolean padded)
{
    return Java_org_mozilla_jss_pkcs11_PK11Cipher_initContextWithKeyBits
        ( env, clazz, encrypt, keyObj, algObj, ivBA, 0, padded);
}

JNIEXPORT jobject JNICALL
Java_org_mozilla_jss_pkcs11_PK11Cipher_initContextWithKeyBits
    (JNIEnv *env, jclass clazz, jboolean encrypt, jobject keyObj,
        jobject algObj, jbyteArray ivBA, jint keyBits, jboolean padded)
{
    CK_MECHANISM_TYPE mech;
    PK11SymKey *key=NULL;
    SECItem *param=NULL;
    SECItem *iv=NULL;
    PK11Context *context=NULL;
    CK_ATTRIBUTE_TYPE op;
    jobject contextObj = NULL;

    PR_ASSERT(env!=NULL && clazz!=NULL && keyObj!=NULL && algObj!=NULL);

    /* get mechanism */
    mech = JSS_getPK11MechFromAlg(env, algObj);
    if(mech == CKM_INVALID_MECHANISM) {
        JSS_throwMsg(env, TOKEN_EXCEPTION, "Unable to resolve algorithm to"
            " PKCS #11 mechanism");
        goto finish;
    }

    if (padded)
        mech = PK11_GetPadMechanism(mech);

    /* get operation type */
    if( encrypt ) {
        op = CKA_ENCRYPT;
    } else {
        op = CKA_DECRYPT;
    }

    /* get key */
    if( JSS_PK11_getSymKeyPtr(env, keyObj, &key) != PR_SUCCESS) {
        goto finish;
    }

    /* get param, if there is one */
    if( ivBA != NULL ) {
        iv = JSS_ByteArrayToSECItem(env, ivBA);
        if( iv == NULL ) {
            /* exception was thrown */
            goto finish;
        }
    }
    param = PK11_ParamFromIV(mech, iv);

    /*
     * Set RC2 effective key length.
     */
    if( mech == CKM_RC2_CBC || mech == CKM_RC2_CBC_PAD ) {
        ((CK_RC2_CBC_PARAMS*)param->data)->ulEffectiveBits = keyBits;
    }
        

    /* create crypto context */
    context = PK11_CreateContextBySymKey(mech, op, key, param);
    if(context == NULL ) {
        JSS_throwMsg(env, TOKEN_EXCEPTION,
            "Failed to generate crypto context");
        goto finish;
    }

    /* wrap crypto context. This sets context to NULL. */
    contextObj = JSS_PK11_wrapCipherContextProxy(env, &context);

finish:
    if( param != NULL ) {
        SECITEM_FreeItem(param, PR_TRUE /*freeit*/);
    }
    if(iv) {
        SECITEM_FreeItem(iv, PR_TRUE /*freeit*/);
    }
    if(context != NULL) {
        /* if the function succeeded, context would be NULL */
        PK11_DestroyContext(context, PR_TRUE /*freeit*/);
    }
    PR_ASSERT( contextObj || (*env)->ExceptionOccurred(env) );
    return contextObj;
}

/***********************************************************************
 *
 * PK11Cipher.updateContext
 */
JNIEXPORT jbyteArray JNICALL
Java_org_mozilla_jss_pkcs11_PK11Cipher_updateContext
    (JNIEnv *env, jclass clazz, jobject contextObj, jbyteArray inputBA,
    jint blockSize)
{
    PK11Context *context=NULL;
    jbyte *inbuf=NULL;
    unsigned int inlen;
    unsigned char *outbuf=NULL;
    unsigned int outlen;
    jbyteArray outArray=NULL;

    PR_ASSERT(env!=NULL && clazz!=NULL && contextObj!=NULL && inputBA!=NULL);

    /* get the context */
    if( JSS_PK11_getCipherContext(env, contextObj, &context) != PR_SUCCESS) {
        goto finish;
    }

    /* extract input from byte array */
    inlen = (*env)->GetArrayLength(env, inputBA);
    PR_ASSERT(inlen >= 0);
    inbuf = (*env)->GetByteArrayElements(env, inputBA, NULL);
    if(inbuf == NULL) {
        ASSERT_OUTOFMEM(env);
        goto finish;
    }

    /* create output buffer */
    outlen = inlen + blockSize; /* this will hold the output */
    outbuf = PR_Malloc(outlen);
    if(outbuf == NULL) {
        JSS_throw(env, OUT_OF_MEMORY_ERROR);
        goto finish;
    }

    /* do the operation */
    if( PK11_CipherOp(context, outbuf, (int*)&outlen, outlen,
            (unsigned char*)inbuf, inlen) != SECSuccess) {
        JSS_throwMsgPrErrArg(
            env, TOKEN_EXCEPTION, "Cipher context update failed",
            PR_GetError());
        goto finish;
    }
    PR_ASSERT(outlen >= 0);

    /* convert output buffer to byte array */
    outArray = (*env)->NewByteArray(env, outlen);
    if(outArray == NULL) {
        ASSERT_OUTOFMEM(env);
        goto finish;
    }
    (*env)->SetByteArrayRegion(env, outArray, 0, outlen, (jbyte*)outbuf);

finish:
    if(inbuf) {
        (*env)->ReleaseByteArrayElements(env, inputBA, inbuf, JNI_ABORT);
    }
    if(outbuf) {
        PR_Free(outbuf);
    }
    return outArray;
}

/***********************************************************************
 *
 * PK11Cipher.finalizeContext
 *
 */
JNIEXPORT jbyteArray JNICALL
Java_org_mozilla_jss_pkcs11_PK11Cipher_finalizeContext
    (JNIEnv *env, jclass clazz, jobject contextObj, jint blockSize,
        jboolean padded)
{
    PK11Context *context=NULL;
    unsigned char *outBuf = NULL;
    unsigned int outLen, newOutLen;
    jobject outBA=NULL;
    SECStatus status;

    PR_ASSERT(env!=NULL && contextObj!=NULL);

    /* get context */
    if( JSS_PK11_getCipherContext(env, contextObj, &context) != PR_SUCCESS) {
        goto finish;
    }

    /* create output buffer */
    outLen = blockSize; /* maximum amount needed */
    outBuf = PR_Malloc(outLen);
    if(outBuf == NULL) {
        JSS_throw(env, OUT_OF_MEMORY_ERROR);
        goto finish;
    }

    /* perform the finalization */
    status = PK11_DigestFinal(context, outBuf, &newOutLen, outLen);
    if( (status != SECSuccess) ) {
        JSS_throwMsgPrErrArg(
            env, TOKEN_EXCEPTION, "Cipher context finalization failed",
            PR_GetError());
        goto finish;
    }

    /* convert output buffer to byte array */
    PR_ASSERT(newOutLen >= 0);
    outBA = (*env)->NewByteArray(env, newOutLen);
    if(outBA == NULL) {
        ASSERT_OUTOFMEM(env);
        goto finish;
    }
    (*env)->SetByteArrayRegion(env, outBA, 0, newOutLen, (jbyte*)outBuf);

finish:
    if(outBuf) {
        PR_Free(outBuf);
    }
    PR_ASSERT( outBA || (*env)->ExceptionOccurred(env) );
    return outBA;
}
    


/***********************************************************************
 *
 * J S S _ P K 1 1 _ g e t C i p h e r C o n t e x t
 *
 * Extracts the PK11Context from a CipherContextProxy.
 *
 * proxy
 *      A CipherContextProxy.
 *
 * pContext
 *      Address of a PK11Context*, which will be filled with the pointer
 *      extracted from the CipherContextProxy.
 *
 * RETURNS
 *      PR_SUCCESS for success, or PR_FAILURE if an exception was thrown.
 */
PRStatus
JSS_PK11_getCipherContext(JNIEnv *env, jobject proxy, PK11Context **pContext)
{

    PR_ASSERT(env!=NULL && proxy!=NULL && pContext!=NULL);

    return JSS_getPtrFromProxy(env, proxy, (void**)pContext);
}

/***********************************************************************
 *
 * J S S _ P K 1 1 _ m a k e C i p h e r C o n t e x t P r o x y
 *
 * Wraps a PK11Context in a CipherContextProxy.
 *
 * context
 *      address of a pointer to a PK11Context, which must not be NULL.
 *      It will be eaten by the wrapper and set to NULL, even if the
 *      function returns NULL.
 *
 * RETURNS
 *      A new CipherContextProxy, or NULL if an exception was thrown.
 */
jobject
JSS_PK11_wrapCipherContextProxy(JNIEnv *env, PK11Context **context) {

    jbyteArray pointer=NULL;
    jclass proxyClass;
    jmethodID constructor;
    jobject contextObj=NULL;

    PR_ASSERT( env!=NULL && context!=NULL && *context!=NULL );

    /* convert pointer to byte array */
    pointer = JSS_ptrToByteArray(env, *context);

    /*
     * Lookup the class and constructor
     */
    proxyClass = (*env)->FindClass(env, CIPHER_CONTEXT_PROXY_CLASS_NAME);
    if(proxyClass == NULL) {
        ASSERT_OUTOFMEM(env);
        goto finish;
    }
    constructor = (*env)->GetMethodID(env, proxyClass,
                            PLAIN_CONSTRUCTOR,
                            CIPHER_CONTEXT_PROXY_CONSTRUCTOR_SIG);
    if(constructor == NULL) {
        ASSERT_OUTOFMEM(env);
        goto finish;
    }

    /* call the constructor */
    contextObj = (*env)->NewObject(env, proxyClass, constructor, pointer);

finish:
    if(contextObj == NULL) {
        /* didn't work, so free resources */
        PK11_DestroyContext( (PK11Context*)*context, PR_TRUE /*freeit*/ );
    }
    *context=NULL;
    PR_ASSERT( contextObj || (*env)->ExceptionOccurred(env) );
    return contextObj;
}

/***********************************************************************
 *
 * CipherContextProxy.releaseNativeResources
 */
JNIEXPORT void JNICALL
Java_org_mozilla_jss_pkcs11_CipherContextProxy_releaseNativeResources
    (JNIEnv *env, jobject this)
{
    PK11Context *context;

    if( JSS_PK11_getCipherContext(env, this, &context) == PR_SUCCESS ) {
        PK11_DestroyContext(context, PR_TRUE /*freeit*/);
    }
}
