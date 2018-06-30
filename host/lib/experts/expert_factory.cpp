//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/experts/expert_factory.hpp>

namespace uhd { namespace experts {

expert_container::sptr expert_factory::create_container(const std::string& name)
{
    return expert_container::make(name);
}

}}
