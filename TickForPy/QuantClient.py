import time
from datetime import datetime

from tzlocal import get_localzone_name

from StockBar import StockBar
from common import *


class QuantClient(object):
    """

    """

    def __init__(self):
        """
        doc
        """
        self._code = None
        logger.add("/tmp/quote_py.log")

        # symid
        self._symid = 0

        # 间隔报价的时间 , 接下来 一分钟更新一次可以了
        self._tick_sleep_time = 0.2

        self._symIds = {}
        self._sym_dict = {}
        self._unixtime = 10 ** 9

        self._kf = None
        self._dformat = None
        self._dxdxr = None
        self._mtickm = None

        # 记录最高最低的值
        self._HighLowIng = {}
        # 开盘
        self._init = 0
        self._start_open = 0
        self._end_index = None

        # ticke number
        # == 0 is debug
        self._idx_number = 1

        self._gmt = 0
        # Period_Current = 0;
        Period_D1 = 1440
        # Period_H1 = 60;
        # Period_H4 = 240;
        Period_M = 5
        Period_M1 = 1
        # Period_M15 = 15;
        # Period_M30 = 30;
        # Period_MN1 = 43200;
        # Period_W1 = 10080;

        self._dir_to_freq = {"day": 9, "minline": 8}
        self._tf_to_dir = {Period_M1: "minline", Period_D1: "day"}

        self._dir_to_tf = {"minline": Period_M1, "day": Period_D1}

        self._update_sleep_time = 1

        # api or shell
        self._show_process = False

        # 涨跌停比
        self._upper_low_limit = 0

        self._dbstart = 0
        self._dblimit = 0
        self.TimeFrame = None

        self._index_data = None

        self._start_limit_date = None
        self._end_limit_date = None

        self._sym_id_hdf = "./code/SymId.hdf5"

        self._StockBar = StockBar()
        self._xdxr_day = 0

        self._backtest = False

    def __SetTimeFrame(self, tfame):
        self.TimeFrame = tfame

        # 默认以上证指数为对齐 symbol

        # days = self._tf_to_dir[tfame]

    def __onLine(self):
        '''
        交易的，发送数据到 kafka 去
        :return:
        '''
        # self.UpdateHistory(all=False)
        # self.Index("sh", "000001")

        self.__MarketToKafka()

        logger.info(self._end_index)

    def __InitHeader(self):
        '''
        发送header 使用的
        :param limit:
        :return:
        '''
        idx_day = int(str(self._start_index)[:10].replace("-", ""))

        for code in self._code:
            (symId, symbol_info) = self._dformat.add_symbol(code)
            self._symIds[code] = symId

            self._kf.publish(symbol_info)

            self._dxdxr.xdxr(self._kf.publish, code, symId, idx_day, 0)

        header = self._dformat.Index(b"index")

        self._kf.publish(header)

        store = pd.HDFStore(self._sym_id_hdf, 'a')

        if len(store.keys()) > 0:
            store.remove('data')

        dfsym = pd.DataFrame.from_dict(self._symIds, orient='index')
        # print(dfsym)
        store.put("data", dfsym, format="table")

        store.close()

        time.sleep(3)

    def __IndexData(self, btest_day=""):
        # 默认以上证指数为对齐 symbol

        days = self._tf_to_dir[self.TimeFrame]

        self._StockBar.Code('sh000001', timeframe=days)

        # 在这儿先固定一个时间，免得乱了
        if len(btest_day) > 0:
            wh = "index < '%s'" % btest_day
            self._StockBar.Condition(wh)

        self._index_data = self._StockBar.HDFRead(self._dblimit)

        # print(self._index_data.tail(2))
        if self._start_limit_date is not None:
            self._index_data = self._index_data[self._index_data.index > self._start_limit_date]

        if self._end_limit_date is not None:
            self._index_data = self._index_data[self._index_data.index <= self._end_limit_date]

        # print(self._index_data)
        if self._index_data.empty:
            logger.error("index is errro")
            exit(0)
        self._start_index = self._index_data.head(1).index.values[0]

        end_time = self._index_data.tail(1).index.values[0]
        # logger.info(end_time)
        if end_time != self._end_index and self._start_open == 1:
            # 检查是不是下一个周期了
            logger.info(end_time)
            logger.warning(self._HighLowIng)

        self._end_index = end_time

        logger.info("%s - %s" % (self._start_index, self._end_index))

    def __SymbolData(self):
        '''
        取各个品种的数据
        :param limit:
        :return:
        '''
        self._sym_dict = {}
        days = self._tf_to_dir[self.TimeFrame]

        for code in self._code:
            # logger.info("code:%s" % code)
            self._StockBar.Code(code, timeframe=days)

            reread = self._StockBar.HDFRead(self._dblimit)
            if reread is None:
                continue
            # print(reread.tail(3))
            stock = reread[["open", "low", "high", "close", "volume"]]
            self._sym_dict[code] = stock

    def __MarketToKafka(self):

        '''
        历史数据
        '''

        # custom data
        value = 5

        self.__CheckSymId()

        tatol_values = len(self._index_data.values)

        if not self._show_process:
            tatol_values = 0

        self._kf.ProgressBar(0, tatol_values, prefix="Progress:", suffix="Complete", length=50)
        ppbar = 0
        # 前一天收盘价格
        pre_price = {}
        # 取各个品种的数据
        self.__SymbolData()

        tick_num = 4

        for index, index_row in self._index_data.iterrows():

            index_bar = index_row[["open", "low", "high", "close", "volume"]].values
            # logger.info(index_bar)
            NowBarData = {}

            if index.tz is None:
                index = index.tz_localize(get_localzone_name())

            # logger.info(index.value)
            unix_time = self.__unixtime(index.value)

            idx_day = datetime.fromtimestamp(unix_time).strftime("%Y-%m-%d %H:%M:%S")
            # logger.info(unix_time)

            if not self._show_process:
                logger.info("start unix_time:%d, day:%s" % (unix_time, idx_day))

            xdxr_day = int(idx_day[:10].replace("-", ""))

            # 取各个 sym 的数据
            for sym in self._code:
                # 当前开盘的时候
                # 实际过程中，会自动更新 分红的
                if self._xdxr_day == 0 or self._xdxr_day != xdxr_day:
                    self._dxdxr.xdxr(self._kf.publish, sym, self._symIds[sym], xdxr_day, 1)
                    # logger.info("self:%d  day:%d "%(self._xdxr_day, xdxr_day))

                sym_tdx = self._sym_dict.get(sym)
                nullDatay = [0, 0, 0, 0, 0, 0]
                if sym_tdx is None:
                    # logger.error("sym tdx is empty:%s"%sym)
                    NowBarData[sym] = nullDatay
                    continue

                if sym_tdx.index.tz is None:
                    sym_tdx.index = sym_tdx.index.tz_localize(get_localzone_name())

                sym_day_data = sym_tdx[sym_tdx.index == index]

                # 停牌的数据

                if len(sym_day_data.values) == 0:
                    # logger.info("%s : %s" % (sym, index))

                    NowBarData[sym] = nullDatay
                else:
                    priceData = sym_day_data.values[0]
                    NowBarData[sym] = priceData

            self._xdxr_day = xdxr_day
            # 按ticket 发送价格
            pty_idx = tick_num
            # self._start_open == 0 从 open 开始，正式的时候从 low=1 开始
            # 正式开始之后，最高最低价如果没有变化的话，就发送 close 价格，免得会出现多次的最高最低
            for tick_idx in range(self._start_open, tick_num):
                self._mtickm.UinxTime(unix_time)

                for code in NowBarData:
                    price = NowBarData[code][tick_idx]

                    if tick_idx == 2 and (
                            code + "_high" not in self._HighLowIng or self._HighLowIng[code + "_high"] < price):
                        self._HighLowIng[code + "_high"] = price

                    if tick_idx == 1 and (
                            code + "_low" not in self._HighLowIng or self._HighLowIng[code + "_low"] > price):
                        self._HighLowIng[code + "_low"] = price

                    qty = NowBarData[code][pty_idx]

                    # if qty == 0:
                    #     logger.error("qty ==0 %s" % (idx_day))

                    if self._upper_low_limit > 0 and code in pre_price:
                        # 涨停
                        upper_last = pre_price[code] * (1.0 + self._upper_low_limit / 100.0)
                        # 跌停
                        lower_last = pre_price[code] * (1.0 - self._upper_low_limit / 100.0)

                        if upper_last > price > lower_last:
                            qty = 0

                            logger.info(
                                "%s : pre price: %.3f - upper price: %.3f - lower price: %.3f - price: %.3f  [%s]" % (
                                    code, pre_price[code], upper_last, lower_last, price, idx_day))

                    # if price == 0:
                    #     logger.error("price == 0 [code:%s idx: %d day:%s]" % (code, tick_idx, idx_day))
                    # logger.info("ids:%d"%self._symIds[code])
                    self._mtickm.Stock(self.TimeFrame, qty, price, self._idx_number,
                                       self._symIds[code])
                    stock_data = self._mtickm.toString()

                    if tick_idx == 3 and price > 0:
                        pre_price[code] = price

                    self._kf.publish(stock_data)

                price = index_bar[tick_idx]

                if tick_idx == 2 and ("index_high" not in self._HighLowIng or self._HighLowIng["index_high"] < price):
                    self._HighLowIng["index_high"] = price

                if tick_idx == 1 and ("index_low" not in self._HighLowIng or self._HighLowIng["index_low"] > price):
                    self._HighLowIng["index_low"] = price

                if price == 0:
                    logger.error("index idx: %d" % (tick_idx))

                self._mtickm.Index(self.TimeFrame, index_bar[pty_idx], price, self._idx_number,
                                   self._dformat.IndexCfiCode(), )
                idx_data = self._mtickm.toString()
                # logger.info(idx_day)
                self._kf.publish(idx_data)
                self._idx_number += 1

                if ppbar < 2:
                    time.sleep(1)
                else:
                    time.sleep(self._tick_sleep_time)

            ppbar += 1
            if not self._show_process:
                tatol_values = 0
            self._kf.ProgressBar(ppbar, tatol_values, prefix="Progress:", suffix="Complete : " + idx_day,
                                 length=50)
        logger.info("end data")

    def __online_market(self):
        '''
        会定时更新数据，再发送历史数据
        '''
        self._gmt = 0
        size_number = 0
        limit = -1
        _start_open = 1
        logger.info("start online market----")
        while True:

            stop = int(datetime.now().strftime("%H%M%S"))
            if (93000 <= stop < 113001) or (130000 <= stop < 150001):

                self.UpdateHistory(False)
                isUpdate = self.Index("sh", "000001")
                if isUpdate == 1:
                    self.__MarketToKafka(limit)
                    logger.info("online size:%d" % size_number)
                else:
                    logger.warning("not data to kafka")

            else:
                logger.info("sleep time:%d" % (stop))

            if stop > 150000:
                logger.info("stop")
                break

            time.sleep(60)
            size_number += 1

    def __unixtime(self, v):
        # offtime = self.TimeFrame * 60
        """
        在这儿转一下 on-open
        :param v:
        :return:
        """
        ret = int(v / self._unixtime)

        return ret

    def __ExitKafka(self):
        dformat = DataFormatProto(symId=self._symid, tick_time=self._tick_sleep_time)
        data = dformat.pExit()
        if self._kf is None:
            self._kf = kafka_producer("fix-events", "key", self._tick_sleep_time)
        self._kf.publish(data)

        print("quote_min end")

    def __init_market(self):

        logger.info("market start: %s " % (datetime.now().strftime("%H:%M:%S")))
        # a = 0
        self._kf = kafka_producer("fix-events", "key", self._tick_sleep_time)

        self._dformat = DataFormatProto(symId=self._symid, tick_time=self._tick_sleep_time)
        self._dxdxr = StockAXdxrMessage()
        self._mtickm = MarketTickMessage()

    def __CheckSymId(self):
        if len(self._symIds) == 0:
            store = pd.HDFStore(self._sym_id_hdf, 'r')

            dreads = store.get('data')

            dlist = dreads.to_dict(orient='dict')
            for key in dlist:
                self._symIds.update(dlist[key])

            store.close()

    def __xdxr(self):
        dxdxr = StockAXdxrMessage()

        for code in self._code:

            if code[:3] == "sh0":
                logger.info(code)
                continue
            dxdxr.update(code)

    def market(self, acode, days="minline", idx=0):
        '''
        读取本地数据就可以了
        先做一些自定义的 上证股票再说，以后再优化
        :return:
        '''

        self._code = []
        if acode is not None:
            if isinstance(acode, list):
                self._code = acode
            else:
                self._code = [acode]

        self._symid = idx

        logger.info(self._code)

        tfame = self._dir_to_tf[days]

        self.__SetTimeFrame(tfame)
        self.__init_market()
        self.__xdxr()

        if abs(self._dblimit) > 1:
            """
            初始化
            """
            self._StockBar.OnLineUpdate(self._code)
            self.__IndexData()
            self.__InitHeader()

            self.__MarketToKafka()

        else:
            # m = 0
            size_number = 0
            while True:

                stop = int(datetime.now().strftime("%H%M%S"))
                if (93000 <= stop < 113001) or (130000 <= stop < 150001):
                    try:
                        self._StockBar.OnLineUpdate(self._code)
                    finally:
                        logger.debug("update error")
                    self.__IndexData()
                    self.__MarketToKafka()
                    logger.info("online size:%d" % size_number)
                else:
                    logger.info("sleep time:%d" % (stop))

                if stop > 150000:
                    logger.info("stop")
                    break

                time.sleep(60)
                size_number += 1

    def BackTest(self, acode, days="day", idx=0):
        Period_D1 = 1440
        Period_H1 = 60
        Period_H4 = 240
        Period_M = 5
        Period_M1 = 1
        Period_M15 = 15
        Period_M30 = 30
        Period_MN1 = 43200
        Period_W1 = 10080

        self._backtest = True

        self._code = []
        if acode is not None:
            if isinstance(acode, list):
                self._code = acode
            else:
                self._code = [acode]

        self._symid = idx

        logger.info(self._code)
        self.__xdxr()

        tfame = self._dir_to_tf[days]

        self.__SetTimeFrame(tfame)
        self.__init_market()

        stop_day = "2024-08-01"
        # stop_day = "2022-07-29"
        self.__IndexData(btest_day=stop_day)

        self.__InitHeader()

        self.__MarketToKafka()

        self.__ExitKafka()

    def ULlimit(self, val=9.6):
        is_day = self._dir_to_tf['day'] == self.TimeFrame
        if is_day:
            self._upper_low_limit = val

    def gmt(self):
        self._gmt = 1

    def DbStart(self, val):
        self._dbstart = val

    def DbLimit(self, val):
        self._dblimit = val

    def showProcess(self):
        self._show_process = True

    def StartLimit(self, day=None):
        self._start_limit_date = day

    def EndLimit(self, day=None):
        self._end_limit_date = day

    def Stop(self):
        dformat = DataFormatProto(symId=self._symid, tick_time=self._tick_sleep_time)
        data = dformat.pExit()
        if self._kf is None:
            self._kf = kafka_producer("fix-events", "key", self._tick_sleep_time)
        self._kf.publish(data)
