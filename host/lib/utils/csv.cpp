//
// Copyright 2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/csv.hpp>

using namespace uhd;

csv::rows_type csv::to_rows(std::istream &input){
    csv::rows_type rows;
    std::string line;
    //for each line in the input stream
    while (std::getline(input, line)){
        csv::row_type row(1, "");
        bool in_quote = false;
        char last_ch, next_ch = ' ';
        //for each character in the line
        for(char ch:  line){
            last_ch = next_ch;
            next_ch = ch;
            //catch a quote character and change the state
            //we handle double quotes by checking last_ch
            if (ch == '"'){
                in_quote = not in_quote;
                if (last_ch != '"') continue;
            }
            //a comma not inside quotes is a column delimiter
            if (not in_quote and ch == ','){
                row.push_back("");
                continue;
            }
            //if we got here we record the character
            row.back() += ch;
        }
        rows.push_back(row);
    }
    return rows;
}
