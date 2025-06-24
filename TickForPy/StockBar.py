import os

import pandas as pd
import pandas.api.types as pdt
from loguru import logger
from tzlocal import get_localzone_name


class StockBar(object):
    """$"""

    def __init__(self):
        """Constructor for $"""
        self._fname = ""
        self._code = ""
        self._dir = "./bar"

        self._ext = "hdf5"

        self._dbstart = 0

        self._gmt = 0
        self._dir_to_freq = {"day": 9, "minline": 8}
        self._where = ""

    def Condition(self, where=""):
        """
        wh = "index < '2025-05-13'"
        """
        self._where = where

    def gmt(self):
        self._gmt = 1

    def Code(self, code, timeframe, market='sh'):
        self._code = code

        if len(code) > 6:
            market = code[:2]
        else:
            code = market + code

        self._fname = "%s/%s/%s_%s.%s" % (self._dir, timeframe, code, timeframe, self._ext)

    def HDFRead(self, limit=-1):
        """
        hdf read
        :param path:
        :param limit:
        :return:
        """

        if len(self._fname) == 0:
            logger.error("code not init")
            return None

        ok = os.path.isfile(self._fname)
        if not ok:
            logger.error(self._fname)
            return None
        store = pd.HDFStore(self._fname, 'r')

        _limit = limit
        if len(self._where) > 0:
            _limit = 0

        if _limit < 0:
            reread = store.select('data', start=limit, where=self._where)
        elif _limit == 0:
            # logger.error(self._where)
            reread = store.select('data', where=self._where)
        else:
            reread = store.select('data', start=self._dbstart, stop=limit, where=self._where)

        store.close()

        if not pdt.is_datetime64_dtype(reread.index.dtype):
            reread.index = pd.to_datetime(reread.index)

        if self._gmt > 0:
            reread = reread.tz_localize(get_localzone_name())

        if len(self._where) > 0:
            if limit > 0:
                return reread.head(limit)
            elif limit == 0:
                return reread
            else:
                return reread.tail(abs(limit))
        return reread

    def GetLast(self):
        """
        取数据
        :param fname:
        :param limit:
        :return:
        """
        limit = -1
        reread = self.HDFRead(limit)

        return reread

    def showLog(self, code, timeframe, _offset=0):
        # self._gmt = 0

        offset = _offset
        if offset == 0:
            offset = -10
            if timeframe == "fzline":
                offset = -60
            elif timeframe == "minline":
                offset = -245

        reread = self.HDFRead(offset)
        if reread is None:
            logger.error("error %s" % code)
            return
        logger.info(reread.info(verbose=True))
        logger.info(reread)

        reread = reread.reset_index()

        jdata = reread.to_json(orient='records', date_format='iso')

        return jdata

def run():
    sb = StockBar()
    code = "601658"

    sb.Code(code, timeframe="day")
    df = sb.HDFRead(-20)
    print(df)

if __name__ == '__main__':
    run()
