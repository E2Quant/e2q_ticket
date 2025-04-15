#!/usr/bin/env python
# -*- coding: utf-8 -*-
# @Time    : 2024/12/21 下午5:27
# @Author  : vyouzhi
# @File    : common.py
# @Software: PyCharm

import enum
import pandas as pd

from loguru import logger
from kafka import KafkaProducer

from time import sleep
import os
from datetime import datetime
import struct


class MsgType:
    INIT = b'I'
    XDXR = b'X'
    TICK = b'T'
    EXIT = b'E'
    LOG = b'L'


class Aligned:
    UNDER = b'U'
    PULL = b'P'


class InitType:
    INDEX = b'i'
    TRADE = b't'


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

        if isinstance(symbol, bytearray) == False:
            self.stock = symbol.encode('utf-8')
        else:
            self.stock = symbol

        self.cficode = cficode
        self.itype = self._it.TRADE
        self.offer_time = 0  # int(self._tick_sleep_time * self._number_deci)
        self.anligned = self._al.UNDER

    def toString(self):
        data = struct.pack("!c8sIcIc", self.msgtype, self.stock[:8],
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

    def __init__(self, symId=1, tick_time=0.05):
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
        return 0

    def thash(self):
        self._symId = self._symId + 1
        return self._symId

    def add_symbol(self, sym):
        symId = self.thash()

        self.init_data.Stock(sym, symId)
        data = self.init_data.toString()

        return (symId, data)

    def Index(self,  index_code):
        offer_time = int(self._tick_sleep_time * self._number_deci)
        print(offer_time)
        self.init_data.Index(index_code, offer_time)
        data = self.init_data.toString()
        return data

    def pExit(self):
        data = struct.pack("!c", MsgType.EXIT)
        return data


class StockAXdxrMessage(BaseMessage):
    def __init__(self,  xdxr=True):
        self._idxdxr = xdxr
        self._mt = MsgType()
        self._al = Aligned()

        # 转成整数
        self._number_deci = BaseMessage.number_deci

    def xdxr(self, kpush, sym, symId, days, init):
        """
        如果发现没有数据的话， mootdx bestip -v 这样来更新一下线路
        A 股的分红除权
        :return:
        """
        if self._idxdxr == False:
            return



        xdxr_file =  sym + ".csv"
        file_exit = os.path.isfile(xdxr_file)

        data = None
        if file_exit:
            data = pd.read_csv(xdxr_file)
        else:
            logger.bug("dd")
            return

        if init == 0:
            data_row = data[data.years <= days]
        else:
            data_row = data[data.years == days]

        if len(data_row.index) == 0:
            return

        # print(data_row.head(3))

        for index, row in data_row.iterrows():

            fenhong = int(self._number_deci * row['fenhong'])
            songzhuangu = int(self._number_deci * row['songzhuangu'])

            # outstanding,outstandend,mrketCaping
            data = struct.pack("!cIHHHHIIIIIc", self._mt.XDXR, symId,
                               row['year'], row['month'], row['day'], row['category'], fenhong, songzhuangu, 0, 0, 0, self._al.PULL)
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
        uinx_time_64 = uinx_time_64[2:]

        qty_64 = struct.pack("!Q", int(self._qty))
        qty_64 = qty_64[2:]

        price_64 = struct.pack("!Q", int(self._price))
        price_64 = price_64[2:]

        data = struct.pack("!cI6sHc6s6sIc", self.msgtype, self._CfiCode, uinx_time_64, self._frame, self._side,
                           price_64, qty_64, self._number, self.anligned)

        return data
    

class kafka_producer:
    def __init__(self, topic_name, key, sleep):
        self._topic = topic_name
        self._key = key
        self._sleep = sleep
        self._producer_instance = self.get_kafka_producer()

    def publish(self, value):
        logger.info(value)
        return
        try:
            if type(value) is not bytes:
                value_bytes = bytes(value, encoding='utf-8')
            else:
                value_bytes = value

            self._producer_instance.send(
                self._topic, key=None, value=value_bytes)
            self._producer_instance.flush()
            # sleep(self._sleep)
            # debug_print(f"Publish Succesful ({self._key}, {value}) -> {self._topic}")
        except Exception as ex:
            logger.info('Exception in publishing message')
            logger.info(str(ex))

    def get_kafka_producer(self, servers=['kafkaserver:9092']):
        _producer = None
        try:
            _producer = KafkaProducer(
                bootstrap_servers=servers, api_version=(0, 10))
        except Exception as ex:
            logger.info('Exception while connecting Kafka')
            logger.info(str(ex))
        finally:
            return _producer

    def debug_printProgressBar(self, iteration, total, prefix='', suffix='', decimals=1, length=100, fill='█',
                               debug_printEnd="\r"):
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
            debug_printEnd    - Optional  : end character (e.g. "\r", "\r\n") (Str)
        """
        # if total > 0:
        #     return
        percent = ("{0:." + str(decimals) + "f}").format(100 *
                                                         (iteration / float(total)))
        filledLength = int(length * iteration // total)
        bar = fill * filledLength + '-' * (length - filledLength)
        print(f'\r{prefix} |{bar}| {percent}% {suffix}', end=debug_printEnd)
        # debug_print New Line on Complete
        if iteration == total:
            print()

    def end(self):
        close_data = "."
        self.publish(close_data)
