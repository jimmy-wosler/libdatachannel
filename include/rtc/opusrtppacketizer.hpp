/**
 * Copyright (c) 2020 Filip Klembara (in2core)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef RTC_OPUS_RTP_PACKETIZER_H
#define RTC_OPUS_RTP_PACKETIZER_H

#if RTC_ENABLE_MEDIA

#include "rtppacketizer.hpp"

namespace rtc {

// RTP packetizer for Opus
class RTC_CPP_EXPORT OpusRtpPacketizer final : public RtpPacketizer {
public:
	// default clock rate used in opus RTP communication
	inline static const uint32_t defaultClockRate = 48 * 1000;

	// Constructs opus packetizer with given RTP configuration.
	// @note RTP configuration is used in packetization process which may change some configuration
	// properties such as sequence number.
	// @param rtpConfig  RTP configuration
	OpusRtpPacketizer(shared_ptr<RtpPacketizationConfig> rtpConfig);

	message_ptr incoming(message_ptr message) override;
	message_ptr outgoing(message_ptr message) override;
};

// Dummy wrapper for backward compatibility
class RTC_CPP_EXPORT OpusPacketizationHandler final : public MediaHandler {
public:
	OpusPacketizationHandler(shared_ptr<OpusRtpPacketizer> packetizer)
	    : mPacketizer(std::move(packetizer)) {}

	inline message_ptr incoming(message_ptr message) override {
		return mPacketizer->incoming(std::move(message));
	}
	inline message_ptr outgoing(message_ptr message) override {
		return mPacketizer->outgoing(std::move(message));
	}

private:
	shared_ptr<OpusRtpPacketizer> mPacketizer;
};

} // namespace rtc

#endif /* RTC_ENABLE_MEDIA */

#endif /* RTC_OPUS_RTP_PACKETIZER_H */
