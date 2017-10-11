//
// Created by mmath on 9/25/17.
//

#ifndef PYSCAN_DISKSCAN_HPP_HPP
#define PYSCAN_DISKSCAN_HPP_HPP

#include <vector>

#include "Point.hpp"

namespace pyscan {
    class Disk {
        double value;
        double a;
        double b;
        double radius;
    public:
        Disk(double val, double a, double b, double r) : value(val), a(a), b(b), radius(r) {
        }

        Disk() : value(0), a(0), b(0), radius(0) {}

        bool contains(Point<> const &pt) const {
            return (get<0>(pt) - getA()) * (get<0>(pt) - getA()) +  (get<1>(pt) - getB()) * (get<1>(pt) - getB()) <= radius * radius;
        }

        void setA(double la) {
            a = la;
        }

        void setB(double lb) {
            b = lb;
        }

        double getA() const {
            return a;
        }

        double getB() const {
            return b;
        }

        double getR() const {
            return radius;
        }

        void setR(double rb) {
            radius = rb;
        }

        double fValue() const {
            return value;
        }
    };


    void solveCircle3(Point<> const& pt1, Point<> const& pt2, Point<> const& pt3,
                      double &a, double &b);

    void solveCircle3(Point<> const& pt1, Point<> const& pt2, Point<> const& pt3,
                      double &a, double &b, double &r);

    template <typename F>
    Disk diskScan(std::vector<Point<>>& net, std::vector<Point<>>& sampleM, std::vector<Point<>>& sampleB, F scan);

    Disk diskScanSlowStat(std::vector<Point<>>& net, std::vector<Point<>>& sampleM, std::vector<Point<>>& sampleB, double rho);

    Disk diskScanStatLabels(std::vector<LPoint<2>>& net, std::vector<LPoint<2>>& sampleM, std::vector<LPoint<2>>& sampleB, double rho);

    /*
    Disk diskScanStatLabels(std::vector<LPoint<int, 2>>& net, std::vector<LPoint<int, 2>>& sampleM, std::vector<LPoint<int, 2>>& sampleB, double rho);
    */

    Disk diskScanStat(std::vector<Point<2>>& net, std::vector<Point<2>>& sampleM, std::vector<Point<2>>& sampleB, double rho);



}
#endif //PYSCAN_DISKSCAN_HPP_HPP