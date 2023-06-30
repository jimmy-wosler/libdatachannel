/**
 * Copyright (c) 2020 Filip Klembara (in2core)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef RTC_RTCP_SENDER_REPORTABLE_H
#define RTC_RTCP_SENDER_REPORTABLE_H

#if RTC_ENABLE_MEDIA

#include "mediahandler.hpp"
#include "rtppacketizationconfig.hpp"
#include "rtp.hpp"

namespace rtc {

class RTC_CPP_EXPORT RtcpSrReporter final : public MediaHandler {
	void addToReport(RtpHeader *rtp, uint32_t rtpSize);
	message_ptr getSenderReport(uint32_t timestamp);

public:
	// RTP configuration
	const shared_ptr<RtpPacketizationConfig> rtpConfig;

	RtcpSrReporter(shared_ptr<RtpPacketizationConfig> rtpConfig);

	uint32_t lastReportedTimestamp() const;
	void setNeedsToReport();

private:
	message_ptr incoming(message_ptr message) override;
	message_ptr outgoing(message_ptr message) override;

	uint32_t mPacketCount = 0;
	uint32_t mPayloadOctets = 0;
	uint32_t mLastReportedTimestamp = 0;
	bool mNeedsToReport = false;
};

} // namespace rtc

#endif /* RTC_ENABLE_MEDIA */

#endif /* RTC_RTCP_SENDER_REPORTABLE_H */
