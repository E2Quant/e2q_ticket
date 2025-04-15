/*
 * =====================================================================================
 *
 *       Filename:  itch50.hpp
 *
 *    Description:  itch50
 *
 *        Version:  1.0
 *        Created:  2024年03月16日 11时44分35秒
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

#ifndef ITCH50_INC
#define ITCH50_INC
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>

#include "kafka/producer.hpp"
#include "kafka/protocol/nbo.hpp"
#include "utility/DataFormat.hpp"

namespace e2q {
#define STOCK_LENGTH 8

// nanoseconds
#define NANO 1e-9

std::uint64_t format_time(std::string day, uint64_t Timestamp);

enum ITCH_T {
    // 1.1 System Event Message
    EVENT = 'S',

    // 1.2 Stock Related Messages
    DIRECTORY = 'R',
    TRADING = 'H',
    SHO = 'Y',
    PARTICIPANT = 'L',
    DECLINE = 'V',
    STATUS = 'W',
    IPO = 'K',
    LULD = 'J',
    HALT = 'h',

    // 1.3 Add Order Message
    NOMPID = 'A',
    MPID = 'F',

    // 1.4 Modify Order Messages
    ORDER = 'E',
    ORDERPRICE = 'C',
    ORDERCANCEL = 'X',
    ORDERDELETE = 'D',
    ORDERREPLACE = 'U',

    // 1.5 Trade Message
    TRADE = 'P',
    CROSSTRADE = 'Q',
    BROKENTRADE = 'B',

    // 1.6 Net Order Imbalance Indicator (NOII)Message
    NOII = 'I',

    // 1.7 Retail Price Improvement Indicator(RPII)
    RPII = 'N',

    // 1.8 Direct Listing with Capital Raise Price Discovery Message
    CAPITAL = 'O',

}; /* ----------  end of enum ITCH_T  ---------- */

typedef enum ITCH_T ITCH_T;

struct BaseMessage {
    char Type;
    uint16_t StockLocate;
    uint16_t TrackingNumber;
    uint64_t Timestamp;

    std::size_t size()
    {
        std::size_t _size = 0;
        _size += fldsiz(BaseMessage, Type);
        _size += fldsiz(BaseMessage, StockLocate);
        _size += fldsiz(BaseMessage, TrackingNumber);
        _size += fldsiz(BaseMessage, Timestamp) - 2;

        return _size - 1;
    };
}; /* ----------  end of struct BaseMessage  ---------- */

typedef struct BaseMessage BaseMessage;

//! System Event Message
struct SystemEventMessage : public BaseMessage {
    char EventCode;

    std::size_t size()
    {
        std::size_t _size = 1 + BaseMessage::size();

        return _size;
    }
};

//! Stock Directory Message
struct StockDirectoryMessage : public BaseMessage {
    char Stock[STOCK_LENGTH];
    char MarketCategory;
    char FinancialStatusIndicator;
    uint32_t RoundLotSize;
    char RoundLotsOnly;
    char IssueClassification;
    char IssueSubType[2];
    char Authenticity;
    char ShortSaleThresholdIndicator;
    char IPOFlag;
    char LULDReferencePriceTier;
    char ETPFlag;
    uint32_t ETPLeverageFactor;
    char InverseIndicator;

    // size = 39
    std::size_t size()
    {
        std::size_t _size = STOCK_LENGTH + BaseMessage::size();
        _size += fldsiz(StockDirectoryMessage, MarketCategory);
        _size += fldsiz(StockDirectoryMessage, FinancialStatusIndicator);
        _size += fldsiz(StockDirectoryMessage, RoundLotSize);
        _size += fldsiz(StockDirectoryMessage, RoundLotsOnly);
        _size += fldsiz(StockDirectoryMessage, IssueClassification);
        _size += fldsiz(StockDirectoryMessage, IssueSubType);
        _size += fldsiz(StockDirectoryMessage, Authenticity);
        _size += fldsiz(StockDirectoryMessage, ShortSaleThresholdIndicator);
        _size += fldsiz(StockDirectoryMessage, IPOFlag);
        _size += fldsiz(StockDirectoryMessage, LULDReferencePriceTier);
        _size += fldsiz(StockDirectoryMessage, ETPFlag);
        _size += fldsiz(StockDirectoryMessage, ETPLeverageFactor);
        _size += fldsiz(StockDirectoryMessage, InverseIndicator);

        return _size;
    }

    template <class TOutputStream>
    friend TOutputStream& operator<<(TOutputStream& stream,
                                     const StockDirectoryMessage& message);
};

//! Stock Trading Action Message
struct StockTradingActionMessage : public BaseMessage {
    char Stock[STOCK_LENGTH];
    char TradingState;
    char Reserved;
    char Reason[4];

    // size = 25
    std::size_t size()
    {
        std::size_t _size = STOCK_LENGTH + BaseMessage::size();
        _size += fldsiz(StockTradingActionMessage, TradingState);
        _size += fldsiz(StockTradingActionMessage, Reserved);
        _size += fldsiz(StockTradingActionMessage, Reason);

        return _size;
    }
    template <class TOutputStream>
    friend TOutputStream& operator<<(TOutputStream& stream,
                                     const StockTradingActionMessage& message);
};

//! Reg SHO Short Sale Price Test Restricted Indicator Message
struct RegSHOMessage : public BaseMessage {
    char Stock[STOCK_LENGTH];
    char RegSHOAction;

    // size = 20
    std::size_t size()
    {
        std::size_t _size = STOCK_LENGTH + BaseMessage::size();

        _size += fldsiz(RegSHOMessage, RegSHOAction);
        return _size;
    }
    template <class TOutputStream>
    friend TOutputStream& operator<<(TOutputStream& stream,
                                     const RegSHOMessage& message);
};

//! Market Participant Position Message
struct MarketParticipantPositionMessage : public BaseMessage {
    char MPID[4];
    char Stock[8];
    char PrimaryMarketMaker;
    char MarketMakerMode;
    char MarketParticipantState;

    // size = 26
    std::size_t size()
    {
        std::size_t _size = STOCK_LENGTH + BaseMessage::size();

        _size += fldsiz(MarketParticipantPositionMessage, MPID);
        _size += fldsiz(MarketParticipantPositionMessage, PrimaryMarketMaker);
        _size += fldsiz(MarketParticipantPositionMessage, MarketMakerMode);
        _size +=
            fldsiz(MarketParticipantPositionMessage, MarketParticipantState);
        return _size;
    }

    template <class TOutputStream>
    friend TOutputStream& operator<<(
        TOutputStream& stream, const MarketParticipantPositionMessage& message);
};

//! MWCB Decline Level Message
struct MWCBDeclineMessage : public BaseMessage {
    uint64_t Level1;
    uint64_t Level2;
    uint64_t Level3;

    // size = 35
    std::size_t size()
    {
        std::size_t _size = BaseMessage::size();
        _size += fldsiz(MWCBDeclineMessage, Level1);
        _size += fldsiz(MWCBDeclineMessage, Level2);
        _size += fldsiz(MWCBDeclineMessage, Level3);
        return _size;
    }

    template <class TOutputStream>
    friend TOutputStream& operator<<(TOutputStream& stream,
                                     const MWCBDeclineMessage& message);
};

//! MWCB Status Message
struct MWCBStatusMessage : public BaseMessage {
    char BreachedLevel;

    // size = 12
    std::size_t size()
    {
        std::size_t _size = BaseMessage::size();
        _size += fldsiz(MWCBStatusMessage, BreachedLevel);

        return _size;
    }
    template <class TOutputStream>
    friend TOutputStream& operator<<(TOutputStream& stream,
                                     const MWCBStatusMessage& message);
};

//! IPO Quoting Period Update Message
struct IPOQuotingMessage : public BaseMessage {
    char Stock[STOCK_LENGTH];
    uint32_t IPOReleaseTime;
    char IPOReleaseQualifier;
    uint32_t IPOPrice;

    // size = 28
    std::size_t size()
    {
        std::size_t _size = STOCK_LENGTH + BaseMessage::size();
        _size += fldsiz(IPOQuotingMessage, IPOReleaseTime);
        _size += fldsiz(IPOQuotingMessage, IPOReleaseQualifier);
        _size += fldsiz(IPOQuotingMessage, IPOPrice);
        return _size;
    }
    template <class TOutputStream>
    friend TOutputStream& operator<<(TOutputStream& stream,
                                     const IPOQuotingMessage& message);
};

//! Limit Up – Limit Down (LULD) Auction Collar Message
struct LULDAuctionCollarMessage : public BaseMessage {
    char Stock[STOCK_LENGTH];
    uint32_t AuctionCollarReferencePrice;
    uint32_t UpperAuctionCollarPrice;
    uint32_t LowerAuctionCollarPrice;
    uint32_t AuctionCollarExtension;

    // size = 35
    std::size_t size()
    {
        std::size_t _size = STOCK_LENGTH + BaseMessage::size();
        _size += fldsiz(LULDAuctionCollarMessage, AuctionCollarReferencePrice);
        _size += fldsiz(LULDAuctionCollarMessage, UpperAuctionCollarPrice);
        _size += fldsiz(LULDAuctionCollarMessage, LowerAuctionCollarPrice);
        _size += fldsiz(LULDAuctionCollarMessage, AuctionCollarExtension);
        return _size;
    }
    template <class TOutputStream>
    friend TOutputStream& operator<<(TOutputStream& stream,
                                     const LULDAuctionCollarMessage& message);
};

struct OperationalHalt : public BaseMessage {
    char Stock[STOCK_LENGTH];
    char MarketCode;
    char OperationalHaltAction;

    // size = 21
    std::size_t size()
    {
        std::size_t _size = STOCK_LENGTH + BaseMessage::size();
        _size += fldsiz(OperationalHalt, MarketCode);
        _size += fldsiz(OperationalHalt, OperationalHaltAction);

        return _size;
    }
    template <class TOutputStream>
    friend TOutputStream& operator<<(TOutputStream& stream,
                                     const OperationalHalt& message);
}; /* ----------  end of struct OperationalHalt  ---------- */

//! Add Order Message
struct AddOrderMessage : public BaseMessage {
    uint64_t OrderReferenceNumber;
    char BuySellIndicator;
    uint32_t Shares;
    char Stock[STOCK_LENGTH];
    uint32_t Price;

    // size = 36
    std::size_t size()
    {
        std::size_t _size = BaseMessage::size();
        _size += fldsiz(AddOrderMessage, OrderReferenceNumber);
        _size += fldsiz(AddOrderMessage, BuySellIndicator);
        _size += fldsiz(AddOrderMessage, Shares);
        _size += fldsiz(AddOrderMessage, Stock);
        _size += fldsiz(AddOrderMessage, Price);

        return _size;
    }

    template <class TOutputStream>
    friend TOutputStream& operator<<(TOutputStream& stream,
                                     const AddOrderMessage& message);
};

//! Add Order with MPID Attribution Message
struct AddOrderMPIDMessage : public BaseMessage {
    uint64_t OrderReferenceNumber;
    char BuySellIndicator;
    uint32_t Shares;
    char Stock[STOCK_LENGTH];
    uint32_t Price;
    char Attribution[4];

    // size = 40
    std::size_t size()
    {
        std::size_t _size = STOCK_LENGTH + BaseMessage::size();
        _size += fldsiz(AddOrderMPIDMessage, OrderReferenceNumber);
        _size += fldsiz(AddOrderMPIDMessage, BuySellIndicator);
        _size += fldsiz(AddOrderMPIDMessage, Shares);
        _size += fldsiz(AddOrderMPIDMessage, Price);
        _size += fldsiz(AddOrderMPIDMessage, Attribution);
        return _size;
    }
    template <class TOutputStream>
    friend TOutputStream& operator<<(TOutputStream& stream,
                                     const AddOrderMPIDMessage& message);
};

//! Order Executed Message
struct OrderExecutedMessage : public BaseMessage {
    uint64_t OrderReferenceNumber;
    uint32_t ExecutedShares;
    uint64_t MatchNumber;

    // size = 31
    std::size_t size()
    {
        std::size_t _size = BaseMessage::size();
        _size += fldsiz(OrderExecutedMessage, OrderReferenceNumber);
        _size += fldsiz(OrderExecutedMessage, ExecutedShares);
        _size += fldsiz(OrderExecutedMessage, MatchNumber);
        return _size;
    }
    template <class TOutputStream>
    friend TOutputStream& operator<<(TOutputStream& stream,
                                     const OrderExecutedMessage& message);
};

//! Order Executed With Price Message
struct OrderExecutedWithPriceMessage : public BaseMessage {
    uint64_t OrderReferenceNumber;
    uint32_t ExecutedShares;
    uint64_t MatchNumber;
    char Printable;
    uint32_t ExecutionPrice;

    // size = 36
    std::size_t size()
    {
        std::size_t _size = BaseMessage::size();
        _size += fldsiz(OrderExecutedWithPriceMessage, OrderReferenceNumber);
        _size += fldsiz(OrderExecutedWithPriceMessage, ExecutedShares);
        _size += fldsiz(OrderExecutedWithPriceMessage, MatchNumber);
        _size += fldsiz(OrderExecutedWithPriceMessage, Printable);
        _size += fldsiz(OrderExecutedWithPriceMessage, ExecutionPrice);
        return _size;
    }
    template <class TOutputStream>
    friend TOutputStream& operator<<(
        TOutputStream& stream, const OrderExecutedWithPriceMessage& message);
};

//! Order Cancel Message
struct OrderCancelMessage : public BaseMessage {
    uint64_t OrderReferenceNumber;
    uint32_t CanceledShares;

    // size = 23
    std::size_t size()
    {
        std::size_t _size = BaseMessage::size();
        _size += fldsiz(OrderCancelMessage, OrderReferenceNumber);
        _size += fldsiz(OrderCancelMessage, CanceledShares);
        return _size;
    }
    template <class TOutputStream>
    friend TOutputStream& operator<<(TOutputStream& stream,
                                     const OrderCancelMessage& message);
};

//! Order Delete Message
struct OrderDeleteMessage : public BaseMessage {
    uint64_t OrderReferenceNumber;
    // size = 19
    std::size_t size()
    {
        std::size_t _size = BaseMessage::size();
        _size += fldsiz(OrderDeleteMessage, OrderReferenceNumber);
        return _size;
    }
    template <class TOutputStream>
    friend TOutputStream& operator<<(TOutputStream& stream,
                                     const OrderDeleteMessage& message);
};

//! Order Replace Message
struct OrderReplaceMessage : public BaseMessage {
    uint64_t OriginalOrderReferenceNumber;
    uint64_t NewOrderReferenceNumber;
    uint32_t Shares;
    uint32_t Price;

    // size = 35
    std::size_t size()
    {
        std::size_t _size = BaseMessage::size();
        _size += fldsiz(OrderReplaceMessage, OriginalOrderReferenceNumber);
        _size += fldsiz(OrderReplaceMessage, NewOrderReferenceNumber);
        _size += fldsiz(OrderReplaceMessage, Shares);
        _size += fldsiz(OrderReplaceMessage, Price);
        return _size;
    }
    template <class TOutputStream>
    friend TOutputStream& operator<<(TOutputStream& stream,
                                     const OrderReplaceMessage& message);
};

//! Trade Message
struct TradeMessage : public BaseMessage {
    uint64_t OrderReferenceNumber;
    char BuySellIndicator;
    uint32_t Shares;
    char Stock[STOCK_LENGTH];
    uint32_t Price;
    uint64_t MatchNumber;

    // size = 44
    std::size_t size()
    {
        std::size_t _size = BaseMessage::size();
        _size += fldsiz(TradeMessage, OrderReferenceNumber);
        _size += fldsiz(TradeMessage, BuySellIndicator);
        _size += fldsiz(TradeMessage, Shares);
        _size += fldsiz(TradeMessage, Stock);
        _size += fldsiz(TradeMessage, Price);
        _size += fldsiz(TradeMessage, MatchNumber);

        return _size;
    }
    template <class TOutputStream>
    friend TOutputStream& operator<<(TOutputStream& stream,
                                     const TradeMessage& message);
};

//! Cross Trade Message
struct CrossTradeMessage : public BaseMessage {
    uint64_t Shares;
    char Stock[STOCK_LENGTH];
    uint32_t CrossPrice;
    uint64_t MatchNumber;
    char CrossType;

    // size = 40
    std::size_t size()
    {
        std::size_t _size = BaseMessage::size();
        _size += fldsiz(CrossTradeMessage, Shares);
        _size += fldsiz(CrossTradeMessage, Stock);
        _size += fldsiz(CrossTradeMessage, CrossPrice);
        _size += fldsiz(CrossTradeMessage, MatchNumber);
        _size += fldsiz(CrossTradeMessage, CrossType);

        return _size;
    }
    template <class TOutputStream>
    friend TOutputStream& operator<<(TOutputStream& stream,
                                     const CrossTradeMessage& message);
};

//! Broken Trade Message
struct BrokenTradeMessage : public BaseMessage {
    uint64_t MatchNumber;

    // size = 19
    std::size_t size()
    {
        std::size_t _size = BaseMessage::size();
        _size += fldsiz(BrokenTradeMessage, MatchNumber);
        return _size;
    }
    template <class TOutputStream>
    friend TOutputStream& operator<<(TOutputStream& stream,
                                     const BrokenTradeMessage& message);
};

//! Net Order Imbalance Indicator (NOII) Message
struct NOIIMessage : public BaseMessage {
    uint64_t PairedShares;
    uint64_t ImbalanceShares;
    char ImbalanceDirection;
    char Stock[STOCK_LENGTH];
    uint32_t FarPrice;
    uint32_t NearPrice;
    uint32_t CurrentReferencePrice;
    char CrossType;
    char PriceVariationIndicator;

    // size = 50
    std::size_t size()
    {
        std::size_t _size = BaseMessage::size();
        _size += fldsiz(NOIIMessage, PairedShares);
        _size += fldsiz(NOIIMessage, ImbalanceShares);
        _size += fldsiz(NOIIMessage, ImbalanceDirection);
        _size += fldsiz(NOIIMessage, Stock);
        _size += fldsiz(NOIIMessage, FarPrice);
        _size += fldsiz(NOIIMessage, NearPrice);
        _size += fldsiz(NOIIMessage, CurrentReferencePrice);
        _size += fldsiz(NOIIMessage, CrossType);
        _size += fldsiz(NOIIMessage, PriceVariationIndicator);
        return _size;
    }
    template <class TOutputStream>
    friend TOutputStream& operator<<(TOutputStream& stream,
                                     const NOIIMessage& message);
};

//! Retail Price Improvement Indicator (RPII) Message
struct RPIIMessage : public BaseMessage {
    char Stock[STOCK_LENGTH];
    char InterestFlag;
    // size = 20
    std::size_t size()
    {
        std::size_t _size = BaseMessage::size();
        _size += fldsiz(RPIIMessage, Stock);
        _size += fldsiz(RPIIMessage, InterestFlag);
        return _size;
    }
    template <class TOutputStream>
    friend TOutputStream& operator<<(TOutputStream& stream,
                                     const RPIIMessage& message);
};

//! Direct Listing with Capital Raise Price Discovery Message
struct CapitalRaisePriceDiscoveryMessage : public BaseMessage {
    char Stock[STOCK_LENGTH];
    char Eligibility;
    uint32_t MinimumPrice;
    uint32_t MaximumPrice;
    uint32_t NearPrice;
    uint64_t NearTime;
    uint32_t LowerPrice;
    uint32_t UpperPrice;

    // size = 48
    std::size_t size()
    {
        std::size_t _size = BaseMessage::size();
        _size += fldsiz(CapitalRaisePriceDiscoveryMessage, Stock);
        _size += fldsiz(CapitalRaisePriceDiscoveryMessage, Eligibility);
        _size += fldsiz(CapitalRaisePriceDiscoveryMessage, MinimumPrice);
        _size += fldsiz(CapitalRaisePriceDiscoveryMessage, MaximumPrice);
        _size += fldsiz(CapitalRaisePriceDiscoveryMessage, NearPrice);
        _size += fldsiz(CapitalRaisePriceDiscoveryMessage, NearTime);
        _size += fldsiz(CapitalRaisePriceDiscoveryMessage, LowerPrice);
        _size += fldsiz(CapitalRaisePriceDiscoveryMessage, UpperPrice);
        return _size;
    }
    template <class TOutputStream>
    friend TOutputStream& operator<<(
        TOutputStream& stream,
        const CapitalRaisePriceDiscoveryMessage& message);
}; /* ----------  end of struct CapitalRaisePriceDiscoveryMessage ----------
    */

//! Unknown message
struct UnknownMessage {
    char Type;

    template <class TOutputStream>
    friend TOutputStream& operator<<(TOutputStream& stream,
                                     const UnknownMessage& message);
};

#define TSleep(t)                                       \
    ({                                                  \
        do {                                            \
            int sleep_time = (1 + (rand() % 20)) * 10;  \
            if (t > 0) {                                \
                sleep_time = t;                         \
            }                                           \
            std::this_thread::sleep_for(                \
                std::chrono::milliseconds(sleep_time)); \
        } while (0);                                    \
    })

/*
 * ================================
 *        Class:  ItchMsg
 *  Description:
 *  NASDAQ according to the ITCH 50 data protocol specified at
 * https://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTVITCHSpecification.pdf.
 * ================================
 */
class ItchMsg {
public:
    /* =============  LIFECYCLE     =================== */
    ItchMsg(std::string& path, std::uint8_t inter, int limit)
        : _path(path), interval(inter), _limit(limit) {

          }; /* constructor */

    /* =============  ACCESSORS     =================== */

    /* =============  MUTATORS      =================== */
    int init_kafka(std::string, std::string);
    void read(std::string);

    std::size_t message(const char* key, std::FILE*);

    /* =============  OPERATORS     =================== */

protected:
    /* =============  METHODS       =================== */

    /* =============  DATA MEMBERS  =================== */

private:
    /* =============  METHODS       =================== */

    /* =============  DATA MEMBERS  =================== */

    std::string _path;
    std::string _symobl;

    std::string path_day = "";
    std::size_t day = 0;

    Producer* _producer{nullptr};
    std::size_t _message_type = 1;
    std::map<std::string, std::size_t> _symbols;

    int _frame = 1;
    std::string _topic = "";
    DataFormat _DataFmt;
    char* src_ptr = nullptr;
    std::size_t mlen = 0;
    std::string exit_data = "E";
    std::uint8_t interval = 100;

    int _limit = 100;
}; /* -----  end of class ItchMsg  ----- */

}  // namespace e2q
#endif /* ----- #ifndef ITCH50_INC  ----- */
