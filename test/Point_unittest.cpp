//
// Created by mmath on 5/22/18.
//

#include "Point.hpp"

#include <limits.h>
#include <random>
#include <iostream>



#include "gtest/gtest.h"

namespace {


    TEST(Point, intersection) {
        pyscan::Point<> p1(1.0, 1.0, 1.0);
        pyscan::Point<> p2(1.0, 0.0, 1.0);
        pyscan::Point<> h1(1.0, 0.0, -1.0);
        EXPECT_TRUE(pyscan::intersection(p1, p2).approx_eq(h1));

        pyscan::Point<> p3(0.0, 1.0, 0.0);
        pyscan::Point<> p4(1.0, 0.0, 1.0);
        pyscan::Point<> h2(1.0, 0.0, -1.0);
        EXPECT_TRUE(pyscan::intersection(p3, p4).approx_eq(h2));

        pyscan::Point<> p5(2.0, 1.0, -1.0);
        pyscan::Point<> p6(1.0, 1.0, 3.0);
        pyscan::Point<> h3(4.0, -7.0, 1.0);
        EXPECT_TRUE(pyscan::intersection(p5, p6).approx_eq(h3));
    }

    TEST(Point, dot) {
        pyscan::Point<> p1(1.0, -1.0, 3.0);
        pyscan::Point<> p2(2.0, 0.0, 1.0);
        EXPECT_FLOAT_EQ(p1.dot(p2), 5.0);
        pyscan::Point<> p3(1.0, -1.0, 3.0);
        pyscan::Point<> p4(0.0, 2.0, 0.0);
        EXPECT_FLOAT_EQ(p3.dot(p4), -2.0);
    }

    TEST(Point, parallel) {
        pyscan::Point<> p1(0.0, 1.0, 1.0);
        pyscan::Point<> p2(0.0, 1.0, 0.0);
        EXPECT_TRUE(pyscan::is_parallel(p1, p2));
        pyscan::Point<> p5(0.0, 1.0, 0.0);
        pyscan::Point<> p6(0.0, 1.0, 0.0);
        EXPECT_TRUE(pyscan::is_parallel(p5, p6));
        pyscan::Point<> p3(0.0, 1.0, 1.0);
        pyscan::Point<> p4(-1.0, 1.0, 1.0);
        EXPECT_TRUE(!pyscan::is_parallel(p3, p4));
    }

    TEST(Point, above) {
        pyscan::Point<> h1(1.0, 1.0, 0.0);
        pyscan::Point<> p1(0.0, -1.0, 2.0);
        pyscan::Point<> p2(0.0, 0.0, 1.0);
        pyscan::Point<> p3(-1.0, 1.0, 1.0);
        pyscan::Point<> p4(-1.0, 2.0, 1.0);
        EXPECT_TRUE(h1.above(p1));
        EXPECT_TRUE(!h1.above(p2));
        EXPECT_TRUE(!h1.above(p3));
        EXPECT_TRUE(!h1.above(p4));
    }

//    TEST(Point, above_closed_interval) {
//        pyscan::Point<> h1(1.0, 1.0, 0.0);
//        pyscan::Point<> h2(1.0, 1.0, -1.0);
//        pyscan::Point<> p1(0.0, -1.0, 1.0);
//        pyscan::Point<> p2(2.0, 1.0, 1.0);
//        EXPECT_TRUE(!pyscan::above_closed_interval(h1, h2, p1, p2));
//        EXPECT_TRUE(pyscan::above_closed_interval(h2, h1, p1, p2));
//
//        pyscan::Point<> h3(1.0, 2.0, 0.0);
//        pyscan::Point<> h4(1.0, 1.0, 0.0);
//        pyscan::Point<> p3(0.0, 0.0, 1.0);
//        pyscan::Point<> p4(1.0, -1.0, 0.0);
//        pyscan::Point<> p5(-2.0, 1.0, 0.0);
//        EXPECT_TRUE(pyscan::above_closed_interval(h3, h4, p3, p4));
//        EXPECT_TRUE(pyscan::above_closed_interval(h4, h3, p4, p5));
//    }

}