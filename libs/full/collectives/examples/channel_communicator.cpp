//  Copyright (c) 2021 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/iostream.hpp>
#include <hpx/modules/collectives.hpp>

#include <cstdint>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// The name of the used communicator, should be the same on all localities
constexpr char const* channel_communicator_name =
    "/example/channel_communicator/";

// the number of times around the ring
constexpr int num_execution_times = 10;

// the size of the message
constexpr std::size_t message_size = 8192;

///////////////////////////////////////////////////////////////////////////////
bool is_done(std::uint32_t this_locality, int msg)
{
    if (this_locality == 0)
    {
        if (msg == -1)
        {
            hpx::cout << "Process 0 exiting\n";
            return true;
        }
    }
    else if (msg == 0)
    {
        hpx::cout << "Process " << this_locality << "exiting\n";
        return true;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(int argc, char* argv[])
{
    using namespace hpx::collectives;

    std::uint32_t num_localities = hpx::get_num_localities(hpx::launch::sync);
    std::uint32_t this_locality = hpx::get_locality_id();

    // allocate channel communicator
    auto comm = create_channel_communicator(hpx::launch::sync,
        channel_communicator_name, num_sites_arg(num_localities),
        this_site_arg(this_locality));

    std::uint32_t next_locality = (this_locality + 1) % num_localities;
    std::uint32_t prev_locality =
        (this_locality + num_localities - 1) % num_localities;

    int msg1 = -1;
    std::vector<int> msg2(message_size, -1);
    if (this_locality == 0)
    {
        msg1 = num_execution_times;
        hpx::ranges::fill(msg2, num_execution_times);

        // synchronously send initial values to neighbors
        set(comm, that_site_arg(next_locality), msg1).get();
        set(comm, that_site_arg(prev_locality), msg2).get();
    }

    // asynchronously wait for values to be received from neighbors
    auto got_msg1 = get<int>(comm, that_site_arg(prev_locality));
    auto got_msg2 = get<std::vector<int>>(comm, that_site_arg(next_locality));

    while (true)
    {
        auto done_msg1 = got_msg1.then([&](auto&& f) {
            int msg = f.get();

            hpx::cout << "Locality " << this_locality << " received msg1\n";

            if (this_locality == 0)
            {
                msg1 = msg - 1;
                hpx::cout << "Locality 0 decremented msg1\n";
            }
            else
            {
                msg1 = msg;
            }

            // start next round
            if (!is_done(this_locality, msg1))
            {
                got_msg1 = get<int>(comm, that_site_arg(prev_locality));
            }

            if (msg1 >= 0)
            {
                set(comm, that_site_arg(next_locality), msg1).get();
            }
        });

        auto done_msg2 = got_msg2.then([&](auto&& f) {
            std::vector<int> msg = f.get();

            hpx::cout << "Locality " << this_locality << " received msg2\n";

            if (this_locality == 0)
            {
                hpx::ranges::transform(
                    msg, msg2.begin(), [](int val) { return val - 1; });
                hpx::cout << "Locality 0 decremented msg2\n";
            }
            else
            {
                msg2 = std::move(msg);
            }

            // start next round
            if (!is_done(this_locality, msg2[0]))
            {
                got_msg2 =
                    get<std::vector<int>>(comm, that_site_arg(next_locality));
            }

            if (msg2[0] >= 0)
            {
                set(comm, that_site_arg(prev_locality), msg2).get();
            }
        });

        // wait for round to complete
        done_msg1.get();
        done_msg2.get();

        // stop sending messages if done
        if (is_done(this_locality, msg1) && is_done(this_locality, msg2[0]))
        {
            hpx::cout << "Locality " << this_locality << "exiting\n";
            break;
        }
    }

    return hpx::finalize();
}

//while(1)
//{
//    MPI_Waitany(2, request, &index, &status);
//    printf("%d: %d %d\n", rank, msg1, msg2[0]);
//
//    if (rank == 0 && status.MPI_TAG == tag1)
//    {
//        msg1--;
//        printf("Process 0 decremented msg1\n", msg1);
//    }
//    else if (rank == 0 && status.MPI_TAG == tag2)
//    {
//        for (i = 0; i < 8192; i++)
//            msg2[i]--;
//        printf("Process 0 decremented msg2 %d\n", msg2[0]);
//    }
//
//    if (status.MPI_TAG == tag1)
//    {
//        printf("MESSAGE ONE received:  %d --> %d\n", status.MPI_SOURCE, rank);
//        MPI_Irecv(&msg1, 1, MPI_INT, prev, tag1, MPI_COMM_WORLD, &request[0]);
//        if (msg1 >= 0)
//            MPI_Send(&msg1, 1, MPI_INT, next, tag1, MPI_COMM_WORLD);
//    }
//    else
//    {
//        printf("MESSAGE TWO received:  %d --> %d\n", status.MPI_SOURCE, rank);
//        MPI_Irecv(msg2, 8192, MPI_INT, next, tag2, MPI_COMM_WORLD, &request[1]);
//        if (msg2[0] >= 0)
//            MPI_Send(msg2, 8192, MPI_INT, prev, tag2, MPI_COMM_WORLD);
//    }
//
//    if (rank == 0)
//    {
//        if (msg1 == -1 && msg2[0] == -1)
//        {
//            printf("Process %d exiting\n", rank);
//            break;
//        }
//    }
//    else
//    {
//        if (msg1 == 0 && msg2[0] == 0)
//        {
//            printf("Process %d exiting\n", rank);
//            break;
//        }
//    }
//}

int main(int argc, char* argv[])
{
    hpx::init_params params;
    params.cfg = {"--hpx:run-hpx-main"};
    return hpx::init(argc, argv, params);
}
