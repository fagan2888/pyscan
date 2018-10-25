#include "DiskScan.hpp"
#include "HalfSpaceScan.hpp"
#include "SparseGrid.hpp"

#include <unordered_map>
#include <iostream>

namespace pyscan {

    struct LabeledValue {
        size_t label;
        double value;
    };

    using crescent_t = std::vector<LabeledValue>;

    inline static lpt3_t lift_lpt(const lpt2_t &pt) {
        double x = pt(0), y = pt(1);
        return {pt.get_label(), pt.get_weight(), x, y, x * x + y * y, 1.0};
    }

    inline static wpt3_t lift_wpt(const wpt2_t &pt) {
        double x = pt(0), y = pt(1);
        return {pt.get_weight(), x, y, x * x + y * y, 1.0};
    }

    inline static pt3_t lift_pt(const pt2_t &pt) {
        double x = pt(0), y = pt(1);
        return {x, y, x * x + y * y, 1.0};
    }


    inline static bool colinear(const pt2_t &pt1, const pt2_t &pt2, const pt2_t &pt3) {
        double x1 = pt1(0), x2 = pt2(0), x3 = pt3(0);
        double y1 = pt1(1), y2 = pt2(1), y3 = pt3(1);

        return util::aeq(util::det2(x2 - x1, y2 - y1, x2 - x3, y2 - y3), 0.0);
    }

    inline static bool valid_pt(const pt2_t &p1, const pt2_t &p2, const pt2_t &p) {
        return !(p1.approx_eq(p) || p2.approx_eq(p) || colinear(p1, p2, p));
    }

    inline static void get_net_disks(
            const pt2_t &p1, const pt2_t &p2,
            const point_list_t &net,
            const std::function<double(Disk)> &order_func,
            double min_dist, double max_dist,
            std::vector<Disk> &net_disks,
            std::vector<double> &orderV) {

        net_disks.reserve(net.size());
        orderV.reserve(net.size());
        std::for_each(net.begin(), net.end(), [&](const pt2_t &p) {
            if (valid_pt(p1, p2, p)) {
                Disk tmp(p1, p2, p);
                if (min_dist <= tmp.getRadius() && tmp.getRadius() <= max_dist) {
                    net_disks.emplace_back(tmp);
                }
            }
        });

        std::sort(net_disks.begin(), net_disks.end(), [&](const Disk &a, const Disk &b) {
            return order_func(a) < order_func(b);
        });
        for (auto &disk: net_disks) {
            orderV.emplace_back(order_func(disk));
        }

    }

    inline static std::tuple<Disk, double> max_disk_restricted(
            const pt2_t &p1, const pt2_t &p2,
            const point_list_t &net,
            const wpoint_list_t &red,
            const wpoint_list_t &blue,
            double min_dist, double max_dist,
            double red_tot, double blue_tot,
            const discrepancy_func_t &f) {

        Disk cur_max;
        double max_stat = 0.0;
        if (p1.approx_eq(p2)) {
            return std::make_tuple(cur_max, 0.0);
        }

        double orthoX = p2(1) - p1(1);
        double orthoY = p1(0) - p2(0);
        double cX = (p1(0) + p2(0)) / 2.0;
        double cY = (p1(1) + p2(1)) / 2.0;
        auto get_order = [orthoX, orthoY, cX, cY](const Disk &x) {
            auto origin = x.getOrigin();
            return orthoX * (origin(0) - cX) + orthoY * (origin(1) - cY);
        };

        std::vector<Disk> net_disks;
        std::vector<double> orderV;
        get_net_disks(p1, p2, net, get_order, min_dist, max_dist, net_disks, orderV);

        if (net_disks.size() == 0) {
            return std::make_tuple(cur_max, 0.0);
        }


        Disk start_disk = net_disks[0];
        std::vector<double> red_delta(net_disks.size(), 0.0), blue_delta(net_disks.size(), 0.0);
        auto compute_delta = [&get_order, &p1, &p2, &orderV, &start_disk](
                const wpoint_list_t &list,
                std::vector<double> &delta) {

            double weight = 0.0;
            for (auto &p: list) {
                if (start_disk.contains(p)) weight += p.get_weight();

                if (valid_pt(p1, p2, p)) {
                    auto lb = std::lower_bound(orderV.begin(), orderV.end(), get_order(Disk(p1, p2, p)));
                    if (lb == orderV.end()) continue;
                    if (start_disk.contains(p)) {
                        delta[lb - orderV.begin()] -= p.get_weight();
                    } else delta[lb - orderV.begin()] += p.get_weight();
                }
            }
            delta[0] = 0.0;
            return weight;
        };

        double red_weight = compute_delta(red, red_delta);
        double blue_weight = compute_delta(blue, blue_delta);
        for (size_t i = 0; i < net_disks.size(); ++i) {
            red_weight += red_delta[i];
            blue_weight += blue_delta[i];
            double new_stat = f(red_weight, red_tot, blue_weight, blue_tot);
            if (max_stat <= new_stat) {
                cur_max = net_disks[i];
                max_stat = new_stat;
            }
        }
        return std::make_tuple(cur_max, max_stat);
    }

    inline static double update_weight(
            std::unordered_map<size_t, size_t> &cur_set,
            const crescent_t &adding, const crescent_t removing) {

        double update_diff = 0.0;
        for (auto &x: adding) {
            auto it = cur_set.find(x.label);
            if (it != cur_set.end()) {
                cur_set[x.label]++;
            } else {
                update_diff += x.value;
                cur_set[x.label] = 1;
            }
        }

        for (auto &x: removing) {
            auto it = cur_set.find(x.label);
            assert(it != cur_set.end() && it->second > 0);
            if (it->second == 1) {
                update_diff -= x.value;
                cur_set.erase(it);
            } else {
                cur_set[x.label]--;
            }
        }

        return update_diff;
    }

    inline static std::tuple<Disk, double> max_disk_restricted(
            const pt2_t &p1, const pt2_t &p2,
            const point_list_t &net,
            const lpoint_list_t &red,
            const lpoint_list_t &blue,
            double min_dist, double max_dist,
            double red_tot, double blue_tot,
            const discrepancy_func_t &f) {

        Disk cur_max;
        double max_stat = 0.0;

        if (net.size() < 3 || p1.approx_eq(p2)) {
            return std::make_tuple(cur_max, max_stat);
        }

        double orthoX = p2(1) - p1(1);
        double orthoY = p1(0) - p2(0);
        double cX = (p1(0) + p2(0)) / 2.0;
        double cY = (p1(1) + p2(1)) / 2.0;
        auto get_order = [orthoX, orthoY, cX, cY](const Disk &x) {
            auto origin = x.getOrigin();
            return orthoX * (origin(0) - cX) + orthoY * (origin(1) - cY);
        };

        std::vector<Disk> net_disks;
        std::vector<double> orderV;
        get_net_disks(p1, p2, net, get_order, min_dist, max_dist, net_disks, orderV);

        if (net_disks.size() == 0) {
            return std::make_tuple(cur_max, 0.0);
        }

        Disk start_disk = net_disks[0];
        std::vector<crescent_t> red_deltaR(net_disks.size()), blue_deltaR(net_disks.size());
        std::vector<crescent_t> red_deltaA(net_disks.size()), blue_deltaA(net_disks.size());
        std::unordered_map<size_t, size_t> red_set, blue_set;
        auto compute_delta = [&get_order, &p1, &p2, &orderV, &start_disk](
                const lpoint_list_t &list,
                std::vector<crescent_t> &deltaR,
                std::vector<crescent_t> &deltaA,
                std::unordered_map<size_t, size_t> &labels) {
            double weight = 0.0;

            for (auto &p : list) {
                auto cur_label = p.get_label();
                auto cur_weight = p.get_weight();
                if (start_disk.contains(p)) {
                    auto it = labels.find(cur_label);
                    if (it == labels.end()) {
                        labels[cur_label] = 1;
                        weight += cur_weight;
                    } else {
                        labels[cur_label]++;
                    }
                }

                if (valid_pt(p1, p2, p)) {
                    auto lb = std::lower_bound(orderV.begin(), orderV.end(), get_order(Disk(p1, p2, p)));
                    if (lb == orderV.end()) continue;
                    if (start_disk.contains(p)) {
                        deltaR[lb - orderV.begin()].emplace_back(LabeledValue{cur_label, cur_weight});
                    } else {
                        deltaA[lb - orderV.begin()].emplace_back(LabeledValue{cur_label, cur_weight});
                    }
                }
            }

            deltaR[0].clear();
            deltaA[0].clear();

            return weight;
        };

        double red_weight = compute_delta(red, red_deltaR, red_deltaA, red_set);
        double blue_weight = compute_delta(blue, blue_deltaR, blue_deltaA, blue_set);

        for (size_t i = 0; i < net_disks.size(); ++i) {
            red_weight += update_weight(red_set, red_deltaA[i], red_deltaR[i]);
            blue_weight += update_weight(blue_set, blue_deltaA[i], blue_deltaR[i]);
            double new_stat = f(red_weight, red_tot, blue_weight, blue_tot);
            if (max_stat <= new_stat) {
                cur_max = net_disks[i];
                max_stat = new_stat;
            }
        }
        return std::make_tuple(cur_max, max_stat);
    }

    template<typename Pt>
    inline static std::tuple<Disk, double> max_disk_internal(
            const point_list_t &point_net,
            const std::vector<Pt> &red,
            const std::vector<Pt> &blue,
            const discrepancy_func_t &f) {

        double red_tot = computeTotal(red);
        double blue_tot = computeTotal(blue);
        Disk cur_max;
        double max_stat = 0.0;

        for (auto p1 = point_net.begin(); p1 != point_net.end() - 1; ++p1) {
            for (auto p2 = p1 + 1; p2 != point_net.end(); ++p2) {
                Disk local_max_disk;
                double local_max_stat;
                std::tie(local_max_disk, local_max_stat) =
                        max_disk_restricted(*p1, *p2, point_net, red, blue,
                                            0.0, std::numeric_limits<double>::infinity(),
                                            red_tot, blue_tot, f);

                if (local_max_stat > max_stat) {
                    cur_max = local_max_disk;
                    max_stat = local_max_stat;
                }
            }
        }
        return std::make_tuple(cur_max, max_stat);

    }


    std::tuple<Disk, double> max_disk(
            const point_list_t &point_net,
            const wpoint_list_t &red,
            const wpoint_list_t &blue,
            const discrepancy_func_t &f) {

        return max_disk_internal(point_net, red, blue, f);

    }

    std::tuple<Disk, double> max_disk_labeled(
            const point_list_t &point_net,
            const lpoint_list_t &red,
            const lpoint_list_t &blue,
            const discrepancy_func_t &f) {

        return max_disk_internal(point_net, red, blue, f);

    }

#ifdef _DEBUG

    template <typename Pt>
    inline static std::tuple<Disk, double> max_disk_restricted_simple(
            const point_list_t& point_net,
            const std::vector<Pt>& red,
            const std::vector<Pt>& blue,
            double min_dist, double max_dist,
            const discrepancy_func_t& f) {
        double max_stat = 0.0;
        Disk cur_max;
        for (size_t i = 0; i < point_net.size() - 2; ++i) {
            for (size_t j = i + 1; j < point_net.size() - 1; ++j) {
                for (size_t k = j + 1; k < point_net.size(); ++k) {
                    Disk now(point_net[i], point_net[j], point_net[k]);
                    if (now.getRadius() < min_dist || now.getRadius() > max_dist) continue;
                    double cur_stat = evaluate_range(now, red, blue, f);
                    if (cur_stat > max_stat) {
                        cur_max = now;
                        max_stat = cur_stat;
                    }
                }
            }
        }

        return std::make_tuple(cur_max, max_stat);
    }

#endif

    template<typename T>
    inline static std::tuple<Disk, double> max_disk_scale_internal(
            const point_list_t &point_net,
            const std::vector<T> &red,
            const std::vector<T> &blue,
            uint32_t grid_r,
            const discrepancy_func_t &f) {

        double red_tot = computeTotal(red);
        double blue_tot = computeTotal(blue);

        SparseGrid<pt2_t> grid_net(point_net, grid_r);
        SparseGrid<T> grid_red(red, grid_r), grid_blue(blue, grid_r);

        Disk cur_max;
        double max_stat = 0.0;
        for (auto center_cell = grid_net.begin(); center_cell != grid_net.end();) {
            std::vector<pt2_t> net_chunk;
            std::vector<T> red_chunk;
            std::vector<T> blue_chunk;
            net_chunk.clear();
            red_chunk.clear();
            blue_chunk.clear();
            uint32_t i, j;
            std::tie(i, j) = grid_net.get_cell(center_cell->second);
            uint32_t start_k = i < 4 ? 0 : i - 4;
            uint32_t start_l = j < 4 ? 0 : j - 4;
            uint32_t end_k = i + 4 < grid_r ? i + 4 : grid_r;
            uint32_t end_l = j + 4 < grid_r ? j + 4 : grid_r;
            auto range = grid_net(i, j);

            for (uint32_t k = start_k; k <= end_k; ++k) {
                for (uint32_t l = start_l; l <= end_l; ++l) {
                    auto net_range = grid_net(k, l);
                    for (auto it = net_range.first; it != net_range.second; ++it) {
                        net_chunk.emplace_back(it->second);
                    }

                    auto red_range = grid_red(k, l);
                    for (auto it = red_range.first; it != red_range.second; ++it)
                        red_chunk.emplace_back(it->second);


                    auto blue_range = grid_blue(k, l);
                    for (auto it = blue_range.first; it != blue_range.second; ++it)
                        blue_chunk.emplace_back(it->second);
                }
            }

            if (net_chunk.size() >= 3) {
                for (auto pt1 = range.first; pt1 != range.second; ++pt1) {
                    for (auto &pt2: net_chunk) {
                        if (pt1->second.approx_eq(pt2)) continue;

                        Disk local_max_disk;
                        double local_max_stat;
                        std::tie(local_max_disk, local_max_stat) =
                                max_disk_restricted(pt1->second, pt2, net_chunk, red_chunk, blue_chunk,
                                                    grid_net.get_resolution(), 2 * grid_net.get_resolution(),
                                                    red_tot, blue_tot, f);
                        if (local_max_stat > max_stat) {
                            cur_max = local_max_disk;
                            max_stat = local_max_stat;
                        }

                    }
                }
            }


            auto last = center_cell->first;
            do {
                ++center_cell;
            } while (center_cell != grid_net.end() && center_cell->first == last);
        }

        return std::make_tuple(cur_max, max_stat);
    }

    std::tuple<Disk, double> max_disk_scale (
            const point_list_t &point_net,
            const wpoint_list_t &red,
            const wpoint_list_t &blue,
            uint32_t grid_r,
            const discrepancy_func_t &f) {
        return max_disk_scale_internal(point_net, red, blue, grid_r, f);
    }

    std::tuple<Disk, double> max_disk_scale_labeled(
            const point_list_t &point_net,
            const lpoint_list_t &red,
            const lpoint_list_t &blue,
            uint32_t grid_r,
            const discrepancy_func_t &f) {
        return max_disk_scale_internal(point_net, red, blue, grid_r, f);
    }

    template<typename Pt>
    inline static std::tuple<Disk, double> max_disk_cached_internal(
            const point_list_t &point_net,
            const std::vector<Pt> &red,
            const std::vector<Pt> &blue,
            const discrepancy_func_t &f) {

        Disk cur_max;
        double max_stat = 0.0;
        for (uint32_t resolution = 2; resolution < 31; resolution++) {
            uint32_t grid_r = 1u << resolution;
            Disk local_max_disk;
            double local_max_stat;
            std::tie(local_max_disk, local_max_stat) = max_disk_scale_internal(point_net, red, blue, grid_r, f);

#ifdef _DEBUG
            Disk actual_max_disk;
            double actual_max_stat;
            std::tie(actual_max_disk, actual_max_stat) = MaxDiskRestrictedSimple(point_net, red, blue, 1.0 / grid_r, 2.0 / grid_r, f);
            std::cout << grid_r << ", " <<  local_max_stat << "," << actual_max_stat << std::endl;
#endif

            if (max_stat < local_max_stat) {
                cur_max = local_max_disk;
                max_stat = local_max_stat;
            }
        }

        return std::make_tuple(cur_max, max_stat);

    }

    std::tuple<Disk, double> max_disk_cached(
            const point_list_t &point_net,
            const wpoint_list_t &red,
            const wpoint_list_t &blue,
            const discrepancy_func_t &f) {

        return max_disk_cached_internal(point_net, red, blue, f);

    }

    std::tuple<Disk, double> max_disk_cached_labeled(
            const point_list_t &point_net,
            const lpoint_list_t &red,
            const lpoint_list_t &blue,
            const discrepancy_func_t &f) {

        return max_disk_cached_internal(point_net, red, blue, f);

    }


    std::tuple<Disk, double> max_disk_lift(
            const point_list_t &point_net,
            const wpoint_list_t &red,
            const wpoint_list_t &blue,
            const discrepancy_func_t &f) {

        point3_list_t lifted_net(point_net.size());
        wpoint3_list_t lifted_red(red.size()), lifted_blue(blue.size());
        std::transform(point_net.begin(), point_net.end(), lifted_net.begin(), lift_pt);
        std::transform(red.begin(), red.end(), lifted_red.begin(), lift_wpt);
        std::transform(blue.begin(), blue.end(), lifted_blue.begin(), lift_wpt);
        auto mx_h = max_halfspace(lifted_net, lifted_red, lifted_blue, f);
        auto h = std::get<0>(mx_h);
        double a = h[0], b = h[1], c = h[2], d = h[3];
        return std::make_tuple(Disk(-a / (2 * c), -b / (2 * c),
                                    sqrt((a * a + b * b - 4 * c * d) / (4 * c * c))),
                               std::get<1>(mx_h));

    }

    std::tuple<Disk, double> max_disk_lift_labeled(
            const point_list_t &point_net,
            const lpoint_list_t &red,
            const lpoint_list_t &blue,
            const discrepancy_func_t &f) {

        point3_list_t lifted_net(point_net.size());
        lpoint3_list_t lifted_red(red.size()), lifted_blue(blue.size());
        std::transform(point_net.begin(), point_net.end(), lifted_net.begin(), lift_pt);
        std::transform(red.begin(), red.end(), lifted_red.begin(), lift_lpt);
        std::transform(blue.begin(), blue.end(), lifted_blue.begin(), lift_lpt);
        auto mx_h = max_halfspace_labeled(lifted_net, lifted_red, lifted_blue, f);
        auto h = std::get<0>(mx_h);
        double a = h[0], b = h[1], c = h[2], d = h[3];
        return std::make_tuple(Disk(-a / (2 * c), -b / (2 * c),
                                    sqrt((a * a + b * b - 4 * c * d) / (4 * c * c))),
                               std::get<1>(mx_h));

    }


    std::tuple<Disk, double> max_disk_simple(
            const point_list_t &point_net,
            const wpoint_list_t &red,
            const wpoint_list_t &blue,
            const discrepancy_func_t &f) {
        return max_range3<Disk>(point_net, red, blue, f);
    }

    std::tuple<Disk, double> max_disk_simple_labeled(
            const point_list_t &point_net,
            const lpoint_list_t &red,
            const lpoint_list_t &blue,
            const discrepancy_func_t &f) {
        return max_range3_labeled<Disk>(point_net, red, blue, f);
    }

}
