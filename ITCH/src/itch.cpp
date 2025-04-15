/*
 * =====================================================================================
 *
 *       Filename:  itch.cpp
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2024年04月09日 10时21分18秒
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

#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif  // _WIN32

#include "itch50.hpp"

/*
 * ===  FUNCTION  =============================
 *
 *         Name:  itch
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void itch(std::string path, std::string symbol, std::string brokers,
          std::string topic, std::uint8_t inter, int limit)
{
    e2q::ItchMsg im(path, inter, limit);
    int r = im.init_kafka(brokers, topic);
    if (r == -1) {
        exit(-1);
    }
    else {
        im.read(symbol);
    }

} /* -----  end of function itch  ----- */

/*
 * ===  FUNCTION  =============================
 *         Name:  main
 *  Description:
 * ============================================
 */
int main(int argc, char* argv[])
{
    int h;
    char* c = nullptr;
    char* s = nullptr;

    std::string symbol = "";
    std::string itch5_file = "";

    std::string brokers = "";
    std::string topic = "";

    std::uint8_t interval = 100;
    int limit = 100;

    std::string help =
        "%s -c  itch5 file\n \
                Usage: \n \
                -s which loading symbol name \n \
                -c which loading itch5 file \n \
                -b which loading kafka brokers \n \
                -t which loading kafka topic \n \
                -i which loading interval ticket time \n \
                -l which loading limit ticket  \n \
                 \n";

    if (argc < 2) {
        std::cout << help << std::endl;
        exit(-1);
    }

    while ((h = getopt(argc, argv, "Di:l:c:s:b:t:")) != -1) {
        switch (h) {
            case 'i':
                if (optarg != nullptr) {
                    interval = atoi(optarg);
                }

                break;
            case 'l':
                if (optarg != nullptr) {
                    limit = atoi(optarg);
                }

                break;
            case 'D':
                // Debug
                break;
            case 'c':
                c = optarg;
                if (c != nullptr) {
                    itch5_file = std::string(c);
                }
                break;
            case 's':
                s = optarg;
                if (s != nullptr) {
                    symbol = std::string(s);
                }
                break;
            case 'b':
                if (optarg != nullptr) {
                    brokers = std::string(optarg);
                }
                break;
            case 't':
                if (optarg != nullptr) {
                    topic = std::string(optarg);
                }
                break;
            default:
                printf("%s --help\n", argv[0]);
                exit(-1);

        } /* -----  end switch  ----- */
    }

    itch(itch5_file, symbol, brokers, topic, interval, limit);

    return EXIT_SUCCESS;
} /* ----------  end of function main  ---------- */
