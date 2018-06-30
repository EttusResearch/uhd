//
// Copyright 2010-2011,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/gain_group.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/exception.hpp>
#include <boost/bind.hpp>
#include <algorithm>
#include <vector>

using namespace uhd;

static bool compare_by_step_size(
    const size_t &rhs, const size_t &lhs, std::vector<gain_fcns_t> &fcns
){
    return fcns.at(rhs).get_range().step() > fcns.at(lhs).get_range().step();
}

/*!
 * Get a multiple of step with the following relation:
 *     result = step*floor(num/step)
 *
 * Due to small doubleing-point inaccuracies:
 *     num = n*step + e, where e is a small inaccuracy.
 * When e is negative, floor would yield (n-1)*step,
 * despite that n*step is really the desired result.
 * This function is designed to mitigate that issue.
 *
 * \param num the number to approximate
 * \param step the step size to round with
 * \param e the small inaccuracy to account for
 * \return a multiple of step approximating num
 */
template <typename T> static T floor_step(T num, T step, T e = T(0.001)){
    if (num < T(0)) {
        return step*int(num/step - e);
    } else {
        return step*int(num/step + e);
    }
}

gain_group::~gain_group(void){
    /* NOP */
}

/***********************************************************************
 * gain group implementation
 **********************************************************************/
class gain_group_impl : public gain_group{
public:
    gain_group_impl(void){
        /*NOP*/
    }

    gain_range_t get_range(const std::string &name){
        if (not name.empty()) return _name_to_fcns.get(name).get_range();

        double overall_min = 0, overall_max = 0, overall_step = 0;
        for(const gain_fcns_t &fcns:  get_all_fcns()){
            const gain_range_t range = fcns.get_range();
            overall_min += range.start();
            overall_max += range.stop();
            //the overall step is the min (zero is invalid, first run)
            if (overall_step == 0){
                overall_step = range.step();
            }else if (range.step()){
                overall_step = std::min(overall_step, range.step());
            }
        }
        return gain_range_t(overall_min, overall_max, overall_step);
    }

    double get_value(const std::string &name){
        if (not name.empty()) return _name_to_fcns.get(name).get_value();

        double overall_gain = 0;
        for(const gain_fcns_t &fcns:  get_all_fcns()){
            overall_gain += fcns.get_value();
        }
        return overall_gain;
    }

    void set_value(double gain, const std::string &name){
        if (not name.empty()) return _name_to_fcns.get(name).set_value(gain);

        std::vector<gain_fcns_t> all_fcns = get_all_fcns();
        if (all_fcns.size() == 0) return; //nothing to set!

        //get the max step size among the gains
        double max_step = 0;
        for(const gain_fcns_t &fcns:  all_fcns){
            max_step = std::max(max_step, fcns.get_range().step());
        }

        //create gain bucket to distribute power
        std::vector<double> gain_bucket;

        //distribute power according to priority (round to max step)
        double gain_left_to_distribute = gain;
        for(const gain_fcns_t &fcns:  all_fcns){
            const gain_range_t range = fcns.get_range();
            gain_bucket.push_back(floor_step(uhd::clip(
                gain_left_to_distribute, range.start(), range.stop()
            ), max_step));
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
            all_fcns.at(indexes_step_size_dec.front()).get_range().step() >=
            all_fcns.at(indexes_step_size_dec.back()).get_range().step()
        );

        //distribute the remainder (less than max step)
        //fill in the largest step sizes first that are less than the remainder
        for(size_t i:  indexes_step_size_dec){
            const gain_range_t range = all_fcns.at(i).get_range();
            double additional_gain = floor_step(uhd::clip(
                gain_bucket.at(i) + gain_left_to_distribute, range.start(), range.stop()
            ), range.step()) - gain_bucket.at(i);
            gain_bucket.at(i) += additional_gain;
            gain_left_to_distribute -= additional_gain;
        }
        UHD_LOGGER_DEBUG("UHD") << "gain_left_to_distribute " << gain_left_to_distribute ;

        //now write the bucket out to the individual gain values
        for (size_t i = 0; i < gain_bucket.size(); i++){
            UHD_LOGGER_DEBUG("UHD") << i << ": " << gain_bucket.at(i) ;
            all_fcns.at(i).set_value(gain_bucket.at(i));
        }
    }

    const std::vector<std::string> get_names(void){
        return _name_to_fcns.keys();
    }

    void register_fcns(
        const std::string &name,
        const gain_fcns_t &gain_fcns,
        size_t priority
    ){
        if (name.empty() or _name_to_fcns.has_key(name)){
            //ensure the name name is unique and non-empty
            return register_fcns(name + "_", gain_fcns, priority);
        }
        _registry[priority].push_back(gain_fcns);
        _name_to_fcns[name] = gain_fcns;
    }

private:
    //! get the gain function sets in order (highest priority first)
    std::vector<gain_fcns_t> get_all_fcns(void){
        std::vector<gain_fcns_t> all_fcns;
        for(size_t key:  uhd::sorted(_registry.keys())){
            const std::vector<gain_fcns_t> &fcns = _registry[key];
            all_fcns.insert(all_fcns.begin(), fcns.begin(), fcns.end());
        }
        return all_fcns;
    }

    uhd::dict<size_t, std::vector<gain_fcns_t> > _registry;
    uhd::dict<std::string, gain_fcns_t> _name_to_fcns;
};

/***********************************************************************
 * gain group factory function
 **********************************************************************/
gain_group::sptr gain_group::make(void){
    return sptr(new gain_group_impl());
}
