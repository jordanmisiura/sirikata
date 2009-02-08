/*  Sirikata Transfer -- Content Distribution Network
 *  HTTPDownloadHandler.hpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*  Created on: Feb 8, 2009 */

#ifndef SIRIKATA_HTTPDownloadHandler_HPP__
#define SIRIKATA_HTTPDownloadHandler_HPP__

#include "HTTPRequest.hpp"
#include "ProtocolRegistry.hpp"

namespace Sirikata {
namespace Transfer {


// Should keep track of ongoing downloads and provide an abort function.
class HTTPDownloadHandler : public DownloadHandler, public NameLookupHandler {

	class HTTPTransferData : public TransferData {
		HTTPRequestPtr http;
	public:
		HTTPTransferData(const std::tr1::shared_ptr<DownloadHandler> &parent,
				const HTTPRequestPtr &httpreq)
			:TransferData(parent), http(httpreq) {
		}

		virtual ~HTTPTransferData() {}

		virtual void abort() {
			http->abort();
		}
	};

	static void httpCallback(
			DownloadHandler::Callback callback,
			HTTPRequest* httpreq,
			const DenseDataPtr &recvData,
			bool success) {
		callback(recvData, success);
	}

	struct IsSpace {
		bool operator()(const unsigned char c) {
			return std::isspace(c);
		}
	};

	static void nameCallback(
			NameLookupHandler::Callback callback,
			HTTPRequest* httpreq,
			const DenseDataPtr &data,
			bool success) {
		if (success) {
			cache_usize_type length = 0;
			const unsigned char *content = data->data();
			std::string receivedUri (content, content + data->length());
			receivedUri.erase(std::remove_if(receivedUri.begin(), receivedUri.end(), IsSpace()), receivedUri.end());
			URI temp(httpreq->getURI().context(), receivedUri);
			std::string shasum = temp.filename();
			callback(Fingerprint::convertFromHex(shasum), receivedUri, true);
		} else {
			std::cerr << "HTTP name lookup failed for " << httpreq->getURI() << std::endl;
			callback(Fingerprint(), std::string(), false);
		}
	}

public:
	virtual TransferDataPtr download(const URI &uri, const Range &bytes, const DownloadHandler::Callback &cb) {
		HTTPRequestPtr req (new HTTPRequest(uri, bytes));
//		using std::tr1::placeholders::_1;
//		using std::tr1::placeholders::_2;
//		using std::tr1::placeholders::_3;
		req->setCallback(
			std::tr1::bind(&HTTPDownloadHandler::httpCallback, cb, _1, _2, _3));
		// should call callback when it finishes.
		req->go(req);
		return TransferDataPtr(new HTTPTransferData(shared_from_this(), req));
	}

	virtual void nameLookup(const URI &uri, const NameLookupHandler::Callback &cb) {
		HTTPRequestPtr req (new HTTPRequest(uri, Range(true)));
		req->setCallback(
			std::tr1::bind(&HTTPDownloadHandler::nameCallback, cb, _1, _2, _3));
		req->go(req);
	}
};

}
}

#endif /* SIRIKATA_HTTPDownloadHandler_HPP__ */