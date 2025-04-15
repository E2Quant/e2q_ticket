/*
 * =====================================================================================
 *
 *       Filename:  ZtsQuote.cpp
 *
 *    Description:  ZtsQuote
 *
 *        Version:  1.0
 *        Created:  2025/01/23 14时42分18秒
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
 * =====================================================================================
 */

#include "FeedPack/ZtsQuote.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

#include "Log.hpp"
#include "Toolkit/UtilTime.hpp"
#include "kafka/protocol/nbo.hpp"
namespace e2q {

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ZtsQuoteSpi::addSymbol
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void ZtsQuoteSpi::addSymbol(std::size_t cfi, std::string name)
{
    _symbols.insert({name, cfi});
} /* -----  end of function ZtsQuoteSpi::addSymbol  ----- */
/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ZtsQuoteSpi::OnDisconnected
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 *   // 断线后，可以重新连接
 *   // 重新连接成功后，需要重新向服务器发起订阅请求
 * ============================================
 */
void ZtsQuoteSpi::OnDisconnected(int reason)
{
    if (reason == 10200006) {
        log::info(" reason:", reason);
        return;
    }

    log::info("Disconnect from quote server. ");

    if (_fun_ptr) {
        _fun_ptr();
    }

} /* -----  end of function ZtsQuoteSpi::OnDisconnected  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ZtsQuoteSpi::OnSubMarketData
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void ZtsQuoteSpi::OnSubMarketData(XTPST *ticker, XTPRI *error_info,
                                  bool is_last)
{
    if (error_info && error_info->error_id != 0) {
        std::string msg =
            log::format("error_id:%d, msg:%s\n", error_info->error_id,
                        error_info->error_msg);
        log::bug(msg);
        return;
    }
    log::info("OnRspSubMarketData ticker:", std::string(ticker->ticker));
} /* -----  end of function ZtsQuoteSpi::OnSubMarketData  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ZtsQuoteSpi::OnUnSubMarketData
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void ZtsQuoteSpi::OnUnSubMarketData(XTPST *ticker, XTPRI *error_info,
                                    bool is_last)
{
    log::echo("UnSubMarketData");
} /* -----  end of function ZtsQuoteSpi::OnUnSubMarketData  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ZtsQuoteSpi::OnDepthMarketData
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *  T%d:%d@%d,%d,%d,%.2f,%d,%d#%d,%d,%d,%.2f,%d,%d....
 *  Tticker_number:unix_time@frame,side,qty,price,message
 * type,cficode#frame,side,qty,price,message type,cficode ...
 * ============================================
 */
void ZtsQuoteSpi::OnDepthMarketData(XTPMD *market_data, int64_t bid1_qty[],
                                    int32_t bid1_count, int32_t max_bid1_count,
                                    int64_t ask1_qty[], int32_t ask1_count,
                                    int32_t max_ask1_count)
{
    if (market_data == nullptr) {
        log::info(" market_data == nullptr");
        return;
    }
    std::string status =
        log::format("ticker_status:%s\n", market_data->ticker_status);

    if (*market_data->ticker_status != 'T') {
        log::bug(status);
        return;
    }

    // log::info(status);

    // std::cout << " depth " << std::endl;
    // if (bid1_count > 0) {
    //     for (int32_t m = 0; m < bid1_count; m++) {
    //         std::cout << bid1_qty[m] << " .";
    //     }
    //     std::cout << " \n";
    // }
    const char fmt[] = "%Y%m%d%H%M%S000";
    UtilTime ut;
    std::string timestr = std::to_string(market_data->data_time);
    _millisecond unix_time = ut.strtostamp(timestr, fmt);

    _millisecond now = ut.time();

    if (market_data != nullptr) {
        std::cout << "open price:" << market_data->open_price
                  << " last_price:" << market_data->last_price
                  << " close price:" << market_data->close_price
                  << " date_time:" << market_data->data_time
                  << " unix time:" << unix_time
                  << " now:" << ut.stamptostr(now, fmt) << std::endl;
    }
    std::string sym = std::string(market_data->ticker);
    std::size_t cfi = 0;
    if (_symbols.count(sym) == 1) {
        cfi = _symbols.at(sym);
    }
    if (src_ptr == nullptr) {
        mlen = _DataFmt.TickSize();
        src_ptr = (char *)calloc(mlen, sizeof(char *));
    }

    _DataFmt.Tick(unix_time);
    std::uint64_t price = (std::uint64_t)(market_data->last_price * _deci);
    _DataFmt.Stock(src_ptr, _frame, market_data->qty, price, cfi);
    _producer->data(src_ptr, mlen, _topic, nullptr);

    _DataFmt.Stock(src_ptr, _frame, market_data->qty, price,
                   _DataFmt.IndexCfiCode());
    _producer->data(src_ptr, mlen, _topic, nullptr);

} /* -----  end of function ZtsQuoteSpi::OnDepthMarketData  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ZtsQuoteSpi::OnSubOrderBook
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void ZtsQuoteSpi::OnSubOrderBook(XTPST *ticker, XTPRI *error_info, bool is_last)
{
} /* -----  end of function ZtsQuoteSpi::OnSubOrderBook  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ZtsQuoteSpi::OnUnSubOrderBook
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void ZtsQuoteSpi::OnUnSubOrderBook(XTPST *ticker, XTPRI *error_info,
                                   bool is_last)
{
} /* -----  end of function ZtsQuoteSpi::OnUnSubOrderBook  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ZtsQuoteSpi::OnSubTickByTick
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void ZtsQuoteSpi::OnSubTickByTick(XTPST *ticker, XTPRI *error_info,
                                  bool is_last)
{
} /* -----  end of function ZtsQuoteSpi::OnSubTickByTick  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ZtsQuoteSpi::OnUnSubTickByTick
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void ZtsQuoteSpi::OnUnSubTickByTick(XTPST *ticker, XTPRI *error_info,
                                    bool is_last)
{
} /* -----  end of function ZtsQuoteSpi::OnUnSubTickByTick  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ZtsQuoteSpi::OnOrderBook
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void ZtsQuoteSpi::OnOrderBook(XTPOB *order_book) {
} /* -----  end of function ZtsQuoteSpi::OnOrderBook  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ZtsQuoteSpi::OnTickByTick
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void ZtsQuoteSpi::OnTickByTick(XTPTBT *tbt_data) {
} /* -----  end of function ZtsQuoteSpi::OnTickByTick  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ZtsQuoteSpi::OnQueryAllTickers
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void ZtsQuoteSpi::OnQueryAllTickers(XTPQSI *ticker_info, XTPRI *error_info,
                                    bool is_last)
{
} /* -----  end of function ZtsQuoteSpi::OnQueryAllTickers  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ZtsQuoteSpi::OnQueryTickersPriceInfo
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void ZtsQuoteSpi::OnQueryTickersPriceInfo(XTPTPI *ticker_info,
                                          XTPRI *error_info, bool is_last)
{
} /* -----  end of function ZtsQuoteSpi::OnQueryTickersPriceInfo  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ZtsQuoteSpi::OnSubscribeAllMarketData
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void ZtsQuoteSpi::OnSubscribeAllMarketData(XTP_EXCHANGE_TYPE exchange_id,
                                           XTPRI *error_info)
{
} /* -----  end of function ZtsQuoteSpi::OnSubscribeAllMarketData  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ZtsQuoteSpi::OnUnSubscribeAllMarketData
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void ZtsQuoteSpi::OnUnSubscribeAllMarketData(XTP_EXCHANGE_TYPE exchange_id,
                                             XTPRI *error_info)
{
} /* -----  end of function ZtsQuoteSpi::OnUnSubscribeAllMarketData  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ZtsQuoteSpi::OnSubscribeAllOrderBook
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void ZtsQuoteSpi::OnSubscribeAllOrderBook(XTP_EXCHANGE_TYPE exchange_id,
                                          XTPRI *error_info)
{
} /* -----  end of function ZtsQuoteSpi::OnSubscribeAllOrderBook  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ZtsQuoteSpi::OnUnSubscribeAllOrderBook
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void ZtsQuoteSpi::OnUnSubscribeAllOrderBook(XTP_EXCHANGE_TYPE exchange_id,
                                            XTPRI *error_info)
{
} /* -----  end of function ZtsQuoteSpi::OnUnSubscribeAllOrderBook  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ZtsQuoteSpi::OnSubscribeAllTickByTick
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void ZtsQuoteSpi::OnSubscribeAllTickByTick(XTP_EXCHANGE_TYPE exchange_id,
                                           XTPRI *error_info)
{
} /* -----  end of function ZtsQuoteSpi::OnSubscribeAllTickByTick  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ZtsQuoteSpi::OnUnSubscribeAllTickByTick
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void ZtsQuoteSpi::OnUnSubscribeAllTickByTick(XTP_EXCHANGE_TYPE exchange_id,
                                             XTPRI *error_info)
{
} /* -----  end of function ZtsQuoteSpi::OnUnSubscribeAllTickByTick  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ZtsQuoteSpi::OnSubscribeAllOptionMarketData
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void ZtsQuoteSpi::OnSubscribeAllOptionMarketData(XTP_EXCHANGE_TYPE exchange_id,
                                                 XTPRI *error_info)
{
} /* -----  end of function ZtsQuoteSpi::OnSubscribeAllOptionMarketData  -----
   */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ZtsQuoteSpi::OnUnSubscribeAllOptionMarketData
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void ZtsQuoteSpi::OnUnSubscribeAllOptionMarketData(
    XTP_EXCHANGE_TYPE exchange_id, XTPRI *error_info)
{
} /* -----  end of function ZtsQuoteSpi::OnUnSubscribeAllOptionMarketData  -----
   */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ZtsQuoteSpi::OnSubscribeAllOptionOrderBook
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void ZtsQuoteSpi::OnSubscribeAllOptionOrderBook(XTP_EXCHANGE_TYPE exchange_id,
                                                XTPRI *error_info)
{
} /* -----  end of function ZtsQuoteSpi::OnSubscribeAllOptionOrderBook  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ZtsQuoteSpi::OnUnSubscribeAllOptionOrderBook
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void ZtsQuoteSpi::OnUnSubscribeAllOptionOrderBook(XTP_EXCHANGE_TYPE exchange_id,
                                                  XTPRI *error_info)
{
} /* -----  end of function ZtsQuoteSpi::OnUnSubscribeAllOptionOrderBook  -----
   */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ZtsQuoteSpi::OnSubscribeAllOptionTickByTick
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void ZtsQuoteSpi::OnSubscribeAllOptionTickByTick(XTP_EXCHANGE_TYPE exchange_id,
                                                 XTPRI *error_info)
{
} /* -----  end of function ZtsQuoteSpi::OnSubscribeAllOptionTickByTick  -----
   */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ZtsQuoteSpi::OnUnSubscribeAllOptionTickByTick
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void ZtsQuoteSpi::OnUnSubscribeAllOptionTickByTick(
    XTP_EXCHANGE_TYPE exchange_id, XTPRI *error_info)
{
} /* -----  end of function ZtsQuoteSpi::OnUnSubscribeAllOptionTickByTick  -----
   */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ZtsQuoteSpi::OnQueryAllTickersFullInfo
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void ZtsQuoteSpi::OnQueryAllTickersFullInfo(XTPQFI *ticker_info,
                                            XTPRI *error_info, bool is_last)
{
} /* -----  end of function ZtsQuoteSpi::OnQueryAllTickersFullInfo  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ZtsQuoteSpi::OnRebuildQuoteServerDisconnected
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void ZtsQuoteSpi::OnRebuildQuoteServerDisconnected(int reason) {
} /* -----  end of function ZtsQuoteSpi::OnRebuildQuoteServerDisconnected  -----
   */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ZtsQuoteSpi::OnRequestRebuildQuote
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void ZtsQuoteSpi::OnRequestRebuildQuote(
    XTPQuoteRebuildResultRsp *rebuild_result)
{
} /* -----  end of function ZtsQuoteSpi::OnRequestRebuildQuote  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ZtsQuoteSpi::OnRebuildTickByTick
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void ZtsQuoteSpi::OnRebuildTickByTick(XTPTBT *tbt_data) {
} /* -----  end of function ZtsQuoteSpi::OnRebuildTickByTick  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ZtsQuoteSpi::OnRebuildMarketData
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void ZtsQuoteSpi::OnRebuildMarketData(XTPMD *md_data) {
} /* -----  end of function ZtsQuoteSpi::OnRebuildMarketData  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ZtsQuoteSpi::IsErrorRspInfo
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
bool ZtsQuoteSpi::IsErrorRspInfo(XTPRI *pRspInfo)
{
    return false;
} /* -----  end of function ZtsQuoteSpi::IsErrorRspInfo  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ZtsQuoteSpi::callBack
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void ZtsQuoteSpi::callBack(func_type func)
{
    _fun_ptr = std::move(func);
} /* -----  end of function ZtsQuoteSpi::callBack  ----- */
}  // namespace e2q
