/*
 * =====================================================================================
 *
 *       Filename:  DataFormat.hpp
 *
 *    Description:  DataFormat
 *
 *        Version:  1.0
 *        Created:  2025/02/06 10时18分55秒
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

#ifndef DATAFORMAT_INC
#define DATAFORMAT_INC
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <thread>

#include "Toolkit/UtilTime.hpp"
#include "kafka/protocol/nbo.hpp"
namespace e2q {
#define SeqType std::int64_t

/**
 *  auto-increment type
 */
struct AutoIncrement {
    AutoIncrement() { init(); }
    AutoIncrement(SeqType i) { _storeId.store(i, std::memory_order_release); }

    ~AutoIncrement() {}
    SeqType StoreId()
    {
        SeqType inc = 1;
        return _storeId.fetch_add(inc, std::memory_order_release) + inc;
    }
    SeqType Id() { return _storeId.load(std::memory_order_acquire); }
    void init()
    {
        SeqType def = E2QCfiStart;
        _storeId.store(def, std::memory_order_release);
    }
    std::atomic_int64_t _storeId;  //
    std::thread::id _tid;          // std::thread::id
}; /* ----------  end of struct AutoIncrement  ---------- */
/*
 * ================================
 *        Class:  DataFormat
 *  Description:
 * ================================
 */
class DataFormat {
public:
    /* =============  LIFECYCLE     =================== */
    DataFormat() {}; /* constructor */

    /* =============  ACCESSORS     =================== */

    /* =============  MUTATORS      =================== */
    void Debug()
    {
        _debug = true;
        _tick_num = 0;
    };
    std::size_t IndexCfiCode();
    std::size_t thash();
    std::size_t add_symbol(char *ptr, std::string &);
    void Index(char *ptr, std::string &);
    std::size_t SystemInit();

    std::size_t TickSize();

    void Stock(char *ptr, int, double, double, std::size_t);

    void Tick(_millisecond);

    void xdxr(std::string &, std::size_t, std::string &, int);

    /* =============  OPERATORS     =================== */

protected:
    /* =============  METHODS       =================== */

    /* =============  DATA MEMBERS  =================== */

private:
    /* =============  METHODS       =================== */

    void init(char *ptr, std::uint32_t cficode, std::string symbol, char type);
    /* =============  DATA MEMBERS  =================== */

    std::string _header = "";
    int _tick_num = 1;
    _millisecond _unix_time;
    std::string _tick_data = "";
    AutoIncrement _symId;
    float _tick_sleep_time = 0.2;

    bool _debug = false;
}; /* -----  end of class DataFormat  ----- */

}  // namespace e2q
#endif /* ----- #ifndef DATAFORMAT_INC  ----- */
