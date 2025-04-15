// testTradeApi.cpp : 定义控制台应用程序的入口点。
//

#include <stdlib.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>

#include "xtp_api_data_type.h"
#include "xtp_trader_api.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif  // _WIN32

#include "FileUtils.h"
#include "quote_spi.h"
#include "trade_spi.h"
#include "xtp_quote_api.h"

XTP::API::TraderApi* pUserApi;
bool is_connected_ = false;
std::string trade_server_ip;
int trade_server_port;
std::map<uint64_t, uint64_t> map_session;
uint64_t* session_arrary = NULL;
FileUtils* fileUtils = NULL;
XTPOrderInsertInfo* orderList = NULL;

XTP_PROTOCOL_TYPE quote_protocol = XTP_PROTOCOL_UDP;

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  quote
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void quote(FileUtils* futils)
{
    std::string quote_username;
    std::string quote_password;
    std::string quote_server_ip;
    int quote_server_port;

    int client_id = futils->intForKey("client_id");
    std::string account_key = futils->stdStringForKey("account_key");
#ifdef _WIN32
    std::string filepath = futils->stdStringForKey("path");
#else
    std::string filepath = futils->stdStringForKey("path_linux");
#endif  // _WIN32
    // 读取行情配置
    quote_server_ip = futils->stdStringForKey("quote_ip");
    quote_server_port = futils->intForKey("quote_port");
    quote_username = futils->stdStringForKey("quote_user");
    quote_password = futils->stdStringForKey("quote_password");
    quote_protocol = (XTP_PROTOCOL_TYPE)(futils->intForKey("quote_protocol"));
    int32_t quote_buffer_size = futils->intForKey("quote_buffer_size");

    // 读取心跳超时配置
    int32_t heat_beat_interval = futils->intForKey("hb_interval");

    // 初始化行情api
    XTP::API::QuoteApi* pQuoteApi = XTP::API::QuoteApi::CreateQuoteApi(
        client_id, filepath.c_str(),
        XTP_LOG_LEVEL_DEBUG);  // log日志级别可以调整
    if (!pQuoteApi) {
        std::cout
            << "Failed to create quote api, please check the input parameters."
            << std::endl;
        return;
    }
    MyQuoteSpi* pQuoteSpi = new MyQuoteSpi();
    if (!pQuoteSpi) {
        std::cout
            << "Failed to create quote spi, please check the input parameters."
            << std::endl;
        return;
    }
    pQuoteApi->RegisterSpi(pQuoteSpi);

    // 设定行情服务器超时时间，单位为秒
    pQuoteApi->SetHeartBeatInterval(heat_beat_interval);  // 此为1.1.16新增接口
    // 设定行情本地缓存大小，单位为MB
    pQuoteApi->SetUDPBufferSize(quote_buffer_size);  // 此为1.1.16新增接口

    int loginResult_quote = -1;
    // 登录行情服务器,自1.1.16开始，行情服务器支持UDP连接，推荐使用UDP
    loginResult_quote = pQuoteApi->Login(
        quote_server_ip.c_str(), quote_server_port, quote_username.c_str(),
        quote_password.c_str(), quote_protocol);

    if (loginResult_quote == 0) {
        std::cout << "login ok" << std::endl;
        // 登录行情服务器成功后，订阅行情
        int instrument_count = futils->countForKey("quote_ticker.instrument");
        int quote_exchange = futils->intForKey("quote_ticker.exchange");

        // 从配置文件中读取需要订阅的股票
        char** allInstruments = new char*[instrument_count];
        for (int i = 0; i < instrument_count; i++) {
            allInstruments[i] = new char[XTP_TICKER_LEN];
            std::string instrument =
                futils->stdStringForKey("quote_ticker.instrument[%d]", i);
            strncpy(allInstruments[i], instrument.c_str(), XTP_TICKER_LEN);
        }

        // 开始订阅,注意公网测试环境仅支持TCP方式，如果使用UDP方式会没有行情数据，实盘大多数使用UDP连接
        pQuoteApi->SubscribeMarketData(allInstruments, instrument_count,
                                       (XTP_EXCHANGE_TYPE)quote_exchange);

        pQuoteApi->QueryAllTickersFullInfo(XTP_EXCHANGE_SH);

        // 保持主线程，防止程序退出
        int m = 0;
        while (m < 10) {
#ifdef _WIN32
            Sleep(10000);
#else
            sleep(10);
#endif  // _WIN32

            m++;
        }

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
        XTPRI* error_info = pQuoteApi->GetApiLastError();
        std::cout << "Login to server error, " << error_info->error_id << " : "
                  << error_info->error_msg << std::endl;
    }

} /* -----  end of function quote  ----- */

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  trader
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void trader(FileUtils* futils)
{
    // 读取交易配置
    trade_server_ip = futils->stdStringForKey("trade_ip");
    trade_server_port = futils->intForKey("trade_port");
    bool auto_save = futils->boolForKey(
        "auto_save");  // 是否将回报数据落盘，此参数仅在此demo中使用，仅涉及demo回调函数中逻辑，不是必需的
    int client_id = futils->intForKey("client_id");
    int account_count = futils->countForKey("account");
    int resume_type = futils->intForKey("resume_type");
    std::string account_key = futils->stdStringForKey("account_key");
#ifdef _WIN32
    std::string filepath = futils->stdStringForKey("path");
#else
    std::string filepath = futils->stdStringForKey("path_linux");
#endif  // _WIN32
    bool ping_pong_test = futils->boolForKey(
        "ping_pong_test");  // demo是否开启乒乓测试，默认是不开启的，如果需要开启，请在配置文件中修改，此参数仅在此demo中使用，仅涉及demo回调函数中逻辑，不是必需的

    // 读取心跳超时配置
    int32_t heat_beat_interval = futils->intForKey("hb_interval");

    if (account_count > 0) {
        // 针对多用户的情况
        orderList = new XTPOrderInsertInfo[account_count];
        memset(orderList, 0, sizeof(XTPOrderInsertInfo) * account_count);
    }

    // 初始化交易类Api
    pUserApi = XTP::API::TraderApi::CreateTraderApi(
        client_id, filepath.c_str(),
        XTP_LOG_LEVEL_DEBUG);  // 创建UserApi，log日志级别可以调整
    if (!pUserApi) {
        std::cout
            << "Failed to create trader api, please check the input parameters."
            << std::endl;
        return;
    }
    pUserApi->SubscribePublicTopic((XTP_TE_RESUME_TYPE)resume_type);
    pUserApi->SetSoftwareVersion(
        "1.1.0");  // 设定此软件的开发版本号，用户自定义
    pUserApi->SetSoftwareKey(
        account_key
            .c_str());  // 设定用户的开发代码，在XTP申请开户时，由xtp人员提供
    pUserApi->SetHeartBeatInterval(
        heat_beat_interval);  // 设定交易服务器超时时间，单位为秒，此为1.1.16新增接口
    MyTraderSpi* pUserSpi = new MyTraderSpi();
    if (!pUserSpi) {
        std::cout
            << "Failed to create trader spi, please check the input parameters."
            << std::endl;
        return;
    }
    pUserApi->RegisterSpi(pUserSpi);  // 注册事件类
    pUserSpi->setUserAPI(pUserApi);
    pUserSpi->set_save_to_file(auto_save);
    pUserSpi->set_ping_pong_test_flag(
        ping_pong_test);  // 设置demo是否开启乒乓报单测试

    uint64_t session_id_ =
        0;  // 标识是否有用户登陆成功，存储第一个可用的session_id
    if (account_count > 0) {
        // 多用户时，用session数组来管理用户session_id
        session_arrary = new uint64_t[account_count];

        // 所有用户挨个登陆
        for (int i = 0; i < account_count; i++) {
            // 从配置文件中读取第i个用户登录信息
            std::string account_name =
                futils->stdStringForKey("account[%d].user", i);
            std::string account_pw =
                futils->stdStringForKey("account[%d].password", i);

            uint64_t temp_session_ = 0;
            std::cout << account_name << " login begin." << std::endl;
            temp_session_ =
                pUserApi->Login(trade_server_ip.c_str(), trade_server_port,
                                account_name.c_str(), account_pw.c_str(),
                                XTP_PROTOCOL_TCP);  // 第i个用户登录交易服务器

            if (session_id_ == 0) {
                session_id_ = temp_session_;
            }

            if (temp_session_ > 0) {
                // 登录成功后，建立session_id与用户之间的映射关系
                map_session.insert(std::make_pair(temp_session_, i));
            }
            else {
                // 登录失败，获取登录失败原因
                XTPRI* error_info = pUserApi->GetApiLastError();
                std::cout << account_name << " login to server error, "
                          << error_info->error_id << " : "
                          << error_info->error_msg << std::endl;
            }

            session_arrary[i] = temp_session_;
        }
    }

    if (session_id_ > 0) {
        // 有用户成功登录交易服务器
        std::cout << "Login to server success." << std::endl;

        is_connected_ = true;

        // 每个用户均根据配置文件报单
        for (int i = 0; i < account_count; i++) {
            // 从配置文件中读取报单信息
            int j = 0;
            orderList[i].order_client_id = i;
            std::string instrument =
                futils->stdStringForKey("order[%d].instrument_id", j);
            strncpy(orderList[i].ticker, instrument.c_str(), XTP_TICKER_LEN);
            orderList[i].market =
                (XTP_MARKET_TYPE)futils->intForKey("order[%d].exchange", j);
            orderList[i].price = futils->floatForKey("order[%d].price", j);
            orderList[i].quantity = futils->intForKey("order[%d].quantity", j);
            orderList[i].side =
                (XTP_SIDE_TYPE)futils->intForKey("order[%d].side", j);
            orderList[i].price_type =
                (XTP_PRICE_TYPE)futils->intForKey("order[%d].price_type", j);
            orderList[i].business_type = (XTP_BUSINESS_TYPE)futils->intForKey(
                "order[%d].business_type",
                j);  // 此为1.1.16新增字段，普通业务使用0，其他业务请参考注释
            orderList[i]
                .position_effect = (XTP_POSITION_EFFECT_TYPE)futils->intForKey(
                "order[%d].position_effect",
                j);  // 此为1.1.18新增字段,供期权、信用业务使用，普通业务均使用0

            if (session_arrary[i] == 0) {
                // 用户登录不成功时，跳过
                continue;
            }

            int m = 1;
            if (m == 0) {
                // 发送报单
                int64_t xtp_id =
                    pUserApi->InsertOrder(&(orderList[i]), session_arrary[i]);
                if (xtp_id == 0) {
                    // 报单不成功时，等待一会儿，跳过
#ifdef _WIN32
                    Sleep(1000);
#else
                    sleep(1);
#endif  // WIN32
                    continue;
                }

                // 报单发送成功输出日志
                std::string account_name =
                    futils->stdStringForKey("account[%d].user", i);
                std::cout << account_name << " insert order success."
                          << std::endl;
            }

            if (m == 1) {
                pUserApi->QueryAsset(session_arrary[i], 1);
                pUserApi->QueryPosition(nullptr, session_arrary[i], 1);
            }
        }
    }

    // 保持主线程，防止程序退出
    int m = 0;
    while (m < 10) {
#ifdef _WIN32
        Sleep(10000);
#else
        sleep(10);
#endif  // _WIN32

        m++;
    }

    if (orderList) {
        delete[] orderList;
        orderList = NULL;
    }

    if (session_arrary) {
        delete[] session_arrary;
        session_arrary = NULL;
    }
} /* -----  end of function trader  ----- */

/*
 * ===  FUNCTION  =============================
 *         Name:  main
 *  Description:
 * ============================================
 */
int main_test(int argc, char* argv[])
{
    int h;
    char* c = nullptr;
    char* s = nullptr;

    FileUtils* fileUtils = NULL;

    std::string help =
        "%s -e script.e2 \n \
                Usage: \n \
                -e which loading ea e2l script \n \
                -s which loading oms e2l script \n \
                -p which loading db properties \n \
                -l debug e2l \n \
                -d daemon run \n";

    if (argc < 2) {
        std::cout << help << std::endl;
        exit(-1);
    }

    while ((h = getopt(argc, argv, "dc:s:")) != -1) {
        switch (h) {
            case 'd':
                break;
            case 'c':
                c = optarg;
                if (c != nullptr) {
                    printf("config: %s\n", c);

                    fileUtils = new FileUtils();
                    if (!fileUtils->init(c)) {
                        std::cout << "The config.json file parse error."
                                  << std::endl;
#ifdef _WIN32
                        system("pause");
#endif

                        exit(-1);
                    }
                }
                break;
            case 's':
                s = optarg;
                if (s != nullptr) {
                    printf("config: %s\n", s);
                }
                break;
            default:
                printf("%s --help\n", argv[0]);
                exit(-1);

        } /* -----  end switch  ----- */
    }
    if (fileUtils == nullptr) {
        exit(-1);
    }
    // quote(fileUtils);
    //

    trader(fileUtils);

    if (fileUtils != nullptr) {
        delete fileUtils;
        fileUtils = NULL;
    }
    return EXIT_SUCCESS;
} /* ----------  end of function main  ---------- */

int maini()
{
    fileUtils = new FileUtils();
    char p[] = "config.json";
    if (!fileUtils->init(p)) {
        std::cout << "The config.json file parse error." << std::endl;
#ifdef _WIN32
        system("pause");
#endif

        return 0;
    }
    int account_count = fileUtils->countForKey("account");
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///------------------------------------如果用户想要下算法母单，可以参考以下代码，否则可以注释掉--------------------------------------------------------------///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::string server_ip_algo = fileUtils->stdStringForKey("ip_algo");
    int server_port_algo = fileUtils->intForKey("port_algo");
    std::string account_pw_algo = fileUtils->stdStringForKey("password_algo");
    std::string account_name_algo = fileUtils->stdStringForKey("account_algo");
    int login_ret = 0;
    /// 登录AlgoBus算法服务器
    std::cout << account_name_algo << " begin to login AlgoBus." << std::endl;
    login_ret = pUserApi->LoginALGO(server_ip_algo.c_str(), server_port_algo,
                                    account_name_algo.c_str(),
                                    account_pw_algo.c_str(), XTP_PROTOCOL_TCP);
    if (login_ret != 0) {
        std::cout << account_name_algo << " login to AlgoBus error!!!!!!!!!!!"
                  << std::endl;
    }
    else {
        std::cout << account_name_algo << " login to AlgoBus success."
                  << std::endl;

        /// 在用户成功登录交易服务器后，算法用户建立算法通道
        // 假设每个登陆成功的用户均需要下算法母单，挨个建立算法通道
        for (int i = 0; i < account_count; i++) {
            if (session_arrary[i] == 0) {
                // 用户登录不成功时，跳过
                continue;
            }

            // 从配置文件中读取第i个用户登录信息
            std::string account_name_oms =
                fileUtils->stdStringForKey("account[%d].user", i);
            std::string account_pw_oms =
                fileUtils->stdStringForKey("account[%d].password", i);

            std::cout << account_name_oms << " begin to establish channel."
                      << std::endl;
            int user_ret = pUserApi->ALGOUserEstablishChannel(
                trade_server_ip.c_str(), trade_server_port,
                account_name_oms.c_str(), account_pw_oms.c_str(),
                session_arrary[i]);
            if (user_ret != 0) {
                std::cout << account_name_oms
                          << " establish channel send error!!!!!!!!!!!"
                          << std::endl;
            }
            else {
                std::cout << account_name_oms
                          << " establish channel send success." << std::endl;
            }
        }
    }

    // 主线程循环，防止进程退出
    while (true) {
#ifdef _WIN32
        Sleep(1000);
#else
        sleep(1);
#endif  // _WIN32
    }

    delete fileUtils;
    fileUtils = NULL;

    if (orderList) {
        delete[] orderList;
        orderList = NULL;
    }

    if (session_arrary) {
        delete[] session_arrary;
        session_arrary = NULL;
    }

    return 0;
}
