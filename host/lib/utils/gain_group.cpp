//
// Copyright 2010 Ettus Research LLC
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

#include <uhd/utils/gain_group.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/utils/assert.hpp>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>
#include <algorithm>
#include <vector>
#include <iostream>

using namespace uhd;

static const bool verbose = false;

static bool compare_by_step_size(
    const size_t &rhs, const size_t &lhs, std::vector<gain_fcns_t> &fcns
){
    return fcns.at(rhs).get_range().step > fcns.at(lhs).get_range().step;
}

/***********************************************************************
 * gain group implementation
 **********************************************************************/
class gain_group_impl : public gain_group{
public:
    gain_group_impl(void){
        /*NOP*/
    }

    gain_range_t get_range(void){
        float overall_min = 0, overall_max = 0, overall_step = 0;
        BOOST_FOREACH(const gain_fcns_t &fcns, get_all_fcns()){
            const gain_range_t range = fcns.get_range();
            overall_min += range.min;
            overall_max += range.max;
            //the overall step is the min (zero is invalid, first run)
            if (overall_step == 0) overall_step = range.step;
            overall_step = std::min(overall_step, range.step);
        }
        return gain_range_t(overall_min, overall_max, overall_step);
    }

    float get_value(void){
        float overall_gain = 0;
        BOOST_FOREACH(const gain_fcns_t &fcns, get_all_fcns()){
            overall_gain += fcns.get_value();
        }
        return overall_gain;
    }

    void set_value(float gain){
        std::vector<gain_fcns_t> all_fcns = get_all_fcns();
        if (all_fcns.size() == 0) return; //nothing to set!

        //get the max step size among the gains
        float max_step = 0;
        BOOST_FOREACH(const gain_fcns_t &fcns, all_fcns){
            max_step = std::max(max_step, fcns.get_range().step);
        }

        //create gain bucket to distribute power
        std::vector<float> gain_bucket;

        //distribute power according to priority (round to max step)
        float gain_left_to_distribute = gain;
        BOOST_FOREACH(const gain_fcns_t &fcns, all_fcns){
            const gain_range_t range = fcns.get_range();
            gain_bucket.push_back(
                max_step*int(std::clip(gain_left_to_distribute, range.min, range.max)/max_step)
            );
            gain_left_to_distribute -= gain_bucket.back();
        }

        //get a list of indexes sorted by step size large to small
        std::vector<size_t> indexes_step_size_dec;
        for (size_t i = 0; i < all_fcns.size(); i++){
            indexes_step_size_dec.push_back(i);
        }
        std::sort(
            indexes_step_size_dec.begin(), indexes_step_size_dec.end(),
            boost::bind(&compare_by_step_size, _1, _2, all_fcns)
        );
        UHD_ASSERT_THROW(
            all_fcns.at(indexes_step_size_dec.front()).get_range().step >=
            all_fcns.at(indexes_step_size_dec.back()).get_range().step
        );

        //distribute the remainder (less than max step)
        //fill in the largest step sizes first that are less than the remainder
        BOOST_FOREACH(size_t i, indexes_step_size_dec){
            const gain_range_t range = all_fcns.at(i).get_range();
            float additional_gain = range.step*int(
                std::clip(gain_bucket.at(i) + gain_left_to_distribute, range.min, range.max
            )/range.step) - gain_bucket.at(i);
            gain_bucket.at(i) += additional_gain;
            gain_left_to_distribute -= additional_gain;
        }
        if (verbose) std::cout << "gain_left_to_distribute " << gain_left_to_distribute << std::endl;

        //now write the bucket out to the individual gain values
        for (size_t i = 0; i < gain_bucket.size(); i++){
            if (verbose) std::cout << gain_bucket.at(i) << std::endl;
            all_fcns.at(i).set_value(gain_bucket.at(i));
        }
    }

    void register_fcns(
        const gain_fcns_t &gain_fcns, size_t priority
    ){
        _registry[priority].push_back(gain_fcns);
    }

private:
    //! get the gain function sets in order (highest priority first)
    std::vector<gain_fcns_t> get_all_fcns(void){
        std::vector<gain_fcns_t> all_fcns;
        BOOST_FOREACH(size_t key, std::sorted(_registry.keys())){
            const std::vector<gain_fcns_t> &fcns = _registry[key];
            all_fcns.insert(all_fcns.begin(), fcns.begin(), fcns.end());
        }
        return all_fcns;
    }

    uhd::dict<size_t, std::vector<gain_fcns_t> > _registry;
};

/***********************************************************************
 * gain group factory function
 **********************************************************************/
gain_group::sptr gain_group::make(void){
    return sptr(new gain_group_impl());
}
