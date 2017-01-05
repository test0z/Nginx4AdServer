/**
 * Copyright (c) 2012 Tanx.com
 * All rights reserved.
 *
 * This is a sample code to show the decode encoded deviceId.
 * It uses the Base64 decoder and the MD5 in the OpenSSL library.
 */

#include "logging.h"
#include <arpa/inet.h>
#include <iostream>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/md5.h>

using std::string;

// Calc the length of the decoded
#define base64DecodedLength(len) (((len + 3) / 4) * 3)

#define VERSION_LENGTH 1
#define DEVICEID_LENGTH 1
#define CRC_LENGTH 4
#define KEY_LENGTH 16

#define VERSION_OFFSITE 0
#define LENGTH_OFFSITE (VERSION_OFFSITE + VERSION_LENGTH)
#define ENCODEDEVICEID_OFFSITE (LENGTH_OFFSITE + DEVICEID_LENGTH)

typedef unsigned char uchar;

// Key just for test
// Get it from tanx, format like:
// f7dbeb735b7a07f1cfca79cc1dfe4fa4
static uchar g_key[]
    = { 0x01, 0xd5, 0xba, 0xaf, 0x00, 0x9c, 0x4d, 0xd6, 0xbb, 0x01, 0xb7, 0x11, 0xfc, 0x74, 0x92, 0x92 };

// Standard base64 decoder
static int base64Decode(char * src, int srcLen, char * dst, int dstLen)
{
    BIO *bio, *b64;
    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_new_mem_buf(src, srcLen);
    bio = BIO_push(b64, bio);
    int dstNewLen = BIO_read(bio, dst, dstLen);
    BIO_free_all(bio);
    return dstNewLen;
}

// Step list for decoding original deviceId
// If this function returns true, real deviceId will
// return to realDeviceId variable
// If this function returns false, it means format error or checksum
// error, realDeviceId variable is invalid
static bool decodeDeviceId(uchar * src, uchar * realDeviceId)
{
    // Get version and check
    int v = *src;
    if (v != 1) {
        LOG_ERROR << "version:" << v << "!=1, error";
        return false;
    }

    // get deviceId length
    uchar len = *(src + VERSION_LENGTH);

    // Copy key
    uchar buf[64];
    memcpy(buf, g_key, KEY_LENGTH);

    // ctxBuf=MD5(key)
    uchar ctxBuf[KEY_LENGTH];
    MD5(buf, KEY_LENGTH, ctxBuf);

    // Get deviceId
    uchar deviceId[64];
    uchar * pDeviceId = deviceId;
    uchar * pEncodeDeviceId = src + ENCODEDEVICEID_OFFSITE;
    uchar * pXor = ctxBuf;
    uchar * pXorB = pXor;
    uchar * pXorE = pXorB + KEY_LENGTH;
    for (int i = 0; i < len; ++i) {
        if (pXor == pXorE) {
            pXor = pXorB;
        }
        *pDeviceId++ = *pEncodeDeviceId++ ^ *pXor++;
    }

    // Copy decode deviceId
    memmove(realDeviceId, deviceId, len);
    realDeviceId[len] = '\0';
    // Calc crc and compare with src
    // If not match, it is illegal

    // copy(version+length+deviceId+key)
    uchar * pbuf = buf;
    memcpy(pbuf, src, VERSION_LENGTH + DEVICEID_LENGTH); // copy version+length
    pbuf += VERSION_LENGTH + DEVICEID_LENGTH;

    // Notice: here is deviceId not realDeviceId
    // More important for big endian !
    memcpy(pbuf, deviceId, len); // copy deviceId
    pbuf += len;
    memcpy(pbuf, g_key, KEY_LENGTH); // copy key

    // MD5(version+length+deviceId+key)
    MD5(buf, VERSION_LENGTH + DEVICEID_LENGTH + len + KEY_LENGTH, ctxBuf);
    if (0 != memcmp(ctxBuf, src + ENCODEDEVICEID_OFFSITE + len, CRC_LENGTH)) {
        LOG_ERROR << "checksum error!";
        return false;
    }

    return true;
}

/*
 * More sample string for encoded deviceId:
 *   1. src: AQ927DKCkp3HaJ+1n60VSBngmY2K
 *      result:
 *           Decode deviceId ok, deviceId = 493002407599521
 *   2. src: ASRy5TGF4Z7HbYXG4NISVxy1c+wsi5CWwHXqxuSmFj8QwHDiQ/CVPeSb
 *      result:
 *           Decode deviceId ok, deviceId = 0007C145-FFF2-4119-9293-BFB26E8D27BB
 *   3. src: AREDlCzw4IKwG4XE4rllPwW0c166xOg=
 *      result:
 *           Decode deviceId ok, deviceId = AA-BB-CC-DD-EE-01
 */
std::string tanxDeviceId(const std::string & src)
{
    // The sample string for encoded deviceId
    // result:
    //    Decode deviceId ok, deviceId = 0007C145-FFF2-4119-9293-BFB26E8D27BB
    // char src[] = "ASRy5TGF4Z7HbYXG4NISVxy1c+wsi5CWwHXqxuSmFj8QwHDiQ/CVPeSb";
    int len = src.length();
    if (len == 0) {
        return src;
    }
    int origLen = base64DecodedLength(len);
    uchar orig[1024];

    // Standard BASE64 decoder using ssl library
    origLen = base64Decode(const_cast<char *>(src.c_str()), len, (char *)orig, origLen);

    uchar deviceId[64] = { '\0' };
    if (decodeDeviceId(orig, deviceId)) {
        return std::string((const char *)deviceId);
    }

    return "";
}
