#!/usr/bin/env python
# -*- coding: utf-8 -*-
# @Time    : 2024/12/21 下午5:27
# @Author  : vyouzhi
# @File    : common.py
# @Software: PyCharm
import os
import struct

import pandas as pd
from kafka import KafkaProducer
from loguru import logger


class MsgType:
    INIT = b'I'
    XDXR = b'X'
    TICK = b'T'
    CUSTOM = b'C'
    EXIT = b'E'
    LOG = b'L'


class Aligned:
    # 进行中
    UNDER = b'U'
    # 完成
    PULL = b'P'


class InitType:
    INDEX = b'i'
    TRADE = b't'


class CmType:
    UINT16 = b'6'
    UINT32 = b'2'
    UINT64 = b'4'


class SystemInitMessage:
    '''
    初始化的
    '''

    def __init__(self):
        self._mt = MsgType()
        self._it = InitType()
        self._al = Aligned()

        self.msgtype = self._mt.INIT
        self.stock = ""
        self.cficode = 0
        self.itype = self._it.INDEX
        self.offer_time = 0
        self.anligned = self._al.UNDER

    def Index(self, index_code, tick_time):
        self.stock = index_code
        self.cficode = 0
        self.itype = self._it.INDEX
        # int(self._tick_sleep_time * self._number_deci)
        self.offer_time = tick_time
        self.anligned = self._al.PULL

    def Stock(self, symbol, cficode):

        if not isinstance(symbol, bytearray):
            self.stock = symbol.encode('utf-8')
        else:
            self.stock = symbol

        self.cficode = cficode
        self.itype = self._it.TRADE
        self.offer_time = 0  # int(self._tick_sleep_time * self._number_deci)
        self.anligned = self._al.UNDER

    def toString(self):
        # logger.info(self.stock)
        data = struct.pack("!c9sIcIc", self.msgtype, self.stock[:8],
                           self.cficode, self.itype, self.offer_time, self.anligned)
        return data


class BaseMessage:
    '''
    整数的精度
    '''
    number_deci = 10000.0


class DataFormatProto(BaseMessage):
    '''
    Q = uint64
    I = uint32
    H = uint16
    s = Alpha
    [x]s = char[x]
    fmt = cIH8sIH
    '''

    def __init__(self, symId=0, tick_time=0.05):
        # 自己定义 id
        self._symId = symId
        # 当前一笔 ticket 报价有多少个 symbol
        self._tick_num = 0
        self._tick_data = ""

        # 间隔报价的时间
        self._tick_sleep_time = tick_time
        # 转成整数
        self._number_deci = BaseMessage.number_deci

        self.init_data = SystemInitMessage()

    def IndexCfiCode(self):
        '''
        指数代码
        '''
        return 0

    def thash(self):
        '''
        cfi code index
        '''
        self._symId = self._symId + 1
        return self._symId

    def add_symbol(self, sym):
        '''
        订阅 symbol

        '''
        symId = self.thash()

        self.init_data.Stock(sym, symId)
        data = self.init_data.toString()
        # logger.info(symId)
        return (symId, data)

    def Index(self, index_code):
        '''
        转成 二进制 char
        '''
        offer_time = int(self._tick_sleep_time * self._number_deci)
        # print(offer_time)
        self.init_data.Index(index_code, offer_time)
        data = self.init_data.toString()
        return data

    def pExit(self):
        '''

        退出
        '''
        data = struct.pack("!c", MsgType.EXIT)
        return data


class StockAXdxrMessage(BaseMessage):
    def __init__(self):

        self._mt = MsgType()
        self._al = Aligned()
        self._dir = "./xdxr/"

        # 转成整数
        self._number_deci = BaseMessage.number_deci

    def update(self, sym):
        '''

        '''

        if sym[:3] == "sh0":
            logger.info(sym)
            return None

        if len(sym) > 6:
            sym = sym[2:]

        if sym[1] not in ['0', '6']:
            return None

        xdxr_hdf5 = self._dir + sym + "_xdxr.hdf5"

        if not os.path.isdir(self._dir):
            os.makedirs(self._dir)

        store = pd.HDFStore(xdxr_hdf5, 'a')
        data = None
        if len(store.keys()) > 0:
            data = store.select('data')
        else:
            logger.error("xdxr sym:%s , not found", sym)

        store.close()

        return data

    def getData(self, sym, days, init):
        data = self.update(sym)
        if data is None or data.empty:
            logger.error("xdxr code %s, data is none" % (sym))
            return None

        if init == 0:
            data_row = data[data.years <= days]
        else:
            data_row = data[data.years == days]

        if len(data_row.index) == 0:
            return None

        return data_row

    def xdxr(self, kpush, sym, symId, days, init):
        """
        A 股的分红除权
        :return:
        """

        data_row = self.getData(sym, days, init)
        if data_row is None:
            # logger.error("sym:%s, days:%d, init:%d" % (sym, days, init))
            return
        ## 10 送 xx.xx
        uint = 10
        for index, row in data_row.iterrows():
            fenhong = row['fenhong']

            ## 倍数
            songzhuangu = row['songzhuangu']

            fenhong = int(self._number_deci * fenhong)
            songzhuangu = int(self._number_deci * songzhuangu)
            # logger.info("%d %d" % (fenhong, songzhuangu))

            # outstanding,outstandend,mrketCaping
            data = struct.pack("!cIHHHHIIIIIHc", self._mt.XDXR, symId,
                               row['year'], row['month'], row['day'], row['category'], fenhong, songzhuangu, 0, 0, 0,
                               uint,
                               self._al.PULL)
            kpush(data)


class MarketTickMessage(BaseMessage):
    '''
    报价
    '''

    def __init__(self):
        self._mt = MsgType()
        self._al = Aligned()

        # 转成整数
        self._number_deci = BaseMessage.number_deci

        self._CfiCode = 0
        self._unix_time = 0
        self._frame = 0
        self._side = b'B'
        self._price = 0
        self._qty = 0
        self._number = 0
        self.anligned = self._al.UNDER

    def UinxTime(self, time):
        self._unix_time = time
        # 转成毫秒
        if len(str(self._unix_time)) < 11:
            self._unix_time *= 1000

    def data(self, frame, qty, price, number, stock):
        self._CfiCode = stock
        self._qty = qty
        self._frame = frame

        self._price = int(price * self._number_deci)
        self._number = number

    def Stock(self, frame, qty, price, number, stock):
        self.data(frame, qty, price, number, stock)
        self.anligned = self._al.UNDER

    def Index(self, frame, qty, price, number, stock):
        self.data(frame, qty, price, number, stock)
        self.anligned = self._al.PULL

    def toString(self):
        '''
        Q = uint64
        I = uint32
        H = uint16
        s = Alpha
        [x]s = char[x]
        fmt = cIH8sIH
        '''
        self.msgtype = self._mt.TICK
        uinx_time_64 = struct.pack("!Q", self._unix_time)

        qty_64 = struct.pack("!Q", int(self._qty))
        qty_64 = qty_64[2:]

        price_64 = struct.pack("!Q", int(self._price))
        price_64 = price_64[2:]

        data = struct.pack("!cI8sHc6s6sIc", self.msgtype, self._CfiCode, uinx_time_64, self._frame, self._side,
                           price_64, qty_64, self._number, self.anligned)

        return data


class CustomMsg(BaseMessage):
    """
    自定义数据
    """

    def __init__(self, cfi, mtype):
        """Constructor for CustomMsg"""
        self._mt = MsgType()
        self._al = Aligned()

        # 转成整数
        self._number_deci = BaseMessage.number_deci

        self._CfiCode = cfi
        self._index = 0
        self._size = 0
        self._values = b''
        self._mtype = mtype  # CmType()

    def data(self, value):
        _value = int(value * self._number_deci)

        type = CmType()
        self._size += 1
        if self._mtype == type.UINT16:
            uint_16 = struct.pack("!H", _value)
            self._values += uint_16
        elif self._mtype == type.UINT32:
            uint_32 = struct.pack("!I", _value)
            self._values += uint_32
        elif self._mtype == type.UINT64:
            uint_64 = struct.pack("!Q", _value)
            self._values += uint_64

    def toString(self, end=True):
        msgtype = self._mt.CUSTOM
        data = struct.pack("!cIHHc", msgtype, self._CfiCode, self._index, self._size, self._mtype)
        data += self._values
        if end:
            data += struct.pack("!c", self._al.PULL)
        else:
            data += struct.pack("!c", self._al.UNDER)
        return data


class kafka_producer:
    def __init__(self, topic_name, key, sleep):
        self._topic = topic_name
        self._key = key
        self._sleep = sleep
        self._producer_instance = self.get_kafka_producer()

    def publish(self, value):

        try:
            if type(value) is not bytes:
                value_bytes = bytes(value, encoding='utf-8')
            else:
                value_bytes = value

            self._producer_instance.send(
                self._topic, key=None, value=value_bytes)
            self._producer_instance.flush()
            # sleep(self._sleep)
            # logger.error(f"Publish Succesful ({self._key}, {value}) -> {self._topic}")
        except Exception as ex:
            logger.error('Exception in publishing message')
            logger.error(str(ex))

    def get_kafka_producer(self, servers=['kafkaserver:9092']):
        _producer = None
        try:
            _producer = KafkaProducer(
                bootstrap_servers=servers, api_version=(0, 10))
        except Exception as ex:
            logger.error('Exception while connecting Kafka')
            logger.error(str(ex))
        finally:
            return _producer

    def ProgressBar(self, iteration, total, prefix='', suffix='', decimals=1, length=100, fill='█',
                    End="\r"):
        """
        Call in a loop to create terminal progress bar
        @params:
            iteration   - Required  : current iteration (Int)
            total       - Required  : total iterations (Int)
            prefix      - Optional  : prefix string (Str)
            suffix      - Optional  : suffix string (Str)
            decimals    - Optional  : positive number of decimals in percent complete (Int)
            length      - Optional  : character length of bar (Int)
            fill        - Optional  : bar fill character (Str)
            logger.errorEnd    - Optional  : end character (e.g. "\r", "\r\n") (Str)
        """
        if total == 0:
            return
        percent = ("{0:." + str(decimals) + "f}").format(100 *
                                                         (iteration / float(total)))
        filledLength = int(length * iteration // total)
        bar = fill * filledLength + '-' * (length - filledLength)
        print(f'\r{prefix} |{bar}| {percent}% {suffix}', end=End)
        # logger.error New Line on Complete
        if iteration == total:
            print()

    def end(self):
        close_data = "."
        self.publish(close_data)
