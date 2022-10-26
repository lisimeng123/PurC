/*
 * Copyright (C) 2006 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ResourceError_h
#define ResourceError_h

#include "ResourceErrorBase.h"

#if USE(SOUP)

#include <wtf/glib/GRefPtr.h>

typedef struct _GTlsCertificate GTlsCertificate;
typedef struct _SoupRequest SoupRequest;
typedef struct _SoupMessage SoupMessage;

namespace PurCFetcher {

class ResourceError : public ResourceErrorBase
{
public:
    ResourceError(Type type = Type::Null)
        : ResourceErrorBase(type)
        , m_tlsErrors(0)
    {
    }

    ResourceError(const String& domain, int errorCode, const URL& failingURL, const String& localizedDescription, Type type = Type::General)
        : ResourceErrorBase(domain, errorCode, failingURL, localizedDescription, type)
        , m_tlsErrors(0)
    {
    }

    static ResourceError httpError(SoupMessage*, GError*, SoupRequest*);
    static ResourceError transportError(SoupRequest*, int statusCode, const String& reasonPhrase);
    static ResourceError genericGError(GError*, SoupRequest*);
    static ResourceError tlsError(const URL&, unsigned tlsErrors, GTlsCertificate*);
    static ResourceError timeoutError(const URL& failingURL);
    static ResourceError authenticationError(SoupMessage*);

    unsigned tlsErrors() const { return m_tlsErrors; }
    void setTLSErrors(unsigned tlsErrors) { m_tlsErrors = tlsErrors; }
    GTlsCertificate* certificate() const { return m_certificate.get(); }
    void setCertificate(GTlsCertificate* certificate) { m_certificate = certificate; }

    static bool platformCompare(const ResourceError& a, const ResourceError& b);

private:
    friend class ResourceErrorBase;
    void doPlatformIsolatedCopy(const ResourceError&);

    unsigned m_tlsErrors;
    GRefPtr<GTlsCertificate> m_certificate;
};

}

#endif

#endif // ResourceError_h_
