/*
 * =====================================================================================
 *
 *       Filename:  zts.cpp
 *
 *    Description:  zts
 *
 *        Version:  1.0
 *        Created:  2025/01/23 17时31分47秒
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

#include <stdlib.h>
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

#include "FeedPack/QuoteEvent.hpp"
#include "FileUtils.h"
/*
 * ===  FUNCTION  =============================
 *
 *         Name:  zts_server
 *  ->  void *
 *  Parameters:
 *  - size_t  arg
 *  Description:
 *
 * ============================================
 */
void zts_server(int argc, char* argv[])
{
    int h;
    char* c = nullptr;
    char* s = nullptr;

    FileUtils* fileUtils = NULL;

    std::string help =
        "%s -c config.json \n \
                Usage: \n \
                -c which loading config.json \n \
                -p which loading db properties \n \
                -D debug zts \n \
                -d daemon run \n";

    if (argc < 2) {
        std::cout << help << std::endl;
        exit(-1);
    }

    while ((h = getopt(argc, argv, "dDc:p:")) != -1) {
        switch (h) {
            case 'd':
                // deamon
                break;
            case 'D':
                // Debug
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
            case 'p':
                s = optarg;
                if (s != nullptr) {
                    printf("db properties: %s\n", s);
                }
                break;
            default:
                printf("%s --help\n", argv[0]);
                exit(-1);

        } /* -----  end switch  ----- */
    }

    e2q::QuoteEvent qe(fileUtils);
    qe.run();
    if (fileUtils == nullptr) {
        exit(-1);
    }

    if (fileUtils != nullptr) {
        delete fileUtils;
        fileUtils = NULL;
    }
} /* -----  end of function zts_server  ----- */

/*
 * ===  FUNCTION  =============================
 *         Name:  main
 *  Description:
 * ============================================
 */
int main(int argc, char* argv[])
{
    zts_server(argc, argv);
    return EXIT_SUCCESS;
} /* ----------  end of function main  ---------- */
