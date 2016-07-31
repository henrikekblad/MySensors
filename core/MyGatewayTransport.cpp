/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Tomas Hozza <thozza@gmail.com>
 * Copyright (C) 2015  Tomas Hozza
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include "MyGatewayTransport.h"

extern bool transportSendRoute(MyMessage &message);
extern MyMessage _msg;

void gatewayPresent() {
	if (presentation) {
		#if !defined(MY_RADIO_FEATURE)
			present(NODE_SENSOR_ID, S_ARDUINO_NODE);
		#endif
		presentation();
	}
}

inline void gatewayTransportProcess() {
	if (gatewayTransportAvailable()) {
		_msg = gatewayTransportReceive();
		if (_msg.destination == GATEWAY_ADDRESS) {

			// Check if sender requests an ack back.
			if (mGetRequestAck(_msg)) {
				// Copy message
				_msgTmp = _msg;
				mSetRequestAck(_msgTmp, false); // Reply without ack flag (otherwise we would end up in an eternal loop)
				mSetAck(_msgTmp, true);
				_msgTmp.sender = _nc.nodeId;
				_msgTmp.destination = _msg.sender;
				gatewayTransportSend(_msgTmp);
			}
			if (mGetCommand(_msg) == C_INTERNAL) {
				if (_msg.type == I_VERSION) {
					// Request for version. Create the response
					gatewayTransportSend(buildGw(_msg, I_VERSION).set(MYSENSORS_LIBRARY_VERSION));
				#ifdef MY_INCLUSION_MODE_FEATURE
				} else if (_msg.type == I_INCLUSION_MODE) {
					// Request to change inclusion mode
					inclusionModeSet(atoi(_msg.data) == 1);
				#endif
				} else {
					_processInternalMessages();
				}
			} else {
				// Call incoming message callback if available
				if (receive) {
					receive(_msg);
				}
			}
		} else {
			#if defined(MY_RADIO_FEATURE)
				transportSendRoute(_msg);
			#endif
		}
	}
}
