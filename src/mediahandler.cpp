/**
 * Copyright (c) 2023 Paul-Louis Ageneau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "mediahandler.hpp"

#include "impl/internals.hpp"

namespace rtc {

void MediaHandler::onOutgoing(const std::function<void(message_ptr)> &cb) {
	mOutgoingCallback = synchronized_callback<message_ptr>(cb);
}

void MediaHandler::send(message_ptr message) {
	try {
		mOutgoingCallback(std::move(message));

	} catch (const std::exception &e) {
		PLOG_WARNING << "Media handler send failed: " << e.what();
	}
}

void MediaHandler::addToChain(shared_ptr<MediaHandler> handler) { last()->setNext(handler); }

void MediaHandler::setNext(shared_ptr<MediaHandler> handler) {
	std::atomic_store(&mNext, handler);
	if(handler)
		onOutgoing(weak_bind(&MediaHandler::send, handler, std::placeholders::_1));
	else
		onOutgoing(nullptr);
}

shared_ptr<MediaHandler> MediaHandler::next() { return std::atomic_load(&mNext); }

shared_ptr<const MediaHandler> MediaHandler::next() const { return std::atomic_load(&mNext); }

shared_ptr<MediaHandler> MediaHandler::last() {
	if (auto handler = next())
		return handler->last();
	else
		return shared_from_this();
}

shared_ptr<const MediaHandler> MediaHandler::last() const {
	if (auto handler = next())
		return handler->last();
	else
		return shared_from_this();
}

bool MediaHandler::requestKeyframe() { return false; }

void MediaHandler::mediaChain(const Description::Media &desc) {
	media(desc);

	if(auto handler = next())
		handler->mediaChain(desc);
}

message_ptr MediaHandler::incomingChain(message_ptr message) {
	if(auto handler = next())
		message = handler->incomingChain(std::move(message));

	return incoming(std::move(message));
}

message_ptr MediaHandler::outgoingChain(message_ptr message) {
	message = outgoing(std::move(message));

	if(auto handler = next())
		return handler->outgoingChain(std::move(message));
	else
		return message;
}

} // namespace rtc

