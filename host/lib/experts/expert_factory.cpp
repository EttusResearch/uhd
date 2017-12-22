//
// Copyright 2016 Ettus Research
//
// SPDX-License-Identifier: GPL-3.0
//

#include "expert_factory.hpp"

namespace uhd { namespace experts {

expert_container::sptr expert_factory::create_container(const std::string& name)
{
    return expert_container::make(name);
}

}}
