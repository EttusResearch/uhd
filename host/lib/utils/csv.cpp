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

#include <uhd/utils/csv.hpp>
#include <boost/foreach.hpp>

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
        BOOST_FOREACH(char ch, line){
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
