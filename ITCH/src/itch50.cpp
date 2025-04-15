/*
 * =====================================================================================
 *
 *       Filename:  itch50.cpp
 *
 *    Description:  itch50
 *
 *        Version:  1.0
 *        Created:  2024年03月16日 11时51分05秒
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
#include "itch50.hpp"

#include <unistd.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

#include "Log.hpp"
#include "Toolkit/UtilTime.hpp"

namespace e2q {
/*
 * ===  FUNCTION  =============================
 *
 *         Name:  format_time
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
std::uint64_t format_time(std::string day, uint64_t Timestamp)
{
    double seconds = (double)Timestamp * NANO;
    double m = seconds / 60.0;
    double s = (double)((int)seconds % 60);
    int h = (int)(m / 60.0);
    m = ((int)m % 60);

    std::string date =
        log::format("%s %02d:%02d:%02d", day.c_str(), h, (int)m, (int)s);
    UtilTime ut;
    uint64_t now = ut.strtostamp(date, "%Y-%m-%d %H:%M:%S");

    //  printf("%s sed:%ld\n", date.c_str(), now);

    return now;

} /* -----  end of function format_time  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ItchMsg::init_kafka
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
int ItchMsg::init_kafka(std::string brokers, std::string topic)
{
    if (brokers.length() == 0 || topic.length() == 0) {
        log::info("empty!");
        return -1;
    }
    this->_producer = new Producer();
    int r = this->_producer->init(brokers, topic);
    if (r == 0) {
        auto pub_fun = [this](std::string brokers, std::string topic) {
            this->_producer->daemon();
        };  // -----  end lambda  -----
        std::thread pub_thread(pub_fun, brokers, topic);
        pub_thread.detach();
    }
    else {
        return -1;
    }

    _topic = topic;

    return 0;
} /* -----  end of function ItchMsg::init_kafka  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ItchMsg::read
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */

void ItchMsg::read(std::string symbol)
{
    std::size_t id = 0;

    std::FILE* _fp = fopen(_path.c_str(), "r");
    if (_fp == NULL) {
        std::cerr << "error path:" << std::endl;

        return;
    }
    UtilTime ut;
    std::string filename = _path.substr(_path.find_last_of("/\\") + 1);
    path_day = filename.substr(4, 4);
    path_day += "-";
    path_day += filename.substr(0, 2);
    path_day += "-";
    path_day += filename.substr(2, 2);

    //    const char fmt[] = "%m%d%Y";
    const char fmt[] = "%Y-%m-%d";

    day = ut.strtostamp(path_day, fmt);

    if (day == 0) {
        e2q::log::bug("filename:", filename, " day:", path_day);
        return;
    }

    if (_producer != nullptr) {
        std::string index = "index";
        std::size_t mlen = _DataFmt.SystemInit();

        char* src_ptr = (char*)calloc(mlen, sizeof(char*));
        if (src_ptr != nullptr) {
            id = _DataFmt.add_symbol(src_ptr, symbol);

            _producer->data(src_ptr, mlen, _topic, nullptr);

            _symbols.insert({symbol, id});

            _DataFmt.Index(src_ptr, index);
            _producer->data(src_ptr, mlen, _topic, nullptr);

            _symbols.insert({index, 0});

            free(src_ptr);
            src_ptr = nullptr;
        }
    }

    _symobl = symbol;

    std::size_t rsize = 0;

    std::size_t key_size = sizeof(char);
    int m = 0;
    char key[1] = {0};
    while ((rsize = fread(key, key_size, 1, _fp)) > 0) {
        rsize = message(key, _fp);
        if (rsize <= 0 || m > _limit) {
            break;
        }

        if (*key == 'P') {
            // log::echo(" m:", m);
            //          break;
            m++;
        }
    }

    log::info("end m:", m);
    fclose(_fp);

    if (_producer != nullptr) {
        _producer->data(exit_data, _topic);
        _producer->exist();
        delete _producer;
    }

} /* -----  end of function ItchMsg::read  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  ItchMsg::message
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
std::size_t ItchMsg::message(const char* key, std::FILE* fp)
{
    char _buffer[50] = {0};
    std::size_t msg_size = 0;
    std::size_t read_size = 0;
    std::size_t offset = 0;
    switch (*key) {
        case ITCH_T::EVENT: {
            // SystemEventMessage
            SystemEventMessage sem;
            msg_size = sem.size();

            read_size = fread(_buffer, sizeof(char), msg_size, fp);
            if (read_size < 1) {
                return -1;
            }
            offset += parse_uint_t(_buffer, sem.StockLocate);
            offset += parse_uint_t(_buffer + offset, sem.TrackingNumber);
            offset +=
                parse_uint_t<std::uint64_t, 2>(_buffer + offset, sem.Timestamp);
            sem.EventCode = *(_buffer + offset);

            // std::printf("ec: %c \n", *(_buffer + offset));

            // log::echo("offset:", offset, " msg:", msg_size,
            //           " stock:", sem.StockLocate, " ec:", sem.EventCode,
            //           " time:", sem.Timestamp);
            break;
        }

        case ITCH_T::DIRECTORY: {
            StockDirectoryMessage sdm;
            msg_size = sdm.size();
            read_size = fread(_buffer, sizeof(char), msg_size, fp);
            if (read_size < 1) {
                return -1;
            }
            offset += parse_uint_t(_buffer, sdm.StockLocate);
            offset += parse_uint_t(_buffer + offset, sdm.TrackingNumber);
            offset +=
                parse_uint_t<std::uint64_t, 2>(_buffer + offset, sdm.Timestamp);

            memcpy(&sdm.Stock, _buffer + offset, STOCK_LENGTH - 1);
            offset += STOCK_LENGTH;

            sdm.MarketCategory += *(_buffer + offset);

            // double seconds = sdm.Timestamp * NANO;
            //  log::echo("sl:", sdm.StockLocate, " tracking:",
            //  sdm.TrackingNumber,
            //            " time:", sdm.Timestamp, " stock:", sdm.Stock,
            //            " MC:", sdm.MarketCategory, " seconds:", seconds);
            //  std::printf("seconds : %.5f", seconds);

            break;
        }
        case ITCH_T::TRADING: {
            StockTradingActionMessage stam;
            msg_size = stam.size();
            fseek(fp, msg_size, SEEK_CUR);
            break;
        }
        case ITCH_T::SHO: {
            RegSHOMessage shom;
            msg_size = shom.size();
            fseek(fp, msg_size, SEEK_CUR);

            break;
        }
        case ITCH_T::PARTICIPANT: {
            MarketParticipantPositionMessage mppm;
            msg_size = mppm.size();
            read_size = fread(_buffer, sizeof(char), msg_size, fp);
            if (read_size < 1) {
                return -1;
            }
            offset += parse_uint_t(_buffer, mppm.StockLocate);
            offset += parse_uint_t(_buffer + offset, mppm.TrackingNumber);
            offset += parse_uint_t<std::uint64_t, 2>(_buffer + offset,
                                                     mppm.Timestamp);
            memset(&mppm.MPID, '\0', 4);
            memcpy(&mppm.MPID, _buffer + offset, 4);
            // std::printf("mpid:%s\n", mppm.MPID);

            offset += 4;

            memset(&mppm.Stock, '\0', STOCK_LENGTH);
            memcpy(&mppm.Stock, _buffer + offset, STOCK_LENGTH);
            offset += STOCK_LENGTH;
            // std::printf("stock: %s \n", mppm.Stock);

            mppm.PrimaryMarketMaker = *(_buffer + offset);
            offset++;

            mppm.MarketMakerMode = *(_buffer + offset);
            offset++;
            mppm.MarketParticipantState = *(_buffer + offset);
            // printf("pr:%c mode: %c, stat:%c\n", mppm.PrimaryMarketMaker,
            //        mppm.MarketMakerMode, mppm.MarketParticipantState);

            // log::echo("sl:", mppm.StockLocate, " mpid:", mppm.MPID,
            //           " time:", mppm.Timestamp, " stock:", mppm.Stock,
            //           " offset:", offset);

            break;
        }
        case ITCH_T::DECLINE: {
            MWCBDeclineMessage mwcb;
            msg_size = mwcb.size();
            fseek(fp, msg_size, SEEK_CUR);

            break;
        }
        case ITCH_T::STATUS: {
            MWCBStatusMessage mwcbm;
            msg_size = mwcbm.size();
            fseek(fp, msg_size, SEEK_CUR);

            break;
        }
        case ITCH_T::IPO: {
            IPOQuotingMessage ipo;
            msg_size = ipo.size();
            fseek(fp, msg_size, SEEK_CUR);

            break;
        }
        case ITCH_T::LULD: {
            LULDAuctionCollarMessage luld;
            msg_size = luld.size();
            fseek(fp, msg_size, SEEK_CUR);

            break;
        }
        case ITCH_T::HALT: {
            OperationalHalt halt;
            msg_size = halt.size();
            fseek(fp, msg_size, SEEK_CUR);

            break;
        }
        case ITCH_T::NOMPID: {
            AddOrderMessage om;
            msg_size = om.size();
            fseek(fp, msg_size, SEEK_CUR);

            break;
        }
        case ITCH_T::MPID: {
            AddOrderMPIDMessage mpid;
            msg_size = mpid.size();

            read_size = fread(_buffer, sizeof(char), msg_size, fp);

            offset += parse_uint_t(_buffer, mpid.StockLocate);
            offset += parse_uint_t(_buffer + offset, mpid.TrackingNumber);
            offset += parse_uint_t(_buffer + offset, mpid.Timestamp);
            offset += parse_uint_t(_buffer + offset, mpid.OrderReferenceNumber);
            offset++;
            offset += parse_uint_t(_buffer + offset, mpid.Shares);

            // log::echo("key:", *key, " share:", mpid.Shares,
            //           " time:", mpid.Timestamp);
            memcpy(&mpid.Stock, _buffer + offset, STOCK_LENGTH);
            // printf("stock %s,  resize:%ld", mpid.Stock, read_size);

            break;
        }
        case ITCH_T::ORDER: {
            OrderExecutedMessage oem;
            msg_size = oem.size();
            fseek(fp, msg_size, SEEK_CUR);

            break;
        }
        case ITCH_T::ORDERPRICE: {
            OrderExecutedWithPriceMessage ewp;
            msg_size = ewp.size();
            fseek(fp, msg_size, SEEK_CUR);

            break;
        }
        case ITCH_T::ORDERCANCEL: {
            OrderCancelMessage canm;
            msg_size = canm.size();
            fseek(fp, msg_size, SEEK_CUR);

            break;
        }
        case ITCH_T::ORDERDELETE: {
            OrderDeleteMessage delm;
            msg_size = delm.size();
            fseek(fp, msg_size, SEEK_CUR);

            break;
        }
        case ITCH_T::ORDERREPLACE: {
            OrderReplaceMessage rep;
            msg_size = rep.size();
            fseek(fp, msg_size, SEEK_CUR);

            break;
        }

        case ITCH_T::TRADE: {
            TradeMessage trade;
            msg_size = trade.size();

            read_size = fread(_buffer, sizeof(char), msg_size, fp);
            if (read_size < 1) {
                return -1;
            }
            offset += parse_uint_t(_buffer, trade.StockLocate);
            offset += parse_uint_t(_buffer + offset, trade.TrackingNumber);
            offset += parse_uint_t<std::uint64_t, 2>(_buffer + offset,
                                                     trade.Timestamp);

            offset +=
                parse_uint_t(_buffer + offset, trade.OrderReferenceNumber);
            trade.BuySellIndicator = *(_buffer + offset);
            offset++;

            offset += parse_uint_t(_buffer + offset, trade.Shares);

            memset(&trade.Stock, '\0', STOCK_LENGTH);

            memcpy(&trade.Stock, _buffer + offset, STOCK_LENGTH - 1);

            int cmp = memcmp(trade.Stock, _symobl.c_str(), _symobl.length());

            offset += STOCK_LENGTH;
            offset += parse_uint_t(_buffer + offset, trade.Price);
            offset += parse_uint_t(_buffer + offset, trade.MatchNumber);

            if (cmp == 0) {
                std::uint64_t seconds = format_time(path_day, trade.Timestamp);

                //                _DataFmt.Debug();
                _DataFmt.Tick(seconds);

                std::size_t cfi = 0;
                if (_symbols.count(_symobl) == 1) {
                    cfi = _symbols.at(_symobl);
                }
                if (src_ptr == nullptr) {
                    mlen = _DataFmt.TickSize();
                    src_ptr = (char*)calloc(mlen, sizeof(char*));
                }
                _DataFmt.Stock(src_ptr, _frame, trade.Shares, trade.Price, cfi);
                _producer->data(src_ptr, mlen, _topic, nullptr);

                _DataFmt.Stock(src_ptr, _frame, trade.Shares, trade.Price,
                               _DataFmt.IndexCfiCode());
                _producer->data(src_ptr, mlen, _topic, nullptr);

                TSleep(interval);
            }

            break;
        }
        case ITCH_T::CROSSTRADE: {
            CrossTradeMessage cross;
            msg_size = cross.size();
            fseek(fp, msg_size, SEEK_CUR);

            break;
        }
        case ITCH_T::BROKENTRADE: {
            BrokenTradeMessage broken;
            msg_size = broken.size();
            fseek(fp, msg_size, SEEK_CUR);

            break;
        }
        case ITCH_T::NOII: {
            NOIIMessage noii;
            msg_size = noii.size();
            fseek(fp, msg_size, SEEK_CUR);

            break;
        }
        case ITCH_T::RPII: {
            RPIIMessage rpii;
            msg_size = rpii.size();
            fseek(fp, msg_size, SEEK_CUR);

            break;
        }
        case ITCH_T::CAPITAL: {
            CapitalRaisePriceDiscoveryMessage crpdm;
            msg_size = crpdm.size();
            fseek(fp, msg_size, SEEK_CUR);

            break;
        }

        default: {
            // log::bug("----");
            msg_size = 1;
            break;
        }
    }
    // if (msg_size > 1 ) {
    //     log::info("key.", *key, " size:", msg_size);
    // }
    return msg_size;
} /* -----  end of function ItchMsg::message  ----- */

}  // namespace e2q
