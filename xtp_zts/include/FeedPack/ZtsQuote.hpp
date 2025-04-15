/*
 * =====================================================================================
 *
 *       Filename:  ZtsQuote.hpp
 *
 *    Description:  ZtsQuote
 *
 *        Version:  1.0
 *        Created:  2025/01/23 14时35分53秒
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

#ifndef ZTSQUOTE_INC
#define ZTSQUOTE_INC

#include <sys/timeb.h>

#include <cstddef>
#include <cstdlib>
#include <functional>
#include <map>
#include <string>

#include "kafka/producer.hpp"
#include "utility/DataFormat.hpp"
#include "xtp_quote_api.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif  // _WIN32

using namespace XTP::API;
namespace e2q {

using func_type = std::function<void()>;
/*
 * ================================
 *        Class:  ZtsQuoteSpi
 *  Description:
 * ================================
 */
class ZtsQuoteSpi : public QuoteSpi {
public:
    /* =============  LIFECYCLE     =================== */
    ZtsQuoteSpi(Producer *p) : _producer(p) {}; /* constructor */
    ~ZtsQuoteSpi()
    {
        if (src_ptr != nullptr) {
            free(src_ptr);
            src_ptr = nullptr;
        }
    };
    /* =============  ACCESSORS     =================== */

    void addSymbol(std::size_t, std::string);
    /* =============  MUTATORS      =================== */
    virtual void OnDisconnected(int reason);

    virtual void OnSubMarketData(XTPST *ticker, XTPRI *error_info,
                                 bool is_last);
    virtual void OnUnSubMarketData(XTPST *ticker, XTPRI *error_info,
                                   bool is_last);
    virtual void OnDepthMarketData(XTPMD *market_data, int64_t bid1_qty[],
                                   int32_t bid1_count, int32_t max_bid1_count,
                                   int64_t ask1_qty[], int32_t ask1_count,
                                   int32_t max_ask1_count);
    virtual void OnSubOrderBook(XTPST *ticker, XTPRI *error_info, bool is_last);
    virtual void OnUnSubOrderBook(XTPST *ticker, XTPRI *error_info,
                                  bool is_last);
    virtual void OnSubTickByTick(XTPST *ticker, XTPRI *error_info,
                                 bool is_last);
    virtual void OnUnSubTickByTick(XTPST *ticker, XTPRI *error_info,
                                   bool is_last);
    virtual void OnOrderBook(XTPOB *order_book);
    virtual void OnTickByTick(XTPTBT *tbt_data);
    virtual void OnQueryAllTickers(XTPQSI *ticker_info, XTPRI *error_info,
                                   bool is_last);
    virtual void OnQueryTickersPriceInfo(XTPTPI *ticker_info, XTPRI *error_info,
                                         bool is_last);
    virtual void OnSubscribeAllMarketData(XTP_EXCHANGE_TYPE exchange_id,
                                          XTPRI *error_info);
    virtual void OnUnSubscribeAllMarketData(XTP_EXCHANGE_TYPE exchange_id,
                                            XTPRI *error_info);
    virtual void OnSubscribeAllOrderBook(XTP_EXCHANGE_TYPE exchange_id,
                                         XTPRI *error_info);
    virtual void OnUnSubscribeAllOrderBook(XTP_EXCHANGE_TYPE exchange_id,
                                           XTPRI *error_info);
    virtual void OnSubscribeAllTickByTick(XTP_EXCHANGE_TYPE exchange_id,
                                          XTPRI *error_info);
    virtual void OnUnSubscribeAllTickByTick(XTP_EXCHANGE_TYPE exchange_id,
                                            XTPRI *error_info);
    virtual void OnSubscribeAllOptionMarketData(XTP_EXCHANGE_TYPE exchange_id,
                                                XTPRI *error_info);
    virtual void OnUnSubscribeAllOptionMarketData(XTP_EXCHANGE_TYPE exchange_id,
                                                  XTPRI *error_info);
    virtual void OnSubscribeAllOptionOrderBook(XTP_EXCHANGE_TYPE exchange_id,
                                               XTPRI *error_info);
    virtual void OnUnSubscribeAllOptionOrderBook(XTP_EXCHANGE_TYPE exchange_id,
                                                 XTPRI *error_info);
    virtual void OnSubscribeAllOptionTickByTick(XTP_EXCHANGE_TYPE exchange_id,
                                                XTPRI *error_info);
    virtual void OnUnSubscribeAllOptionTickByTick(XTP_EXCHANGE_TYPE exchange_id,
                                                  XTPRI *error_info);

    virtual void OnQueryAllTickersFullInfo(XTPQFI *ticker_info,
                                           XTPRI *error_info, bool is_last);

    // 以下为行情回补涉及的回调函数
    virtual void OnRebuildQuoteServerDisconnected(int reason);
    virtual void OnRequestRebuildQuote(
        XTPQuoteRebuildResultRsp *rebuild_result);
    virtual void OnRebuildTickByTick(XTPTBT *tbt_data);
    virtual void OnRebuildMarketData(XTPMD *md_data);

    void callBack(func_type);
    /* =============  OPERATORS     =================== */

protected:
    /* =============  METHODS       =================== */

    /* =============  DATA MEMBERS  =================== */

private:
    /* =============  METHODS       =================== */
    bool IsErrorRspInfo(XTPRI *pRspInfo);
    /* =============  DATA MEMBERS  =================== */

    Producer *_producer{nullptr};
    std::size_t _message_type = 1;
    std::map<std::string, std::size_t> _symbols;

    int _frame = 1;
    std::string _topic = "fix-events";
    DataFormat _DataFmt;
    char *src_ptr = nullptr;
    std::size_t mlen = 0;

    func_type _fun_ptr{nullptr};
}; /* -----  end of class ZtsQuoteSpi  ----- */

}  // namespace e2q
#endif /* ----- #ifndef ZTSQUOTE_INC  ----- */
