//
// Created by mmath on 7/7/17.
//

#include <functional>
#include <iostream>


#include <boost/iterator_adaptors.hpp>
#include <boost/python.hpp>
#include <boost/python/stl_iterator.hpp>
#include  <boost/python/suite/indexing/vector_indexing_suite.hpp>

#include <typeinfo>


#include "ConvexHull.hpp"
#include "Segment.hpp"
#include "RectangleScan.hpp"
#include "HalfSpaceScan.hpp"
#include "DiskScan.hpp"
#include "FunctionApprox.hpp"
#include "TrajectoryScan.hpp"
#include "ConvexHull.hpp"
#include "TrajectoryCoreSet.hpp"
#include "HamTree.hpp"



#define PY_WRAP(FNAME) py::def("FNAME", &pyscan:: FNAME)


namespace py = boost::python;

template <class K, class V>
boost::python::dict toPythonDict(std::unordered_map<K, V> const& map) {
    boost::python::dict dictionary;
    for (auto iter = map.begin(); iter != map.end(); ++iter) {
        dictionary[iter->first] = iter->second;
    }
    return dictionary;
}

template<typename T>
std::vector<T> to_std_vector(const py::object& iterable) {
    return std::vector<T>(py::stl_input_iterator<T>(iterable),
                          py::stl_input_iterator<T>());
}

auto toPointList(py::object const& el) -> pyscan::point_list_t {
    pyscan::point_list_t l;
    for (auto beg = py::stl_input_iterator<pyscan::Point<>>(el);
         beg != py::stl_input_iterator<pyscan::Point<>>(); beg++) {
        l.push_back(*beg);
    }
    return l;
}

template<class T>
py::list std_vector_to_py_list(const std::vector<T>& v) {
    py::list l;
    for (auto it = v.begin(); it != v.end(); it++) {
        l.append(*it);
    }
    return l;
}


namespace pyscan {

    pyscan::Segment to_Segment(Point<2> const& pt) {
        return Segment(HalfSpace<2>(pt));
    }

    auto sized_region(double size) -> std::function<double(double, double, double, double)> {
        /*
         * Useful for finding a region of a certain size.
         */
        return [size] (double m, double m_total, double b, double b_total) {
            (void)b;
            (void)b_total;
            assert(size <= 1 && size >= 0);
            assert(m <= m_total && 0 <= m && m_total > 0);
            return 1 - fabs(m / m_total - size);
        };
    }

    auto linear_f(double a, double b) -> std::function<double(double, double, double, double)> {
        /*
         * Useful for finding a region of a certain size.
         */

        return [a, b] (double mv, double m_total, double bv, double b_total) {
            return a * mv / m_total + b * bv / b_total;
        };
    }

    std::function<double(double, double)> rho_f(std::function<double(double, double, double)> const& f, double rho) {
       return [&](double x, double y) {
           return f(x, y, rho);
       };
    }

    Subgrid maxSubgridLin(Grid const& grid, double eps, discrepancy_func_t const& f) {
      return max_subgrid_convex(grid, eps, f);
    }


    Subgrid maxSubgridSlow(Grid const &grid, discrepancy_func_t const& f) {
        return max_subgrid(grid, f);
    }


    double evaluate(discrepancy_func_t const& f, double m, double m_tot, double b, double b_tot) {
        return f(m, m_tot, b, b_tot);
    }

};

/*
 * Default converter that just assumes that ConvertVal can be static_cast to RetVal
 */
template<typename ConvertVal, typename RetVal>
struct Converter {
    RetVal convert(ConvertVal const& val) {
        return static_cast<RetVal>(val);
    }
};


/*
 * Converter that attempts to convert a PyObject* into a vector of items.
 */
template<typename RetVal>
struct Converter<const py::object&, std::vector<RetVal>> {
    RetVal convert(const py::object& iterable) {
        return std::vector<T>(py::stl_input_iterator<RetVal>(iterable),
                              py::stl_input_iterator<RetVal>())
    }
};

template<typename ConvertVal, typename ElVal>
ConvertVal convert(ElVal obj_ptr) {
    Converter<ConvertVal, ElVal> converter;
    return converter.convert(obj_ptr);
}

template<typename Ret, typename ...Args>
struct auto_converter {
    std::function<Ret(Args...)> func;

    auto_converter(std::function<Ret(Args...)> const& f) : func(f) {}

    template<typename ...Args2>
    Ret operator()(Args2... args) {

        return func(convert<Args>(std::forward<Args2>(args))...);
    }
};

/*
 * This converts two argument c++ tuples into python tuples automatically when they are returned from the c++ code.
 * s -- a two element tuple of type t1 and t2
 * returns -- a two element python tuple of type t1 and t2.
 */
template<typename T1, typename T2>
struct tuple_to_python_tuple {
    static PyObject* convert(const std::tuple<T1, T2> &s) {
        return boost::python::incref(py::make_tuple(std::get<0>(s), std::get<1>(s)).ptr());
    }
};


/*
 * This converts returned vectors automatically into python lists when the are returned from the c++ code.
 * s -- a vector of type T
 * returns -- A python list containing the elements of s.
 */
template <typename T>
struct vector_to_python_list {
    static PyObject* convert(const std::vector<T> &s) {
        auto new_list = std_vector_to_py_list(s);
        return boost::python::incref(new_list.ptr());
    }
};


double evaluate_halfplane(pyscan::halfspace2_t const& d1, pyscan::wpoint_list_t const&  mpts, pyscan::wpoint_list_t const& bpts, pyscan::discrepancy_func_t const& disc) {
    return pyscan::evaluate_range(d1, mpts, bpts, disc);
}

double evaluate_disk(pyscan::Disk const& d1, pyscan::wpoint_list_t const&  mpts, pyscan::wpoint_list_t const& bpts, pyscan::discrepancy_func_t const& disc) {
    return pyscan::evaluate_range(d1, mpts, bpts, disc);
}

double evaluate_rectangle(pyscan::Rectangle const& d1, pyscan::wpoint_list_t const&  mpts, pyscan::wpoint_list_t const& bpts, pyscan::discrepancy_func_t const& disc) {
    return pyscan::evaluate_range(d1, mpts, bpts, disc);
}

double evaluate_disk_labeled(pyscan::Disk const& d1, pyscan::lpoint_list_t const&  mpts, pyscan::lpoint_list_t const& bpts, pyscan::discrepancy_func_t const& disc) {
    return pyscan::evaluate_range(d1, mpts, bpts, disc);
}

double evaluate_rectangle_labeled(pyscan::Rectangle const& d1, pyscan::lpoint_list_t const&  mpts, pyscan::lpoint_list_t const& bpts, pyscan::discrepancy_func_t const& disc) {
    return pyscan::evaluate_range(d1, mpts, bpts, disc);
}

double evaluate_disk_traj(pyscan::Disk const& d1, pyscan::trajectory_set_t const&  mpts, pyscan::trajectory_set_t const& bpts, pyscan::discrepancy_func_t const& disc) {
    return pyscan::evaluate_range(d1, mpts, bpts, disc);
}

double evaluate_rectangle_traj(pyscan::Rectangle const& d1, pyscan::trajectory_set_t const&  mpts, pyscan::trajectory_set_t const& bpts, pyscan::discrepancy_func_t const& disc) {
    return pyscan::evaluate_range(d1, mpts, bpts, disc);
}

double evaluate_halfplane_traj(pyscan::halfspace2_t const& d1, pyscan::trajectory_set_t const&  mpts, pyscan::trajectory_set_t const& bpts, pyscan::discrepancy_func_t const& disc) {
    return pyscan::evaluate_range(d1, mpts, bpts, disc);
}

double evaluate_halfplane_labeled(pyscan::halfspace2_t const& d1, pyscan::lpoint_list_t const&  mpts, pyscan::lpoint_list_t const& bpts, pyscan::discrepancy_func_t const& disc) {
    return pyscan::evaluate_range(d1, mpts, bpts, disc);
}


BOOST_PYTHON_MODULE(libpyscan) {
    using namespace py;

    /*
    py::class_<pyscan::MaximumIntervals>("MaximumIntervals", py::init<std::size_t>())
            .def("mergeZeros")
    */

    to_python_converter<std::tuple<pyscan::Segment, pyscan::Segment>, tuple_to_python_tuple<pyscan::Segment, pyscan::Segment>>();
    to_python_converter<std::tuple<pyscan::Disk, double>, tuple_to_python_tuple<pyscan::Disk, double>>();
    to_python_converter<std::tuple<pyscan::HalfSpace<2>, double>, tuple_to_python_tuple<pyscan::HalfSpace<2>, double>>();
    to_python_converter<std::tuple<pyscan::HalfSpace<3>, double>, tuple_to_python_tuple<pyscan::HalfSpace<3>, double>>();
    to_python_converter<std::tuple<pyscan::Rectangle, double>, tuple_to_python_tuple<pyscan::Rectangle, double>>();
    to_python_converter<std::tuple<pyscan::pt2_t, double>, tuple_to_python_tuple<pyscan::pt2_t, double>>();



    py::class_<std::vector<pyscan::Point<2>> >("VecP2")
            .def(vector_indexing_suite<std::vector<pyscan::Point<2>> >());

    py::class_<std::vector<pyscan::Point<3>>>("VecP3")
            .def(vector_indexing_suite<std::vector<pyscan::Point<3>> >());

    py::class_<std::vector<pyscan::WPoint<2>> >("VecWP2")
            .def(vector_indexing_suite<std::vector<pyscan::WPoint<2>> >());

    py::class_<std::vector<pyscan::WPoint<3>> >("VecWP3")
            .def(vector_indexing_suite<std::vector<pyscan::WPoint<2>> >());

    py::class_<std::vector<pyscan::LPoint<2>> >("VecLP2")
            .def(vector_indexing_suite<std::vector<pyscan::LPoint<2>> >());

    py::class_<std::vector<pyscan::LPoint<3>> >("VecLP3")
            .def(vector_indexing_suite<std::vector<pyscan::LPoint<3>> >());
//
//    py::class_<std::vector<pyscan::trajectory_t> >("VecTraj")
//            .def(vector_indexing_suite<std::vector<pyscan::trajectory_t>>());
//
//    py::class_<std::vector<pyscan::wtrajectory_t > >("VecWTraj")
//            .def(vector_indexing_suite<std::vector<pyscan::wtrajectory_t>>());

    py::def("hull", pyscan::graham_march);

    py::class_<pyscan::Grid>("Grid", py::init<size_t, pyscan::wpoint_list_t const&, pyscan::wpoint_list_t const&>())
            .def("totalRedWeight", &pyscan::Grid::totalRedWeight)
            .def("totalBlueWeight", &pyscan::Grid::totalBlueWeight)
            .def("redWeight", &pyscan::Grid::redWeight)
            .def("blueWeight", &pyscan::Grid::blueWeight)
            .def("redSubWeight", &pyscan::Grid::redSubWeight)
            .def("blueSubWeight", &pyscan::Grid::blueSubWeight)
            .def("xCoord", &pyscan::Grid::xCoord)
            .def("yCoord", &pyscan::Grid::yCoord)
            .def("toRectangle", &pyscan::Grid::toRectangle)
            .def("size", &pyscan::Grid::size);

    py::class_<pyscan::Rectangle>("Rectangle", py::init<double, double, double, double>())
            .def("lowX", &pyscan::Rectangle::lowX)
            .def("upX", &pyscan::Rectangle::upX)
            .def("lowY", &pyscan::Rectangle::lowY)
            .def("upY", &pyscan::Rectangle::upY)
            .def("__str__", &pyscan::Rectangle::toString)
            .def("contains", &pyscan::Rectangle::contains)
            .def("intersects_segment", &pyscan::Rectangle::intersects_segment)
            .def("intersects_trajectory", &pyscan::Rectangle::intersects_trajectory);

    py::class_<pyscan::Subgrid>("Subgrid", py::init<size_t, size_t, size_t, size_t, double>())
            .def("lowCol", &pyscan::Subgrid::lowX)
            .def("upCol", &pyscan::Subgrid::upX)
            .def("lowRow", &pyscan::Subgrid::lowY)
            .def("upRow", &pyscan::Subgrid::upY)
            .def("__str__", &pyscan::Subgrid::toString)
            .def("__repr__", &pyscan::Subgrid::toString)
            .def("fValue", &pyscan::Subgrid::fValue);

    py::class_<pyscan::Disk>("Disk", py::init<double, double, double>())
            .def("get_origin", &pyscan::Disk::getOrigin)
            .def("get_radius", &pyscan::Disk::getRadius)
            .def("contains", &pyscan::Disk::contains)
            .def("intersects_segment", &pyscan::Disk::intersects_segment)
            .def("__str__", &pyscan::Disk::str)
            .def("__repr__", &pyscan::Disk::str)
            .def("intersects_trajectory", &pyscan::Disk::intersects_trajectory);

    py::class_<pyscan::HalfSpace<2>>("Halfplane", py::init<pyscan::Point<2>>())
            .def("get_coords", &pyscan::HalfSpace<2>::get_coords)
            .def("contains", &pyscan::HalfSpace<2>::contains)
            .def("intersects_segment", &pyscan::HalfSpace<2>::intersects_segment)
            .def("__str__", &pyscan::HalfSpace<2>::str)
            .def("__repr__", &pyscan::HalfSpace<2>::str)
            .def("intersects_trajectory", &pyscan::HalfSpace<2>::intersects_trajectory);

    py::class_<pyscan::HalfSpace<3>>("Halfspace", py::init<pyscan::Point<3>>())
            .def("get_coords", &pyscan::HalfSpace<3>::get_coords)
            .def("contains", &pyscan::HalfSpace<3>::contains)
            .def("__str__", &pyscan::HalfSpace<3>::str)
            .def("__repr__", &pyscan::HalfSpace<3>::str)
            .def("intersects_segment", &pyscan::HalfSpace<3>::intersects_segment);

    py::class_<pyscan::pt2_t>("Point", py::init<double, double, double>())
            .def("approx_eq", &pyscan::Point<2>::approx_eq)
            .def("__getitem__", &pyscan::Point<2>::operator())
            .def("get_coord", &pyscan::Point<2>::get_coord)
            .def("above", &pyscan::Point<2>::above)
            .def("above_closed", &pyscan::Point<2>::above_closed)
            .def("below_closed", &pyscan::Point<2>::below_closed)
            .def("below", &pyscan::Point<2>::below)
            .def("crosses", &pyscan::Point<2>::crosses)
            .def("evaluate", &pyscan::Point<2>::evaluate)
            .def("orient_down", &pyscan::Point<2>::orient_down)
            .def("orient_up", &pyscan::Point<2>::orient_up)
            .def("dist", &pyscan::Point<2>::dist)
            .def("pdot", &pyscan::Point<2>::pdot)
            .def("parallel_lt", &pyscan::Point<2>::parallel_lt)
            .def("parallel_lte", &pyscan::Point<2>::parallel_lte)
            .def("__str__", &pyscan::Point<>::str)
            .def("__repr__", &pyscan::Point<>::str)
            .def("__eq__", &pyscan::Point<>::operator==);

    py::class_<pyscan::pt3_t>("Point3", py::init<double, double, double, double>())
            .def("approx_eq", &pyscan::Point<3>::approx_eq)
            .def("__getitem__", &pyscan::Point<3>::operator())
            .def("get_coord", &pyscan::Point<3>::get_coord)
            .def("above", &pyscan::Point<3>::above)
            .def("above_closed", &pyscan::Point<3>::above_closed)
            .def("below_closed", &pyscan::Point<3>::below_closed)
            .def("below", &pyscan::Point<3>::below)
            .def("crosses", &pyscan::Point<3>::crosses)
            .def("evaluate", &pyscan::Point<3>::evaluate)
            .def("orient_down", &pyscan::Point<3>::orient_down)
            .def("orient_up", &pyscan::Point<3>::orient_up)
            .def("dist", &pyscan::Point<3>::dist)
            .def("pdot", &pyscan::Point<3>::pdot)
            .def("parallel_lt", &pyscan::Point<3>::parallel_lt)
            .def("parallel_lte", &pyscan::Point<3>::parallel_lte)
            .def("__str__", &pyscan::Point<3>::str)
            .def("__repr__", &pyscan::Point<3>::str)
            .def("__eq__", &pyscan::Point<3>::operator==);

    py::def("to_segment", &pyscan::to_Segment);

    py::def("aeq", &pyscan::util::aeq);
    py::class_<pyscan::Segment, py::bases<pyscan::pt2_t> >("Segment", py::init<pyscan::HalfSpace<2>, pyscan::Point<>, pyscan::Point<> >())
            .def("lte", &pyscan::Segment::lte)
            .def("lt", &pyscan::Segment::lt)
            .def("gt", &pyscan::Segment::gt)
            .def("gte", &pyscan::Segment::gte)
            .def("crossed", &pyscan::Segment::crossed)
            .def("split", &pyscan::Segment::split)
//            .def("__str__", &pyscan::Segment::str)
//            .def("__repr__", &pyscan::Segment::str)
            .def("get_e1", &pyscan::Segment::get_e1)
            .def("get_e2", &pyscan::Segment::get_e2);

    py::class_<pyscan::WPoint<2>, py::bases<pyscan::pt2_t>>("WPoint", py::init<double, double, double, double>())
            .def("get_weight", &pyscan::WPoint<2>::get_weight);

    py::class_<pyscan::LPoint<2>, py::bases<pyscan::wpt2_t> >("LPoint", py::init<size_t, double, double, double, double>())
            .def("get_label", &pyscan::LPoint<2>::get_label);

    py::class_<pyscan::WPoint<3>, py::bases<pyscan::pt3_t>>("WPoint3", py::init<double, double, double, double, double>())
            .def("get_weight", &pyscan::WPoint<2>::get_weight);

    py::class_<pyscan::LPoint<3>, py::bases<pyscan::wpt3_t> >("LPoint3", py::init<size_t, double, double, double, double, double>())
            .def("get_label", &pyscan::LPoint<3>::get_label);

    py::class_<pyscan::discrepancy_func_t >("CFunction", py::no_init);

    py::scope().attr("KULLDORF") = pyscan::discrepancy_func_t(
        [&](double m, double m_tot, double b, double b_tot) {
            return pyscan::kulldorff(m / m_tot, b / b_tot, .0001);
    });

    py::scope().attr("DISC") = pyscan::discrepancy_func_t (
        [&](double m, double m_tot, double b, double b_tot) {
            return std::abs(m / m_tot - b / b_tot);
    });

    py::scope().attr("RKULLDORF") = pyscan::discrepancy_func_t(
        [&](double m, double m_tot, double b, double b_tot) {
            return pyscan::regularized_kulldorff(m / m_tot, b / b_tot, .0001);
    });

    py::def("evaluate", &pyscan::evaluate);
    py::def("size_region", &pyscan::sized_region);
    py::def("linear_f", &pyscan::linear_f);

    py::def("intersection", &pyscan::intersection);
    py::def("correct_orientation", &pyscan::correct_orientation);


    py::def("max_subgrid", &pyscan::max_subgrid);
    py::def("max_subgrid_convex", &pyscan::max_subgrid_convex);
    py::def("max_subgrid_linear", &pyscan::max_subgrid_linear);
//    py::def("max_rectangle", &pyscan::max_rectangle);

    py::def("make_net_grid", &pyscan::make_net_grid);
    py::def("make_exact_grid", &pyscan::make_exact_grid);

    //Max Halfspace codes
    py::def("max_halfplane", &pyscan::max_halfplane);
    py::def("max_halfplane_labeled", &pyscan::max_halfplane_labeled);
    py::def("max_halfspace", &pyscan::max_halfspace);
    py::def("max_halfspace_labeled", &pyscan::max_halfspace_labeled);
    //py::def("max_halfplane_fast", &pyscan::max_halfplane_fast);
    py::def("ham_tree_sample", &pyscan::ham_tree_sample);

    py::def("max_disk", &pyscan::max_disk);
    py::def("max_disk_labeled", &pyscan::max_disk_labeled);
    //py::def("max_disk_lift_labeled", &pyscan::max_disk_labeled);


    py::def("evaluate_disk", &evaluate_disk);
    py::def("evaluate_disk_alt", &evaluate_disk);
    py::def("evaluate_disk_labeled", &evaluate_disk_labeled);
    py::def("evaluate_disk_trajectory", &evaluate_disk_traj);

    py::def("evaluate_halfplane", &evaluate_halfplane);
    py::def("evaluate_halfplane_labeled", &evaluate_halfplane_labeled);
    py::def("evaluate_halfplane_trajectory", &evaluate_halfplane_traj);

    py::def("evaluate_rectangle", &evaluate_rectangle);
    py::def("evaluate_rectangle_labeled", &evaluate_rectangle_labeled);
    py::def("evaluate_rectangle_trajectory", &evaluate_rectangle_traj);


//    py::def("max_disk_cached", &pyscan::max_disk_cached);
//    py::def("max_disk_cached_labeled", &pyscan::max_disk_cached_labeled);

    py::def("max_disk_scale", &pyscan::max_disk_scale);
    py::def("max_disk_scale_labeled", &pyscan::max_disk_scale_labeled);
    py::def("max_disk_scale_labeled_alt", &pyscan::max_disk_scale_labeled_alt);
    py::def("max_rect_labeled", &pyscan::max_rect_labeled);

    py::def("max_rectangle", &pyscan::max_rectangle);


    py::def("max_rect_labeled_scale", pyscan::max_rect_labeled_scale);

    ////////////////////////////////////////////////////////////////////
    //TrajectoryScan.hpp wrappers///////////////////////////////////////
    ////////////////////////////////////////////////////////////////////

    py::class_<pyscan::trajectory_t>("Trajectory", py::init<pyscan::point_list_t>())
            .def("point_dist", &pyscan::trajectory_t::point_dist)
            .def("get_weight", &pyscan::trajectory_t::get_weight)
            .def("get_length", &pyscan::trajectory_t::get_length)
            .def("point_dist", &pyscan::trajectory_t::point_dist)
            .def("get_pts", &pyscan::trajectory_t::get_pts);

    py::class_<pyscan::wtrajectory_t>("WTrajectory", py::init<double, pyscan::point_list_t>())
            .def("point_dist", &pyscan::wtrajectory_t::point_dist)
            .def("get_weight", &pyscan::wtrajectory_t::get_weight)
            .def("get_length", &pyscan::wtrajectory_t::get_length)
            .def("point_dist", &pyscan::wtrajectory_t::point_dist)
            .def("get_pts", &pyscan::wtrajectory_t::get_pts);

    py::def("max_disk_traj_grid", &pyscan::max_disk_traj_grid);
//

    //This simplifies the trajectory by using the dp algorithm.
    py::def("dp_compress", &pyscan::dp_compress);
    //This grids the trajectory and assigns a single point to each cell.
    py::def("grid_kernel", &pyscan::approx_traj_grid);
    py::def("grid_trajectory", &pyscan::grid_traj);
    //This grids the trajectory and creates an alpha hull in each one.
    py::def("grid_direc_kernel", &pyscan::approx_traj_kernel_grid);
    //This is for 2d eps-kernel useful for halfspaces.
    py::def("halfplane_kernel", pyscan::approx_hull);
    py::def("convex_hull", pyscan::graham_march);
    //This is a 3d eps-kernel for disks.
    py::def("lifting_kernel", &pyscan::lifting_coreset);

    py::def("coreset_error_halfplane", &pyscan::error_halfplane_coreset);
    py::def("coreset_error_disk", &pyscan::error_disk_coreset);

    //This is for partial scanning, but could be used for full scannings.
    py::def("block_sample", &pyscan::block_sample);
    py::def("uniform_sample", &pyscan::uniform_sample);
    py::def("even_sample", &pyscan::even_sample);

    py::def("block_sample_error", &pyscan::block_sample_error);
    py::def("uniform_sample_error", &pyscan::uniform_sample_error);
    py::def("even_sample_error", &pyscan::even_sample_error);

}
