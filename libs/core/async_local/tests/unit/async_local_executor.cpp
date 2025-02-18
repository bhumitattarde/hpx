//  Copyright (c) 2007-2017 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/local/execution.hpp>
#include <hpx/local/future.hpp>
#include <hpx/local/init.hpp>
#include <hpx/modules/testing.hpp>

#include <cstdint>

///////////////////////////////////////////////////////////////////////////////
std::int32_t increment(std::int32_t i)
{
    return i + 1;
}

std::int32_t increment_with_future(hpx::shared_future<std::int32_t> fi)
{
    return fi.get() + 1;
}

///////////////////////////////////////////////////////////////////////////////
struct mult2
{
    std::int32_t operator()(std::int32_t i) const
    {
        return i * 2;
    }
};

///////////////////////////////////////////////////////////////////////////////
struct decrement
{
    std::int32_t call(std::int32_t i) const
    {
        return i - 1;
    }
};

///////////////////////////////////////////////////////////////////////////////
void do_nothing(std::int32_t) {}

struct do_nothing_obj
{
    void operator()(std::int32_t) const {}
};

struct do_nothing_member
{
    void call(std::int32_t) const {}
};

///////////////////////////////////////////////////////////////////////////////
template <typename Executor>
void test_async_with_executor(Executor& exec)
{
    {
        hpx::future<std::int32_t> f1 = hpx::async(exec, &increment, 42);
        HPX_TEST_EQ(f1.get(), 43);

        hpx::future<void> f2 = hpx::async(exec, &do_nothing, 42);
        f2.get();
    }

    {
        hpx::promise<std::int32_t> p;
        hpx::shared_future<std::int32_t> f = p.get_future();

        hpx::future<std::int32_t> f1 =
            hpx::async(exec, &increment_with_future, f);
        hpx::future<std::int32_t> f2 =
            hpx::async(exec, &increment_with_future, f);

        p.set_value(42);
        HPX_TEST_EQ(f1.get(), 43);
        HPX_TEST_EQ(f2.get(), 43);
    }

    {
        using hpx::placeholders::_1;

        hpx::future<std::int32_t> f1 =
            hpx::async(exec, hpx::bind(&increment, 42));
        HPX_TEST_EQ(f1.get(), 43);

        hpx::future<std::int32_t> f2 =
            hpx::async(exec, hpx::bind(&increment, _1), 42);
        HPX_TEST_EQ(f2.get(), 43);
    }

    {
        hpx::future<std::int32_t> f1 = hpx::async(exec, increment, 42);
        HPX_TEST_EQ(f1.get(), 43);

        hpx::future<void> f2 = hpx::async(exec, do_nothing, 42);
        f2.get();
    }

    {
        mult2 mult;

        hpx::future<std::int32_t> f1 = hpx::async(exec, mult, 42);
        HPX_TEST_EQ(f1.get(), 84);
    }

    {
        mult2 mult;

        hpx::future<std::int32_t> f1 = hpx::async(exec, hpx::bind(mult, 42));
        HPX_TEST_EQ(f1.get(), 84);

        using hpx::placeholders::_1;

        hpx::future<std::int32_t> f2 =
            hpx::async(exec, hpx::bind(mult, _1), 42);
        HPX_TEST_EQ(f2.get(), 84);

        do_nothing_obj do_nothing_f;
        hpx::future<void> f3 =
            hpx::async(exec, hpx::bind(do_nothing_f, _1), 42);
        f3.get();
    }

    {
        decrement dec;

        hpx::future<std::int32_t> f1 =
            hpx::async(exec, &decrement::call, dec, 42);
        HPX_TEST_EQ(f1.get(), 41);

        do_nothing_member dnm;
        hpx::future<void> f2 =
            hpx::async(exec, &do_nothing_member::call, dnm, 42);
        f2.get();
    }

    {
        decrement dec;

        using hpx::placeholders::_1;

        hpx::future<std::int32_t> f1 =
            hpx::async(exec, hpx::bind(&decrement::call, dec, 42));
        HPX_TEST_EQ(f1.get(), 41);

        hpx::future<std::int32_t> f2 =
            hpx::async(exec, hpx::bind(&decrement::call, dec, _1), 42);
        HPX_TEST_EQ(f2.get(), 41);

        do_nothing_member dnm;
        hpx::future<void> f3 =
            hpx::async(exec, hpx::bind(&do_nothing_member::call, dnm, _1), 42);
        f3.get();
    }
}

int hpx_main()
{
    {
        hpx::execution::sequenced_executor exec;
        test_async_with_executor(exec);
    }

    {
        hpx::execution::parallel_executor exec;
        test_async_with_executor(exec);
    }

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    // Initialize and run HPX
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
