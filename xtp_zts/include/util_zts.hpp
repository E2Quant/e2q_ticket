/*
 * =====================================================================================
 *
 *       Filename:  util_zts.hpp
 *
 *    Description:  util_zts
 *
 *        Version:  1.0
 *        Created:  2025/01/23 17时24分55秒
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

#ifndef UTIL_ZTS_INC
#define UTIL_ZTS_INC
#include <array>
#include <cstdint>
#include <functional>

#include "xquote_api_struct.h"
namespace e2q {
/**
 * 类型
 */
typedef int64_t SeqType;

/**
 * OMS Quote FeedData
 */
enum Trading {
    t_time = 0,
    t_frame,
    t_side,
    t_qty,
    t_price,
    t_msg,
    t_stock,
    t_adjprice
}; /* ----------  end of enum Trading  ---------- */

typedef enum Trading Trading;
#define trading_protocols 8

using TradType = std::function<void(std::array<SeqType, trading_protocols>&)>;

struct XTPSymbolInfo {
    XTP_EXCHANGE_TYPE exchange_id;          ///< 交易所代码
    char ticker[XTP_TICKER_LEN];            ///< 证券代码
    char ticker_name[XTP_TICKER_NAME_LEN];  ///< 证券名称

    /// 时间类，格式为YYYYMMDDHHMMSSsss
    /// 交易开始时间
    int64_t start_time;
    /// 交易结束时间
    int64_t expiration_time;

    /// 小数点后的数字
    int digits;
}; /* ----------  end of struct XTPSymbolInfo  ---------- */

typedef struct XTPSymbolInfo XTPSymbolInfo;

}  // namespace e2q
#endif /* ----- #ifndef UTIL_ZTS_INC  ----- */
