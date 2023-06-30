/**
 * Copyright (c) 2020 Staz Modrzynski
 * Copyright (c) 2020 Paul-Louis Ageneau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef RTC_MEDIA_HANDLER_H
#define RTC_MEDIA_HANDLER_H

#include "common.hpp"
#include "description.hpp"
#include "message.hpp"

namespace rtc {

class RTC_CPP_EXPORT MediaHandler : std::enable_shared_from_this<MediaHandler> {
public:
	// Called when a media is added or updated
	virtual void media([[maybe_unused]] const Description::Media &desc) {}

	// Called when there is traffic coming from the peer
	virtual message_ptr incoming(message_ptr message) = 0;

	// Called when there is traffic that needs to be sent to the peer
	virtual message_ptr outgoing(message_ptr message) = 0;

	// This callback is used to send traffic back to the peer
	void onOutgoing(const std::function<void(message_ptr)> &cb);

	void send(message_ptr message);

	void addToChain(shared_ptr<MediaHandler> handler);
	void setNext(shared_ptr<MediaHandler> handler);
	shared_ptr<MediaHandler> next();
	shared_ptr<const MediaHandler> next() const;
	shared_ptr<MediaHandler> last();             // never null
	shared_ptr<const MediaHandler> last() const; // never null

	virtual bool requestKeyframe();

	void mediaChain(const Description::Media &desc);
	message_ptr incomingChain(message_ptr message);
	message_ptr outgoingChain(message_ptr message);

private:
	synchronized_callback<message_ptr> mOutgoingCallback;
	shared_ptr<MediaHandler> mNext;
};

} // namespace rtc

#endif // RTC_MEDIA_HANDLER_H
