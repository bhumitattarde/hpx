//  Copyright (c) 2017 Mikael Simberg
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Simple test verifying basic resource_partitioner functionality.

#include <hpx/hpx_init.hpp>
#include <hpx/include/apply.hpp>
#include <hpx/include/resource_partitioner.hpp>
#include <hpx/include/threads.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/threading_base/scheduler_mode.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

int hpx_main()
{
    std::size_t const num_threads = hpx::resource::get_num_threads("default");

    HPX_TEST_EQ(std::size_t(4), num_threads);

    hpx::threads::thread_pool_base& tp =
        hpx::resource::get_thread_pool("default");

    HPX_TEST_EQ(tp.get_active_os_thread_count(), std::size_t(4));

    // Remove all but one pu
    for (std::size_t thread_num = 0; thread_num < num_threads - 1; ++thread_num)
    {
        hpx::threads::suspend_processing_unit(tp, thread_num).get();
    }

    // Schedule some dummy work
    for (std::size_t i = 0; i < 10000; ++i)
    {
        hpx::apply([]() {});
    }

    // Start shutdown
    return hpx::finalize();
}

void test_scheduler(
    int argc, char* argv[], hpx::resource::scheduling_policy scheduler)
{
    hpx::init_params init_args;

    init_args.cfg = {"hpx.os_threads=4"};
    init_args.rp_callback = [scheduler](auto& rp,
                                hpx::program_options::variables_map const&) {
        rp.create_thread_pool("default", scheduler,
            hpx::threads::policies::scheduler_mode(
                hpx::threads::policies::default_mode |
                hpx::threads::policies::enable_elasticity));
    };

    HPX_TEST_EQ(hpx::init(argc, argv, init_args), 0);
}

int main(int argc, char* argv[])
{
    {
        // These schedulers should succeed
        std::vector<hpx::resource::scheduling_policy> schedulers = {
            hpx::resource::scheduling_policy::local,
            hpx::resource::scheduling_policy::local_priority_fifo,
#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
            hpx::resource::scheduling_policy::local_priority_lifo,
#endif
#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
            hpx::resource::scheduling_policy::abp_priority_fifo,
            hpx::resource::scheduling_policy::abp_priority_lifo,
#endif
            // The shared priority scheduler may choose not to create a thread,
            // even when run_now = true and a thread is expected. This can fire
            // an assert in the scheduling_loop if a background thread is not
            // created.
            //hpx::resource::scheduling_policy::shared_priority,
        };

        for (auto const scheduler : schedulers)
        {
            test_scheduler(argc, argv, scheduler);
        }
    }

    {
        // These schedulers should fail
        std::vector<hpx::resource::scheduling_policy> schedulers = {
            hpx::resource::scheduling_policy::static_,
            hpx::resource::scheduling_policy::static_priority,
        };

        for (auto const scheduler : schedulers)
        {
            bool exception_thrown = false;
            try
            {
                test_scheduler(argc, argv, scheduler);
            }
            catch (hpx::exception const&)
            {
                exception_thrown = true;
            }

            HPX_TEST(exception_thrown);
        }
    }

    return hpx::util::report_errors();
}
