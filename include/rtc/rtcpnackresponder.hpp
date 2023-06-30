/**
 * Copyright (c) 2020 Filip Klembara (in2core)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef RTC_RTCP_NACK_RESPONDER_H
#define RTC_RTCP_NACK_RESPONDER_H

#if RTC_ENABLE_MEDIA

#include "mediahandler.hpp"

#include <queue>
#include <unordered_map>

namespace rtc {

class RTC_CPP_EXPORT RtcpNackResponder final : public MediaHandler {

	/// Packet storage
	class RTC_CPP_EXPORT Storage {

		/// Packet storage element
		struct RTC_CPP_EXPORT Element {
			Element(binary_ptr packet, uint16_t sequenceNumber, shared_ptr<Element> next = nullptr);
			const binary_ptr packet;
			const uint16_t sequenceNumber;
			/// Pointer to newer element
			shared_ptr<Element> next = nullptr;
		};

	private:
		/// Oldest packet in storage
		shared_ptr<Element> oldest = nullptr;
		/// Newest packet in storage
		shared_ptr<Element> newest = nullptr;

		/// Inner storage
		std::unordered_map<uint16_t, shared_ptr<Element>> storage{};

		/// Maximum storage size
		const unsigned maximumSize;

		/// Returns current size
		unsigned size();

	public:
		static const unsigned defaultMaximumSize = 512;

		Storage(unsigned _maximumSize);

		/// Returns packet with given sequence number
		optional<binary_ptr> get(uint16_t sequenceNumber);

		/// Stores packet
		/// @param packet Packet
		void store(binary_ptr packet);
	};

	const shared_ptr<Storage> mStorage;

public:
	RtcpNackResponder(unsigned maxStoredPacketCount = Storage::defaultMaximumSize);

	// Checks for RTCP NACK and handles it,
	message_ptr incoming(message_ptr message) override;

	// Stores RTP packets in internal storage
	message_ptr outgoing(message_ptr message) override;
};

} // namespace rtc

#endif /* RTC_ENABLE_MEDIA */

#endif /* RTC_RTCP_NACK_RESPONDER_H */
