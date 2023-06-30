/**
 * Copyright (c) 2020 Filip Klembara (in2core)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#if RTC_ENABLE_MEDIA

#include "opusrtppacketizer.hpp"

#include <cassert>

namespace rtc {

OpusRtpPacketizer::OpusRtpPacketizer(shared_ptr<RtpPacketizationConfig> rtpConfig)
    : RtpPacketizer(rtpConfig) {}

message_ptr OpusRtpPacketizer::incoming(message_ptr message) {
	return message;
}

message_ptr OpusRtpPacketizer::outgoing(message_ptr message) {
	return packetize(message, false);
}

} // namespace rtc

#endif /* RTC_ENABLE_MEDIA */
