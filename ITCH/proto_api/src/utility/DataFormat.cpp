/*
 * =====================================================================================
 *
 *       Filename:  DataFormat.cpp
 *
 *    Description:  DataFormat
 *
 *        Version:  1.0
 *        Created:  2025/02/06 10时26分54秒
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
#include "utility/DataFormat.hpp"

#include <string.h>

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>

#include "Log.hpp"
#include "Toolkit/UtilTime.hpp"
#include "kafka/protocol/nbo.hpp"
#include "kafka/protocol/proto.hpp"
namespace e2q {

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  DataFormat::IndexCfiCode
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
std::size_t DataFormat::IndexCfiCode()
{
    return 0;
} /* -----  end of function DataFormat::IndexCfiCode  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  DataFormat::thash
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
std::size_t DataFormat::thash()
{
    return _symId.StoreId();
} /* -----  end of function DataFormat::thash  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  DataFormat::SystemInit
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
std::size_t DataFormat::SystemInit()
{
    std::size_t mlen = 1;
    std::size_t stock_len = fldsiz(SystemInitMessage, Stock);
    mlen += stock_len;
    mlen += fldsiz(SystemInitMessage, CfiCode);
    mlen += fldsiz(SystemInitMessage, Itype);
    mlen += fldsiz(SystemInitMessage, OfferTime);
    return mlen;
} /* -----  end of function DataFormat::SystemInit  ----- */
/*
 * ===  FUNCTION  =============================
 *
 *         Name:  DataFormat::init
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void DataFormat::init(char* ptr, std::uint32_t cficode, std::string symbol,
                      char type)
{
    using namespace e2q;

    e2q::SystemInitMessage sim;

    if (ptr == nullptr) {
        return;
    }

    std::size_t stock_len = fldsiz(SystemInitMessage, Stock);

    sim.MsgType = e2l_pro_t::INIT;
    stock_len--;

    sim.CfiCode = cficode;
    sim.Itype = type;
    sim.Aligned = aligned_t::UNDER;
    sim.OfferTime = 0;

    if (type == 'i') {
        sim.Aligned = aligned_t::PULL;
        sim.OfferTime = std::uint32_t(_tick_sleep_time * _deci);
    }

    std::size_t idx = 0;

    *(ptr + idx) = sim.MsgType;
    idx++;

    memcpy((ptr + idx), symbol.c_str(), stock_len);

    idx += stock_len;
    idx += serialize_uint_t((ptr + idx), sim.CfiCode);

    *(ptr + idx) = sim.Itype;
    idx++;

    idx += serialize_uint_t((ptr + idx), sim.OfferTime);
    *(ptr + idx) = sim.Aligned;

} /* -----  end of function DataFormat::init  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  DataFormat::add_symbol
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
std::size_t DataFormat::add_symbol(char* ptr, std::string& sym)
{
    std::size_t symId = thash();

    init(ptr, symId, sym, 't');

    return symId;
} /* -----  end of function DataFormat::add_symbol  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  DataFormat::Index
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void DataFormat::Index(char* ptr, std::string& sym)
{
    init(ptr, 0, sym, 'i');

} /* -----  end of function DataFormat::Index  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  DataFormat::TickSize
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
std::size_t DataFormat::TickSize()
{
    using namespace e2q;
    MarketTickMessage mtm;
    std::size_t mlen = 2;
    mlen += fldsiz(MarketTickMessage, CfiCode);
    mlen += fldsiz(MarketTickMessage, unix_time);
    mlen += fldsiz(MarketTickMessage, frame);
    mlen++;  // side
    mlen += fldsiz(MarketTickMessage, price) - 2;
    mlen += fldsiz(MarketTickMessage, qty) - 2;
    mlen += fldsiz(MarketTickMessage, number);
    return mlen;
} /* -----  end of function DataFormat::TickSize  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  DataFormat::Stock
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void DataFormat::Stock(char* ptr, int frame, double qty, double price,
                       std::size_t cficode)
{
    using namespace e2q;

    MarketTickMessage mtm;
    if (ptr == nullptr) {
        return;
    }
    mtm.MsgType = e2l_pro_t::TICK;
    mtm.CfiCode = cficode;
    mtm.unix_time = _unix_time;
    mtm.frame = frame;
    mtm.price = price;
    mtm.qty = qty;
    mtm.number = _tick_num;

    std::size_t idx = 0;

    *(ptr + idx) = mtm.MsgType;
    idx++;

    idx += serialize_uint_t((ptr + idx), mtm.CfiCode);

    idx += e2q::serialize_uint_t(ptr + idx, mtm.unix_time);
    idx += serialize_uint_t((ptr + idx), mtm.frame);
    *(ptr + idx) = mtm.side;
    idx++;

    idx += e2q::serialize_uint_t<std::uint64_t, 2>(ptr + idx, mtm.price);
    idx += e2q::serialize_uint_t<std::uint64_t, 2>(ptr + idx, mtm.qty);
    idx += serialize_uint_t((ptr + idx), _tick_num);

    if (cficode == 0) {
        mtm.Aligned = aligned_t::PULL;
        if (!_debug) {
            _tick_num++;
        }
    }
    else {
        mtm.Aligned = aligned_t::UNDER;
    }

    *(ptr + idx) = mtm.Aligned;
} /* -----  end of function DataFormat::Stock  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  DataFormat::Tick
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void DataFormat::Tick(_millisecond unix_time)
{
    _unix_time = unix_time;
} /* -----  end of function DataFormat::Tick  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  DataFormat::xdxr
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *  以后再优化
 * ============================================
 */
void DataFormat::xdxr(std::string&, std::size_t, std::string&, int) {

} /* -----  end of function DataFormat::xdxr  ----- */
}  // namespace e2q
