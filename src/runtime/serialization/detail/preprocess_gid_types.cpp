//  Copyright (c) 2015-2019 Hartmut Kaiser
//  Copyright (c) 2015-2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/runtime/serialization/detail/preprocess_gid_types.hpp>
#include <hpx/serialization/detail/extra_archive_data.hpp>

namespace hpx { namespace serialization { namespace detail {
    template struct extra_archive_data_id_helper<preprocess_gid_types>;
}}}    // namespace hpx::serialization::detail
