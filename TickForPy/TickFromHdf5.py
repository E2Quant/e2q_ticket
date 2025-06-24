#!/usr/bin/env python
# -*- coding: utf-8 -*-
# @Time    : 2024/6/22 下午2:52
# @Author  : vyouzhi
# @File    : tdx_data.py

import sys

from QuantClient import QuantClient
from common import *

if __name__ == '__main__':

    code = ["600030", "601658", "600519"]

    logger.error("=== code list size: %d   === " % (len(code)))
    qm = QuantClient()

    if len(sys.argv) < 1:
        logger.warning("argv empty")
        exit(0)

    argvs = sys.argv[1:]

    argv_len = len(argvs)

    if argvs[0] == "market":
        '''
        按天的话，还得要更新 分红的文件
        开始 python TickFromHdf5.py market  -100
        之后 python TickFromHdf5.py market  -1
        '''
        if argv_len != 2:
            logger.error("argv")
            exit()

        limit = int(argvs[1])  # limit
        if limit > 0:
            limit = 0 - limit
        qm.DbLimit(limit)
        qm.gmt()

        qm.market(code)
    elif argvs[0] == "backtest":
        # 代码分配仓位的情况
        # python TickFromHdf5.py backtest day 100 0
        limit = 10
        tf = "day"
        runtime = 0
        if argv_len > 1:
            tf = argvs[1]  # tfame
            # print(tf)
        if argv_len > 2:
            limit = int(argvs[2])  # limit
        if argv_len > 3:
            runtime = int(argvs[3])

        qm.showProcess()
        qm.DbLimit(limit)
        qm.gmt()
        qm.ULlimit()

        # 设置限时
        # day = "2025-05-29"
        # qm.EndLimit(day)

        _idx = runtime % len(code)

        qm.BackTest(code[_idx], tf, _idx)

    elif argvs[0] == "btall":
        # 多个股票，采用 risk 来分配仓位的
        # python TickFromHdf5.py btall day -100 0
        # print(argv_len)
        limit = 10
        tf = "day"
        runtime = 0
        if argv_len > 1:
            tf = argvs[1]  # tfame
            # print(tf)
        if argv_len > 2:
            limit = int(argvs[2])  # limit
        if argv_len > 3:
            runtime = int(argvs[3])

        qm.showProcess()
        qm.DbLimit(limit)
        qm.gmt()
        qm.ULlimit()

        # 设置限时
        day = "2025-05-29"
        qm.EndLimit(day)

        _idx = runtime

        qm.BackTest(code, tf, _idx)

    elif argvs[0] == "stop":
        qm.Stop()