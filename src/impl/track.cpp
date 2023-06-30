/**
 * Copyright (c) 2020 Paul-Louis Ageneau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "track.hpp"
#include "internals.hpp"
#include "logcounter.hpp"
#include "peerconnection.hpp"
#include "rtp.hpp"

namespace rtc::impl {

static LogCounter COUNTER_MEDIA_BAD_DIRECTION(plog::warning,
                                              "Number of media packets sent in invalid directions");
static LogCounter COUNTER_QUEUE_FULL(plog::warning,
                                     "Number of media packets dropped due to a full queue");

Track::Track(weak_ptr<PeerConnection> pc, Description::Media desc)
    : mPeerConnection(pc), mMediaDescription(std::move(desc)),
      mRecvQueue(RECV_QUEUE_LIMIT, [](const message_ptr &m) { return m->size(); }) {

	// Discard messages by default if track is send only
	if (mMediaDescription.direction() == Description::Direction::SendOnly)
		messageCallback = [](message_variant) {};
}

Track::~Track() {
	PLOG_VERBOSE << "Destroying Track";
	try {
		close();
	} catch (const std::exception &e) {
		PLOG_ERROR << e.what();
	}
}

string Track::mid() const {
	std::shared_lock lock(mDescriptionMutex);
	return mMediaDescription.mid();
}

Description::Direction Track::direction() const {
	std::shared_lock lock(mDescriptionMutex);
	return mMediaDescription.direction();
}

Description::Media Track::description() const {
	std::shared_lock lock(mDescriptionMutex);
	return mMediaDescription;
}

void Track::setDescription(Description::Media desc) {
	{
		std::unique_lock lock(mDescriptionMutex);
		if (desc.mid() != mMediaDescription.mid())
			throw std::logic_error("Media description mid does not match track mid");

		mMediaDescription = std::move(desc);
	}

	if (auto handler = getMediaHandler())
		handler->media(description());
}

void Track::close() {
	PLOG_VERBOSE << "Closing Track";

	if (!mIsClosed.exchange(true))
		triggerClosed();

	setMediaHandler(nullptr);
	resetCallbacks();
}

optional<message_variant> Track::receive() {
	if (auto next = mRecvQueue.pop()) {
		message_ptr message = *next;
		if (message->type == Message::Control)
			return to_variant(**next); // The same message may be frowarded into multiple Tracks
		else
			return to_variant(std::move(*message));
	}
	return nullopt;
}

optional<message_variant> Track::peek() {
	if (auto next = mRecvQueue.peek()) {
		message_ptr message = *next;
		if (message->type == Message::Control)
			return to_variant(**next); // The same message may be forwarded into multiple Tracks
		else
			return to_variant(std::move(*message));
	}
	return nullopt;
}

size_t Track::availableAmount() const { return mRecvQueue.amount(); }

bool Track::isOpen(void) const {
#if RTC_ENABLE_MEDIA
	return !mIsClosed && getTransport();
#else
	return false;
#endif
}

bool Track::isClosed(void) const { return mIsClosed; }

size_t Track::maxMessageSize() const {
	optional<size_t> mtu;
	if (auto pc = mPeerConnection.lock())
		mtu = pc->config.mtu;

	return mtu.value_or(DEFAULT_MTU) - 12 - 8 - 40; // SRTP/UDP/IPv6
}

#if RTC_ENABLE_MEDIA
void Track::open(shared_ptr<DtlsSrtpTransport> transport) {
	{
		std::lock_guard lock(mTransportMutex);
		mTransport = transport;
	}

	if (!mIsClosed)
		triggerOpen();
}

shared_ptr<DtlsSrtpTransport> Track::getTransport() const {
	std::lock_guard lock(mTransportMutex);
	return mTransport.lock();
}

#endif

void Track::incoming(message_ptr message) {
	if (!message)
		return;

	auto dir = direction();
	if ((dir == Description::Direction::SendOnly || dir == Description::Direction::Inactive) &&
	    message->type != Message::Control) {
		COUNTER_MEDIA_BAD_DIRECTION++;
		return;
	}

	if(auto handler = getMediaHandler()) {
		message = handler->incomingChain(std::move(message));
		if (!message)
			return;
	}

	// Tail drop if queue is full
	if (mRecvQueue.full()) {
		COUNTER_QUEUE_FULL++;
		return;
	}

	mRecvQueue.push(message);
	triggerAvailable(mRecvQueue.size());
}

bool Track::outgoing(message_ptr message) {
	if (mIsClosed)
		throw std::runtime_error("Track is closed");

	auto handler = getMediaHandler();

	// If there is no handler, the track expects RTP or RTCP packets
	if (!handler && IsRtcp(*message))
		message->type = Message::Control; // to allow sending RTCP packets irrelevant of direction

	auto dir = direction();
	if ((dir == Description::Direction::RecvOnly || dir == Description::Direction::Inactive) &&
	    message->type != Message::Control) {
		COUNTER_MEDIA_BAD_DIRECTION++;
		return false;
	}

	if (handler) {
		message = handler->outgoingChain(std::move(message));
		if (!message)
			return false;
	}

	return transportSend(message);
}

bool Track::transportSend([[maybe_unused]] message_ptr message) {
#if RTC_ENABLE_MEDIA
	auto transport = getTransport();
	if (!transport)
		throw std::runtime_error("Track is closed");

	// Set recommended medium-priority DSCP value
	// See https://www.rfc-editor.org/rfc/rfc8837.html#section-5
	if (mMediaDescription.type() == "audio")
		message->dscp = 46; // EF: Expedited Forwarding
	else
		message->dscp = 36; // AF42: Assured Forwarding class 4, medium drop probability

	return transport->sendMedia(message);
#else
	throw std::runtime_error("Track is disabled (not compiled with media support)");
#endif
}

void Track::setMediaHandler(shared_ptr<MediaHandler> handler) {
	auto oldHandler = std::atomic_exchange(&mMediaHandler, handler);
	if (oldHandler) {
		oldHandler->last()->onOutgoing(nullptr);
	}
	if (handler) {
		handler->last()->onOutgoing(weak_bind(&Track::transportSend, this, std::placeholders::_1));
		handler->media(description());
	}
}

shared_ptr<MediaHandler> Track::getMediaHandler() {
	return std::atomic_load(&mMediaHandler);
}

} // namespace rtc::impl
