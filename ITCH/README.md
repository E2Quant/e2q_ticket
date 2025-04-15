# nasdaq ITCH 50

nasdaq trader pdf
https://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTVITCHSpecification.pdf

### Executing program

* [下载文件](https://emi.nasdaq.com/ITCH/Nasdaq%20ITCH/)
*  目前在 debian 12 上面测试通过
```
cd ITCH
mkdir build
cmake ../
make
hi
./itch -c /opt/01302020.NASDAQ_ITCH50 -s TSLA  -b kafkaserver:9092 -t fix-events 
```
