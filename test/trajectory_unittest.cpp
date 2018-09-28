
#include "DiskScan.hpp"
#include "DiskScan2.hpp"
#include "Statistics.hpp"
#include "TrajectoryScan.hpp"
#include "Test_Utilities.hpp"

#include <tuple>
#include <limits>
#include <random>
#include <iostream>

#include "gtest/gtest.h"
//
// Created by mmath on 9/14/18.
//
namespace {
    TEST(TrajectorySimplification, matching) {

        const static int n_size = 50;
        //Trajectory
        auto n_pts = pyscantest::randomPoints(n_size);

        auto grid_mapping = pyscan::grid_traj(n_pts.begin(), n_pts.end(), .01);

        //for (auto )
//
//        EXPECT_FLOAT_EQ(d1.getA(), d2.getA());
//        EXPECT_FLOAT_EQ(d1.getB(), d2.getB());
//        EXPECT_FLOAT_EQ(d1.getR(), d2.getR());

    }
}