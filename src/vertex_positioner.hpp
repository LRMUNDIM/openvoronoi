/*  
 *  Copyright 2010-2011 Anders Wallin (anders.e.e.wallin "at" gmail.com)
 *
 *  This file is part of OpenVoronoi.
 *
 *  OpenVoronoi is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenVoronoi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with OpenVoronoi.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef VERTEX_POSITIONER_HPP
#define VERTEX_POSITIONER_HPP

#include <qd/qd_real.h> // http://crd.lbl.gov/~dhbailey/mpdist/

#include "graph.hpp"
#include "vertex.hpp"
#include "solvers/solution.hpp"

namespace ovd {

class Solver;

// predicate for filtering solutions based on t-value being in [tmin,tmax] range
struct t_filter {
    t_filter(double tmin, double tmax): tmin_(tmin),tmax_(tmax) {}
    bool operator()(Solution s) { 
        double eps=1e-9;
        double tround=s.t;
        if ( fabs(s.t-tmin_) < eps )
            tround=tmin_;
        else if (fabs(s.t-tmax_)<eps)
            tround=tmax_;
        return (tround<tmin_) || (tround>tmax_); // these points rejected!
    }
private:
    double tmin_;
    double tmax_;
};

// predicate for rejecting out-of-region solutions
struct in_region_filter {
    in_region_filter(Site* s): site_(s) {}
    bool operator()(Solution s) { 
        return !site_->in_region(s.p); 
    }
private:
    Site* site_;
};

/// Calculates the (x,y) position of vertices in a voronoi diagram
class VertexPositioner {
public:
    VertexPositioner(HEGraph& gi);
    virtual ~VertexPositioner();
    /// calculate the position of a new voronoi-vertex lying on the given edge.
    /// The new vertex is equidistant to the two sites that defined the edge
    /// and to the new site. 
    Solution position( HEEdge e, Site* s);

    std::vector<double> get_stat() {return errstat;}
    double dist_error(HEEdge e, const Solution& sl, Site* s3);
    void solver_debug(bool b);
private:
    Solution position(Site* s1, double k1, Site* s2, double k2, Site* s3);
    int solver_dispatch(Site* s1, double k1, 
               Site* s2, double k2, 
               Site* s3, double k3, std::vector<Solution>& slns ); 
    bool detect_sep_case(Site* lsite, Site* psite);

// solution-filtering
    double edge_error(Solution& sl);
    Point projection_point(Solution& sl);
// geometry-checks
    
    bool solution_on_edge(Solution& s);
    bool check_far_circle(Solution& s);
    bool check_dist(HEEdge e, const Solution& s, Site* s3);
    bool equal(double d1, double d2);
    
    Solution desperate_solution(Site* s3);

// solvers, to which we dispatch, depending on the input sites
    Solver* ppp_solver;
    Solver* lll_solver;
    Solver* lll_para_solver;
    Solver* qll_solver;
    Solver* sep_solver;
    Solver* alt_sep_solver;
// DATA
    HEGraph& g; // reference to the VD graph.
    double t_min;
    double t_max;
    HEEdge edge;
    std::vector<double> errstat;
};

// error function to minimize when solving by searching for a point on the solution-edge
class VertexError {
public:
    VertexError(HEGraph& gi, HEEdge sln_edge, Site* si3) :
    g(gi),  edge(sln_edge), s3(si3)
    {}
    
    double operator()(const double t) {
        Point p;
        if ( g[edge].type == LINELINE ) { // this is a workaround because the LINELINE edge-parameters are wrong? at least in some cases?
            //HEFace face = g[edge].face;     
            HEEdge twin = g[edge].twin;
            //HEFace twin_face = g[twin].face;
            //Site* s1 =  g[face].site;
            //Site* s2 = g[twin_face].site;
            HEVertex src = g.source(edge);
            HEVertex trg = g.target(edge);
            Point src_p = g[src].position;
            Point trg_p = g[trg].position;
            double src_t = g[src].dist();
            double trg_t = g[trg].dist();
            // edge is src_p -> trg_p
            if ( trg_t > src_t ) {
                double frac = (t-src_t) / (trg_t-src_t);
                p = src_p + frac*(trg_p-src_p);
            } else {
                double frac = (t-trg_t) / (src_t-trg_t);
                p = trg_p + frac*(src_p-trg_p);
            }
            
        } else
            p = g[edge].point(t);
            
        double s3_dist = (p - s3->apex_point(p)).norm();
        return fabs(t-s3_dist);
    }
private:
    HEGraph& g;
    HEEdge edge;
    Site* s3;
};

}
#endif
