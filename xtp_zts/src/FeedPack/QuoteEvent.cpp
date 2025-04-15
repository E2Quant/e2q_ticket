/*
 * =====================================================================================
 *
 *       Filename:  QuoteEvent.hpp
 *
 *    Description:  QuoteEvent
 *
 *        Version:  1.0
 *        Created:  2025/01/24 09时57分23秒
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
#include "FeedPack/QuoteEvent.hpp"

#include <unistd.h>

#include <csignal>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>

#include "Log.hpp"
#include "Toolkit/UtilTime.hpp"
#include "kafka/producer.hpp"
#include "xtp_api_data_type.h"

namespace e2q {

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  exit_handler
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void exit_handler(int s)
{
    e2q::keepRunning = 0;
} /* -----  end of function exit_handler  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  QuoteEvent::init
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void QuoteEvent::init()
{
    client_id = _futils->intForKey("client_id");
    account_key = _futils->stdStringForKey("account_key");
#ifdef _WIN32
    filepath = _futils->stdStringForKey("path");
#else
    filepath = _futils->stdStringForKey("path_linux");
#endif  // _WIN32
    // 读取行情配置
    quote_server_ip = _futils->stdStringForKey("quote_ip");
    quote_server_port = _futils->intForKey("quote_port");
    quote_username = _futils->stdStringForKey("quote_user");
    quote_password = _futils->stdStringForKey("quote_password");
    quote_protocol = (XTP_PROTOCOL_TYPE)(_futils->intForKey("quote_protocol"));
    quote_buffer_size = _futils->intForKey("quote_buffer_size");

    // 读取心跳超时配置
    heat_beat_interval = _futils->intForKey("hb_interval");

    kafkaserver = _futils->stdStringForKey("kafka.host");
    kafkaport = _futils->intForKey("kafka.port");
    log::info("server:", kafkaserver, " port:", kafkaport);

} /* -----  end of function QuoteEvent::init  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  QuoteEvent::run
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void QuoteEvent::run()
{
    log::info("run");

    std::string brokers = kafkaserver + ":" + std::to_string(kafkaport);

    this->_producer = new Producer();

    log::info(brokers);
    auto pub_fun = [this](std::string brokers, std::string topic) {
        this->_producer->init(brokers, topic);
    };  // -----  end lambda  -----

    std::thread pub_thread(pub_fun, brokers, topic);
    pub_thread.detach();

    sleep(2);
    head();
    quote();
} /* -----  end of function QuoteEvent::run  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  QuoteEvent::head
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void QuoteEvent::head()
{
    std::string instrument;
    int instrument_count = _futils->countForKey("quote_ticker.instrument");
    std::size_t id = 0;

    _quote_spi = new ZtsQuoteSpi(_producer);
    if (!_quote_spi) {
        log::bug(
            "Failed to create quote spi, please check the input parameters.");
        return;
    }
    std::string index = "index";
    std::size_t mlen = _DataFmt.SystemInit();
    log::echo("mlen:", mlen);
    char* src_ptr = (char*)calloc(mlen, sizeof(char*));
    // 从配置文件中读取需要订阅的股票
    for (int i = 0; i < instrument_count; i++) {
        instrument = _futils->stdStringForKey("quote_ticker.instrument[%d]", i);
        id = _DataFmt.add_symbol(src_ptr, instrument);
        _producer->data(src_ptr, mlen, topic, nullptr);

        _quote_spi->addSymbol(id, instrument);
    }

    _DataFmt.Index(src_ptr, index);
    _producer->data(src_ptr, mlen, topic, nullptr);

    _quote_spi->addSymbol(0, index);

    free(src_ptr);
    src_ptr = nullptr;

} /* -----  end of function QuoteEvent::head  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  QuoteEvent::quote
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void QuoteEvent::quote()
{
    int instrument_count = _futils->countForKey("quote_ticker.instrument");
    int quote_exchange = _futils->intForKey("quote_ticker.exchange");

    // 从配置文件中读取需要订阅的股票
    char** allInstruments = new char*[instrument_count];
    for (int i = 0; i < instrument_count; i++) {
        allInstruments[i] = new char[XTP_TICKER_LEN];
        std::string instrument =
            _futils->stdStringForKey("quote_ticker.instrument[%d]", i);
        strncpy(allInstruments[i], instrument.c_str(), XTP_TICKER_LEN);
    }

    // 初始化行情api
    _pQuoteApi = XTP::API::QuoteApi::CreateQuoteApi(
        client_id, filepath.c_str(),
        XTP_LOG_LEVEL_DEBUG);  // log日志级别可以调整
    if (!_pQuoteApi) {
        log::bug(
            "Failed to create quote api, please check the input parameters.");
        return;
    }

    auto fun = [this, &allInstruments, &instrument_count, &quote_exchange]() {
        // 重新登陆行情服务器
        int ret = -1;
        // 重连次数
        int i_counter = 0;
        while (0 != ret) {
            ret = this->_pQuoteApi->Login(
                quote_server_ip.c_str(), quote_server_port,
                quote_username.c_str(), quote_password.c_str(),
                this->quote_protocol);
            if (0 != ret) {
                i_counter++;
                if (i_counter >= 3) {
                    log::bug("relogin failed over 3 times. ");
                    return;
                }
                // 登录失败，获取错误信息
                XTPRI* error_info = this->_pQuoteApi->GetApiLastError();
                log::bug("login to server error, ", error_info->error_id, " :",
                         error_info->error_msg);

                // 等待10s以后再次连接，可修改此等待时间，建议不要小于3s
#ifdef _WIN32
                Sleep(10000);
#else
                sleep(10);
#endif  // _WIN32
            }
            else {
                i_counter = 0;
            }
        }

        // 重连成功
        log::info("login to server success. ");
        // 再次订阅行情快照
        // subscribeMarketData();
        this->_pQuoteApi->SubscribeMarketData(
            allInstruments, instrument_count,
            (XTP_EXCHANGE_TYPE)quote_exchange);
    };  // -----  end lambda  -----

    _quote_spi->callBack(fun);

    _pQuoteApi->RegisterSpi(_quote_spi);

    // 设定行情服务器超时时间，单位为秒
    _pQuoteApi->SetHeartBeatInterval(heat_beat_interval);  // 此为1.1.16新增接口
    // 设定行情本地缓存大小，单位为MB
    _pQuoteApi->SetUDPBufferSize(quote_buffer_size);  // 此为1.1.16新增接口

    int loginResult_quote = -1;
    // 登录行情服务器,自1.1.16开始，行情服务器支持UDP连接，推荐使用UDP
    loginResult_quote = _pQuoteApi->Login(
        quote_server_ip.c_str(), quote_server_port, quote_username.c_str(),
        quote_password.c_str(), quote_protocol);

    if (loginResult_quote == 0) {
        log::info("login ok");
        // 登录行情服务器成功后，订阅行情

        // 开始订阅,注意公网测试环境仅支持TCP方式，如果使用UDP方式会没有行情数据，实盘大多数使用UDP连接
        _pQuoteApi->SubscribeMarketData(allInstruments, instrument_count,
                                        (XTP_EXCHANGE_TYPE)quote_exchange);

        _pQuoteApi->QueryAllTickersFullInfo(XTP_EXCHANGE_SZ);

        UtilTime ut;
        int now = ut.time();
        const char fmt[] = "%H%M%S";
        std::string now_day = ut.stamptostr(now, fmt);

        int int_day = std::stoi(now_day.c_str());

        signal(SIGINT, exit_handler);

        // 保持主线程，防止程序退出
        while (int_day > 92000 && int_day < 181000 && keepRunning == 1) {
            now = ut.time();
            now_day = ut.stamptostr(now, fmt);

            int_day = std::stoi(now_day.c_str());

#ifdef _WIN32
            Sleep(10000);
#else
            sleep(10);
#endif  // _WIN32
        }

        _producer->data(exit_data, topic);
        _producer->exist();

        _pQuoteApi->Logout();

        log::info("exit time:", now_day);

        // 释放
        for (int i = 0; i < instrument_count; i++) {
            delete[] allInstruments[i];
            allInstruments[i] = NULL;
        }

        delete[] allInstruments;
        allInstruments = NULL;
    }
    else {
        // 登录失败，获取失败原因
        XTPRI* error_info = _pQuoteApi->GetApiLastError();
        log::bug("Login to server error, ", error_info->error_id, " : ",
                 error_info->error_msg);
    }
} /* -----  end of function QuoteEvent::quote  ----- */
}  // namespace e2q
