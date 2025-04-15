/*
 * =====================================================================================
 *
 *       Filename:  QuoteEvent.hpp
 *
 *    Description:  QuoteEvent
 *
 *        Version:  1.0
 *        Created:  2025/01/24 09时56分32秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  vyouzhi (vz), vyouzhi@gmail.com
 *   Organization:  Etomc2.com
 *        LICENSE:  BSD-3-Clause license
 *
 *  Copyright (c) 2019-2022, vyouzhi
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 *  * Neither the name of vyouzhi and/or the DataFrame nor the
 *  names of its contributors may be used to endorse or promote products
 *  derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL vyouzhi BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * =====================================================================================
 */

#ifndef QUOTEEVENT_INC
#define QUOTEEVENT_INC

#include <cstdint>
#include <map>
#include <string>

#include "FeedPack/ZtsQuote.hpp"
#include "FileUtils.h"
#include "kafka/producer.hpp"
#include "utility/DataFormat.hpp"
namespace e2q {

inline int keepRunning = 1;

/*
 * ================================
 *        Class:  QuoteEvent
 *  Description: QuoteApi
 * ================================
 */
class QuoteEvent {
public:
    /* =============  LIFECYCLE     =================== */
    QuoteEvent(FileUtils* futils) : _futils(futils)
    {
        init();
    }; /* constructor */
    ~QuoteEvent()
    {
        if (_producer != nullptr) {
            delete _producer;
        }
    };
    /* =============  ACCESSORS     =================== */

    /* =============  MUTATORS      =================== */
    void run();
    /* =============  OPERATORS     =================== */

protected:
    /* =============  METHODS       =================== */

    /* =============  DATA MEMBERS  =================== */

private:
    /* =============  METHODS       =================== */
    void init();
    void head();
    void quote();
    /* =============  DATA MEMBERS  =================== */
    XTP::API::QuoteApi* _pQuoteApi{nullptr};
    ZtsQuoteSpi* _quote_spi{nullptr};
    FileUtils* _futils;

    XTP_PROTOCOL_TYPE quote_protocol = XTP_PROTOCOL_UDP;
    std::map<uint64_t, uint64_t> map_session;
    uint64_t* session_arrary = NULL;

    std::string quote_username;
    std::string quote_password;
    std::string quote_server_ip;
    int quote_server_port;

    int client_id;
    std::string account_key;
    std::string filepath;
    int32_t quote_buffer_size = 0;
    int32_t heat_beat_interval;

    std::string kafkaserver;
    int kafkaport;

    std::string _head;

    std::string topic = "fix-events";
    std::string exit_data = "E";
    Producer* _producer{nullptr};

    DataFormat _DataFmt;
}; /* -----  end of class QuoteEvent  ----- */

}  // namespace e2q
#endif /* ----- #ifndef QUOTEEVENT_INC  ----- */
