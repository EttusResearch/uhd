//
// Copyright 2011 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "common.hpp"
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <iostream>

static const size_t num_test_iters = 10000000;

int main(int, char *[]){

    srandom(time(NULL)); //seed random()

    if ((fp = ::open("/dev/usrp_e0", O_RDWR)) < 0){
        std::cerr << "Open failed" << std::endl;
        return -1;
    }

    size_t num_pass = 0, num_fail = 0;
    for (size_t i = 0; i < num_test_iters; i++){
	if(i%1000000 == 0) {
	    std::cout << "num pass: " << num_pass;
	    std::cout << "\tnum fail: " << num_fail << std::endl;
	}
        //make random values
        int random_test32 = ::random();
        int random_test16 = ::random() & 0xffff;
        //int random_secs = ::random();

        //set a bunch of registers
        poke16(E100_REG_MISC_TEST, random_test16);
        poke32(E100_REG_SR_MISC_TEST32, random_test32);
        //poke32(E100_REG_TIME64_TICKS, 0);
        //poke32(E100_REG_TIME64_IMM, 1); //immediate
        //poke32(E100_REG_TIME64_SECS, random_secs);

        //read a bunch of registers
        if (
            (peek16(E100_REG_MISC_TEST) == random_test16) and
            (peek32(E100_REG_RB_MISC_TEST32) == random_test32) and
//            (peek32(E100_REG_RB_TIME_NOW_SECS) == random_secs) and
//            (peek32(E100_REG_RB_TIME_NOW_TICKS) < 1000000) and
        true) num_pass++;
        else  num_fail++;
    }

    std::cout << "num pass: " << num_pass << std::endl;
    std::cout << "num fail: " << num_fail << std::endl;

    ::close(fp);
    return 0;
}
