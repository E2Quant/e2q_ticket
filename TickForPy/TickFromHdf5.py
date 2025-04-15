#!/usr/bin/env python
# -*- coding: utf-8 -*-
# @Time    : 2024/6/22 下午2:52
# @Author  : vyouzhi
# @File    : tdx_data.py


import sys
import os
from time import sleep
import numpy as np
from tzlocal import get_localzone_name
from datetime import datetime
import json
import random
from loguru import logger
from common import *


class TickFromHdf:
    """ """

    def __init__(self, symbols, path, limit, xdxr=False):
        # price hdf path
        self._path = path
        # read limit price
        self._limit = limit
        # symbols

        self._symbols = symbols

        self._gmt = 1

        # 间隔报价的时间
        self._tick_sleep_time = 0.1

        # 分红除息
        self._xdxr = xdxr

        self._start_index = 0
        self._end_index = None

        # 如果 true 就只发送 close 的价格，这样是让多次的回测价格一样
        self._isClose = False

    def readPrice(self, symbol):
        """ """
        fname = self._path + "/" + symbol + ".hdf5"
        logger.info(fname)
        store = pd.HDFStore(fname, "r")
        reread = store.select("data", start=~self._limit)
        store.close()

        return reread

    def InitIndex(self):
        # "sh000001" index

        self._unixtime = 10 ** 9

        self._index_code = "sh000001"

        self._index = self.readPrice(self._index_code)
        self.TimeFrame = 1440

        self._start_index = self._index.head(1).index.values[0]
        self._end_index = self._index.tail(1).index.values[0]
        # print(self._end_index)

        if self._gmt > 0:
            self._index = self._index.tz_localize(get_localzone_name())

        logger.info(
            "%s - %s" % (self._start_index, self._index.tail(1).index.values[0])
        )

    def CloseData(self, values):
        """
        只使用 close 的价格
        :return:
        """
        closesValue = values
        closesValue[0] = values[3]
        closesValue[1] = values[3]
        closesValue[2] = values[3]
        closesValue[3] = values[3]
        return closesValue

    def unixtime(self, v):
        # offtime = self.TimeFrame * 60
        """
        在这儿转一下 on-open
        :param v:
        :return:
        """
        ret = int(v / self._unixtime)

        return ret

    def toKafka(self):
        symIds = {}
        sym_dict = {}

        self.InitIndex()
        dformat = DataFormatProto(symId=1, tick_time=self._tick_sleep_time)
        dxdxr = StockAXdxrMessage(xdxr=self._xdxr)
        mtickm = MarketTickMessage()

        kf = kafka_producer("fix-events", "key", self._tick_sleep_time)
        idx_day = int(str(self._start_index)[:10].replace("-", ""))

        for code in self._symbols:
            (symId, symbol_info) = dformat.add_symbol(code)
            symIds[code] = symId

            reread = self.readPrice(code)
            if reread is None:
                logger.info("bad sym:", code)
                continue

            stock = reread[["open", "low", "high", "close", "volume"]]
            # logger.info(stock.tail(5))
            sym_dict[code] = stock

            kf.publish(symbol_info)

            dxdxr.xdxr(kf.publish, code, symId, idx_day, 0)

        header = dformat.Index(b"index")

        kf.publish(header)

        tick_num = 4
        idx_number = 0

        for index, index_row in self._index.iterrows():

            index_bar = index_row[["open", "low", "high", "close", "volume"]].values

            NowBarData = {}
            unix_time = self.unixtime(index.value)
            idx_day = datetime.fromtimestamp(unix_time).strftime("%Y-%m-%d %H:%M:%S")
            xdxr_day = int(idx_day[:10].replace("-", ""))

            # 取各个 sym 的数据
            for sym in self._symbols:
                # 当前开盘的时候
                dxdxr.xdxr(kf.publish, sym, symIds[sym], xdxr_day, 1)

                sym_tdx = sym_dict.get(sym)
                sym_day_data = sym_tdx[sym_tdx.index == index]
                # 停牌的数据
                nullDatay = [0, 0, 0, 0, 0, 0]
                if len(sym_day_data.values) == 0:
                    NowBarData[sym] = nullDatay
                else:
                    priceData = sym_day_data.values[0]
                    if self._isClose:
                        priceData = self.CloseData(sym_day_data.values[0])

                    NowBarData[sym] = priceData

            # 按ticket 发送价格
            pty_idx = tick_num
            for tick_idx in range(0, tick_num):
                mtickm.UinxTime(unix_time)

                for code in NowBarData:
                    # logger.info(symIds[code])
                    mtickm.Stock(
                        self.TimeFrame,
                        NowBarData[code][pty_idx],
                        NowBarData[code][tick_idx],
                        idx_number,
                        symIds[code],
                    )
                    stock_data = mtickm.toString()
                    kf.publish(stock_data)

                mtickm.Index(
                    self.TimeFrame,
                    index_bar[pty_idx],
                    index_bar[tick_idx],
                    idx_number,
                    dformat.IndexCfiCode(),
                )
                idx_data = mtickm.toString()

                kf.publish(idx_data)
                idx_number += 1

                sleep(self._tick_sleep_time)
        data = dformat.pExit()
        kf.publish(data)


def run():
    path = "./"
    limit = 10
    symbols = ["sh601857"]
    tick = TickFromHdf(symbols, path, limit, xdxr=True)
    tick.toKafka()


if __name__ == "__main__":
    run()
