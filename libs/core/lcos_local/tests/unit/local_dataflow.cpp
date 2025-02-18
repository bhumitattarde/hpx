//  Copyright (c)      2013 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/local/future.hpp>
#include <hpx/local/init.hpp>
#include <hpx/local/thread.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/pack_traversal/unwrap.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

using hpx::program_options::options_description;
using hpx::program_options::value;
using hpx::program_options::variables_map;

using hpx::bind;
using hpx::dataflow;

using hpx::async;
using hpx::future;
using hpx::shared_future;

using hpx::make_ready_future;

using hpx::local::finalize;
using hpx::local::init;

using hpx::unwrapping;
using hpx::util::report_errors;

///////////////////////////////////////////////////////////////////////////////

std::atomic<std::uint32_t> void_f_count;
std::atomic<std::uint32_t> int_f_count;

void void_f()
{
    ++void_f_count;
}
int int_f()
{
    ++int_f_count;
    return 42;
}

std::atomic<std::uint32_t> void_f1_count;
std::atomic<std::uint32_t> int_f1_count;

void void_f1(int)
{
    ++void_f1_count;
}
int int_f1(int i)
{
    ++int_f1_count;
    return i + 42;
}

std::atomic<std::uint32_t> int_f2_count;
int int_f2(int l, int r)
{
    ++int_f2_count;
    return l + r;
}

std::atomic<std::uint32_t> int_f_vector_count;

int int_f_vector(std::vector<int> const& vf)
{
    int sum = 0;
    for (int f : vf)
    {
        sum += f;
    }
    return sum;
}

void function_pointers()
{
    void_f_count.store(0);
    int_f_count.store(0);
    void_f1_count.store(0);
    int_f1_count.store(0);
    int_f2_count.store(0);

    future<void> f1 = dataflow(unwrapping(&void_f1), async(&int_f));
    future<int> f2 = dataflow(unwrapping(&int_f1),
        dataflow(unwrapping(&int_f1), make_ready_future(42)));
    future<int> f3 = dataflow(unwrapping(&int_f2),
        dataflow(unwrapping(&int_f1), make_ready_future(42)),
        dataflow(unwrapping(&int_f1), make_ready_future(37)));

    int_f_vector_count.store(0);
    std::vector<future<int>> vf;
    for (std::size_t i = 0; i < 10; ++i)
    {
        vf.push_back(dataflow(unwrapping(&int_f1), make_ready_future(42)));
    }
    future<int> f4 = dataflow(unwrapping(&int_f_vector), std::move(vf));

    future<int> f5 = dataflow(unwrapping(&int_f1),
        dataflow(unwrapping(&int_f1), make_ready_future(42)),
        dataflow(unwrapping(&void_f), make_ready_future()));

    f1.wait();
    HPX_TEST_EQ(f2.get(), 126);
    HPX_TEST_EQ(f3.get(), 163);
    HPX_TEST_EQ(f4.get(), 10 * 84);
    HPX_TEST_EQ(f5.get(), 126);
    HPX_TEST_EQ(void_f_count, 1u);
    HPX_TEST_EQ(int_f_count, 1u);
    HPX_TEST_EQ(void_f1_count, 1u);
    HPX_TEST_EQ(int_f1_count, 16u);
    HPX_TEST_EQ(int_f2_count, 1u);
}

///////////////////////////////////////////////////////////////////////////////

std::atomic<std::uint32_t> future_void_f1_count;
std::atomic<std::uint32_t> future_void_f2_count;

void future_void_f1(future<void> f1)
{
    HPX_TEST(f1.is_ready());
    ++future_void_f1_count;
}
void future_void_sf1(shared_future<void> f1)
{
    HPX_TEST(f1.is_ready());
    ++future_void_f1_count;
}
void future_void_f2(future<void> f1, future<void> f2)
{
    HPX_TEST(f1.is_ready());
    HPX_TEST(f2.is_ready());
    ++future_void_f2_count;
}

std::atomic<std::uint32_t> future_int_f1_count;
std::atomic<std::uint32_t> future_int_f2_count;

int future_int_f1(future<void> f1)
{
    HPX_TEST(f1.is_ready());
    ++future_int_f1_count;
    return 1;
}
int future_int_f2(future<int> f1, future<int> f2)
{
    HPX_TEST(f1.is_ready());
    HPX_TEST(f2.is_ready());
    ++future_int_f2_count;
    return f1.get() + f2.get();
}

std::atomic<std::uint32_t> future_int_f_vector_count;

int future_int_f_vector(std::vector<future<int>>& vf)
{
    int sum = 0;
    for (future<int>& f : vf)
    {
        HPX_TEST(f.is_ready());
        sum += f.get();
    }
    return sum;
}

void future_function_pointers()
{
    future_void_f1_count.store(0);
    future_void_f2_count.store(0);

    future<void> f1 = dataflow(&future_void_f1,
        async(&future_void_sf1, shared_future<void>(make_ready_future())));

    f1.wait();

    HPX_TEST_EQ(future_void_f1_count, 2u);
    future_void_f1_count.store(0);

    future<void> f2 = dataflow(&future_void_f2,
        async(&future_void_sf1, shared_future<void>(make_ready_future())),
        async(&future_void_sf1, shared_future<void>(make_ready_future())));

    f2.wait();
    HPX_TEST_EQ(future_void_f1_count, 2u);
    HPX_TEST_EQ(future_void_f2_count, 1u);
    future_void_f1_count.store(0);
    future_void_f2_count.store(0);

    future<int> f3 = dataflow(&future_int_f1, make_ready_future());

    HPX_TEST_EQ(f3.get(), 1);
    HPX_TEST_EQ(future_int_f1_count, 1u);
    future_int_f1_count.store(0);

    future<int> f4 =
        dataflow(&future_int_f2, dataflow(&future_int_f1, make_ready_future()),
            dataflow(&future_int_f1, make_ready_future()));

    HPX_TEST_EQ(f4.get(), 2);
    HPX_TEST_EQ(future_int_f1_count, 2u);
    HPX_TEST_EQ(future_int_f2_count, 1u);
    future_int_f1_count.store(0);
    future_int_f2_count.store(0);

    future_int_f_vector_count.store(0);
    std::vector<future<int>> vf;
    for (std::size_t i = 0; i < 10; ++i)
    {
        vf.push_back(dataflow(&future_int_f1, make_ready_future()));
    }
    future<int> f5 = dataflow(&future_int_f_vector, std::ref(vf));

    HPX_TEST_EQ(f5.get(), 10);
}

///////////////////////////////////////////////////////////////////////////////
std::atomic<std::uint32_t> void_f4_count;
std::atomic<std::uint32_t> int_f4_count;

void void_f4(int)
{
    ++void_f4_count;
}
int int_f4(int i)
{
    ++int_f4_count;
    return i + 42;
}

std::atomic<std::uint32_t> void_f5_count;
std::atomic<std::uint32_t> int_f5_count;

void void_f5(int, hpx::future<int>)
{
    ++void_f5_count;
}
int int_f5(int i, hpx::future<int> j)
{
    ++int_f5_count;
    return i + j.get() + 42;
}

void plain_arguments()
{
    void_f4_count.store(0);
    int_f4_count.store(0);

    {
        future<void> f1 = dataflow(&void_f4, 42);
        future<int> f2 = dataflow(&int_f4, 42);

        f1.wait();
        HPX_TEST_EQ(void_f4_count, 1u);

        HPX_TEST_EQ(f2.get(), 84);
        HPX_TEST_EQ(int_f4_count, 1u);
    }

    {
        future<void> f1 = dataflow(hpx::launch::async, &void_f4, 42);
        future<int> f2 = dataflow(hpx::launch::async, &int_f4, 42);

        f1.wait();
        HPX_TEST_EQ(void_f4_count, 2u);

        HPX_TEST_EQ(f2.get(), 84);
        HPX_TEST_EQ(int_f4_count, 2u);
    }

    void_f5_count.store(0);
    int_f5_count.store(0);

    {
        future<void> f1 = dataflow(&void_f5, 42, async(&int_f));
        future<int> f2 = dataflow(&int_f5, 42, async(&int_f));

        f1.wait();
        HPX_TEST_EQ(void_f5_count, 1u);

        HPX_TEST_EQ(f2.get(), 126);
        HPX_TEST_EQ(int_f5_count, 1u);
    }

    {
        future<void> f1 =
            dataflow(hpx::launch::async, &void_f5, 42, async(&int_f));
        future<int> f2 =
            dataflow(hpx::launch::async, &int_f5, 42, async(&int_f));

        f1.wait();
        HPX_TEST_EQ(void_f5_count, 2u);

        HPX_TEST_EQ(f2.get(), 126);
        HPX_TEST_EQ(int_f5_count, 2u);
    }
}

void plain_deferred_arguments()
{
    void_f4_count.store(0);
    int_f4_count.store(0);

    {
        future<void> f1 = dataflow(hpx::launch::deferred, &void_f4, 42);
        future<int> f2 = dataflow(hpx::launch::deferred, &int_f4, 42);

        f1.wait();
        HPX_TEST_EQ(void_f4_count, 1u);

        HPX_TEST_EQ(f2.get(), 84);
        HPX_TEST_EQ(int_f4_count, 1u);
    }

    void_f5_count.store(0);
    int_f5_count.store(0);

    {
        future<void> f1 =
            dataflow(&void_f5, 42, async(hpx::launch::deferred, &int_f));
        future<int> f2 =
            dataflow(&int_f5, 42, async(hpx::launch::deferred, &int_f));

        f1.wait();
        HPX_TEST_EQ(void_f5_count, 1u);

        HPX_TEST_EQ(f2.get(), 126);
        HPX_TEST_EQ(int_f5_count, 1u);
    }
}

void plain_arguments_lazy()
{
    void_f4_count.store(0);
    int_f4_count.store(0);

    auto policy1 = hpx::launch::select([]() { return hpx::launch::sync; });

    {
        future<void> f1 = dataflow(policy1, &void_f4, 42);
        future<int> f2 = dataflow(policy1, &int_f4, 42);

        f1.wait();
        HPX_TEST_EQ(void_f4_count, 1u);

        HPX_TEST_EQ(f2.get(), 84);
        HPX_TEST_EQ(int_f4_count, 1u);
    }

    auto policy2 = hpx::launch::select([]() { return hpx::launch::async; });

    {
        future<void> f1 = dataflow(policy2, &void_f4, 42);
        future<int> f2 = dataflow(policy2, &int_f4, 42);

        f1.wait();
        HPX_TEST_EQ(void_f4_count, 2u);

        HPX_TEST_EQ(f2.get(), 84);
        HPX_TEST_EQ(int_f4_count, 2u);
    }

    void_f5_count.store(0);
    int_f5_count.store(0);

    std::atomic<int> count(0);
    auto policy3 = hpx::launch::select([&count]() -> hpx::launch {
        if (count++ == 0)
            return hpx::launch::async;
        return hpx::launch::sync;
    });

    {
        future<void> f1 = dataflow(policy3, &void_f5, 42, async(&int_f));
        future<int> f2 = dataflow(policy3, &int_f5, 42, async(&int_f));

        f1.wait();
        HPX_TEST_EQ(void_f5_count, 1u);

        HPX_TEST_EQ(f2.get(), 126);
        HPX_TEST_EQ(int_f5_count, 1u);
    }
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(variables_map&)
{
    function_pointers();
    future_function_pointers();
    plain_arguments();
    plain_deferred_arguments();
    plain_arguments_lazy();

    return hpx::local::finalize();
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    // Configure application-specific options
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    // We force this test to use several threads by default.
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    // Initialize and run HPX
    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");
    return report_errors();
}
