/**
 * Copyright (c) 2020 Filip Klembara (in2core)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#if RTC_ENABLE_MEDIA

#include "rtcpnackresponder.hpp"
#include "rtp.hpp"

#include "impl/internals.hpp"

#include <cassert>

namespace rtc {

RtcpNackResponder::Storage::Element::Element(binary_ptr packet, uint16_t sequenceNumber,
                                             shared_ptr<Element> next)
    : packet(packet), sequenceNumber(sequenceNumber), next(next) {}

unsigned RtcpNackResponder::Storage::size() { return unsigned(storage.size()); }

RtcpNackResponder::Storage::Storage(unsigned _maximumSize) : maximumSize(_maximumSize) {
	assert(maximumSize > 0);
	storage.reserve(maximumSize);
}

optional<binary_ptr> RtcpNackResponder::Storage::get(uint16_t sequenceNumber) {
	auto position = storage.find(sequenceNumber);
	return position != storage.end() ? std::make_optional(storage.at(sequenceNumber)->packet)
	                                 : nullopt;
}

void RtcpNackResponder::Storage::store(binary_ptr packet) {
	if (!packet || packet->size() < 12) {
		return;
	}
	auto rtp = reinterpret_cast<RtpHeader *>(packet->data());
	auto sequenceNumber = rtp->seqNumber();

	assert((storage.empty() && !oldest && !newest) || (!storage.empty() && oldest && newest));

	if (size() == 0) {
		newest = std::make_shared<Element>(packet, sequenceNumber);
		oldest = newest;
	} else {
		auto current = std::make_shared<Element>(packet, sequenceNumber);
		newest->next = current;
		newest = current;
	}

	storage.emplace(sequenceNumber, newest);

	if (size() > maximumSize) {
		assert(oldest);
		if (oldest) {
			storage.erase(oldest->sequenceNumber);
			oldest = oldest->next;
		}
	}
}

RtcpNackResponder::RtcpNackResponder(unsigned maxStoredPacketCount)
    : mStorage(std::make_shared<Storage>(maxStoredPacketCount)) {}

message_ptr RtcpNackResponder::incoming(message_ptr message) {
	if (IsRtcp(*message)) {
		size_t p = 0;
		while (p + sizeof(RtcpNack) <= message->size()) {
			auto nack = reinterpret_cast<RtcpNack *>(message->data() + p);
			p += nack->header.header.lengthInBytes();
			if (p > message->size())
				break;

			// check if RTCP is NACK
			if (nack->header.header.payloadType() != 205 || nack->header.header.reportCount() != 1)
				continue;

			unsigned int fieldsCount = nack->getSeqNoCount();
			std::vector<uint16_t> missingSequenceNumbers;
			for (unsigned int i = 0; i < fieldsCount; i++) {
				auto field = nack->parts[i];
				auto newMissingSeqenceNumbers = field.getSequenceNumbers();
				missingSequenceNumbers.insert(missingSequenceNumbers.end(),
				                              newMissingSeqenceNumbers.begin(),
				                              newMissingSeqenceNumbers.end());
			}

			for (auto sequenceNumber : missingSequenceNumbers) {
				if (auto optPacket = mStorage->get(sequenceNumber))
					send(make_message(*optPacket.value()));
			}
		}
	}

	return message;
}

message_ptr RtcpNackResponder::outgoing(message_ptr message) {
	if (!IsRtcp(*message))
		mStorage->store(message);

	return message;
}

} // namespace rtc

#endif /* RTC_ENABLE_MEDIA */
