/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2020 National Technology & Engineering Solutions of Sandia,
 * LLC (NTESS). Under the terms of Contract DE-NA0003525 with NTESS,
 * the U.S. Government retains certain rights in this software.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 *
 */
#ifndef __KD_TREE__
#define __KD_TREE__
#include <iostream>
#include <vector>
#include <algorithm> //std::nth_element
#include <assert.h>
#include <stdlib.h> /*srand, rand*/
#include <time.h> /* time */
#include <math.h> /* log2 */
#include <iomanip> //std::setprecision
#include <map>

// convenient declaration
using KD_Point_Type = double; //must be a numeric type

#define MAX_DIM (3)

/*
 * Our KD Tree supports arbitrary dimensions (MAX_DIM) but is tested for 3 dimensions.
 */
//Macro for less than operator between two kd nodes
#define KD_NODE_LT(a,b){\
        if(m_kdNodes[a].point[l_cut_dim]<m_kdNodes[b].point[l_cut_dim]) l_lt = true;\
        else l_lt = false;\
    } 

#define KD_TREE_SET_PARENT_CHILD {\
        if(subtree&1){/*right subtree*/\
            /*parent was set previously*/\
            m_kdNodes[parent].r_child = first;\
        }\
        else{ /*left subtree*/\
            parent = first-1;\
            m_kdNodes[parent].l_child = first;\
        }\
        m_kdNodes[first].parent = parent;\
        current_tree_size++;\
        }

/// Exception thrown when unable to add something to the tree
class add_error : public std::runtime_error
{
public:
  add_error () : runtime_error ("Error while inserting into tree")
  {
  }
};

/// Exception thrown when unable to remove something from the tree
class remove_error : public std::runtime_error
{
public:
  remove_error () : runtime_error ("Error while removing from tree")
  {
  }
};

/// Exception thrown when unable to update an entry in the tree
class update_error : public std::runtime_error
{
public:
  update_error () : runtime_error ("Error while updating tree")
  {
  }
};

/*!
 * \brief A structure defining the nodes of a k-d tree.
 */
template<typename KD_ID>
struct kd_Node {
    /*! \brief Convenient typedef for id type. */
    using ID_TYPE = KD_ID;

    /*! \brief coordinates of this node in k-dimensional space. */
    KD_Point_Type point[MAX_DIM];

    /*! \brief unique value identifier for this node. */
    KD_ID Id;

    /*! \brief Level of the k-d node; The root node is at level-0. */
    uint8_t depth = -1;

    /*! \brief index of parent node. */
    int parent = -1;

    /*! \brief index of left child. */
    int l_child = -1;

    /*! \brief index of right child. */
    int r_child = -1;

    /*! \brief Default constructor. */
    kd_Node() {}

    /*! \brief Copy constructor. 
     *  \param rhs The node to copy */
    kd_Node(const kd_Node& rhs){
        for(int i=0; i<MAX_DIM; i++) {
            point[i] = rhs.point[i];
        }
        depth = rhs.depth;
        Id = rhs.Id;
        parent = rhs.parent;
        l_child = rhs.l_child;
        r_child = rhs.r_child;
    }
};


/*!
 * \brief A functor class for comparing k-d tree nodes.
 */
class cmpr_kd_nodes{
    public:
        /*!
         * \brief Default constructor.
         * \param a_dim The dimension of the k-dimensional tree along which
         *        comparisons will be made. Value must be less than MAX_DIM.
         */
        cmpr_kd_nodes(uint8_t a_dim){
            assert(a_dim<MAX_DIM);
            m_dim = a_dim;
        }

        /*!
         * \brief The main functor operator for comparing k-d nodes.
         * \param a The first k-d node.
         * \param b The second k-d node.
         * \return True if the first k-d node is less than the second k-d node
         *         along the initialized dimension; otherwise, false is returned.
         */
        template<typename KD_ID>
        bool operator()(const kd_Node<KD_ID> &a, const kd_Node<KD_ID> &b) {
            auto l_dim = m_dim;
            if(a.point[l_dim]<b.point[l_dim]) return true;
            else return false;
        }

    private:
         /*! \brief The dimension along which comparison is made. */
        uint8_t m_dim;
};


/*!
 * \brief A (templated) class for representing a k-d tree.  The templated value
 *        type is for the identifier of the internal k-d tree nodes.  Valid
 *        values include 32-bit signed integers, 64-bit signed integers, etc.
 */
template<typename KD_ID>
class kdTree {
    /*! Convenient typedef for node type. */
    using Node_T = kd_Node<KD_ID>;

    /*! Convenient typedef for vector of node type. */
    using Vector_T = std::vector<Node_T>;

    private:
        /*! \brief Number of dimensions of the k-d tree. */
        uint8_t m_dim;

        /*! \brief Maximum depth of the k-d tree. */
        uint8_t m_maxDepth;

        /*! \brief Threshold depth of the tree beyond which we need to rebalance. */
        uint8_t m_allowedDepth = 100;

        /*! \brief Size of the k-d tree, i.e., number of nodes in the tree. */
        uint32_t m_treeSize;
    
        /*! \brief Iterable container for holding the nodes of the k-d tree. */
        Vector_T m_kdNodes;

        /*!
         * \brief Mapper from IDs to indexes in the m_kdNodes object,
         *        which is used for quick updates.
         */
        std::map<KD_ID, unsigned int> m_idToIndex;

        /*!
         * \brief An initializer for the k-d tree.
         * \param n_points Number of nodes to initialize with.
         * \param a_dim Number of dimensions to copy into our nodes; remaining dimensions,
         *        if any, are set to zero values.
         * \param points The vector of points to used to initialize.
         * \param ids The vector fo identifiers for the points, which must be same size
         *        as the points vector.
         */
        void initialize(size_t n_points, uint8_t a_dim,
                const std::vector<std::vector<KD_Point_Type>>& points,
                const std::vector<KD_ID>& ids) {
            assert(points.size() == ids.size());
            assert(n_points <= points.size());
            m_kdNodes.resize(n_points);
            for (auto i = 0u; i < n_points; i++) {
                for(uint8_t j=0; j < a_dim; j++) m_kdNodes[i].point[j] = points[i][j];
                for(uint8_t j=a_dim; j < MAX_DIM; j++) m_kdNodes[i].point[j] = 0;
                m_kdNodes[i].Id = ids[i];
            }
        }

        /*!
         * \brief Swapper for two k-d nodes at given index into the m_kdNodes collection.
         * \param i Index of one of the k-d nodes.
         * \param j Index of the other k-d node to swap with.
         */
        void swap(size_t i, size_t j) {
            assert(i < m_kdNodes.size() && j < m_kdNodes.size());
            std::swap(m_kdNodes[i],m_kdNodes[j]);
        }

        /*!
         * \brief This function returns the minimum point at the coordinate a_dim
         *        on the subtree rooted at index root.
         * \param a_root The root of the subtree to search for minimum.
         * \param a_dim The coordinate to search along.
         * \return The minimum point's index in the m_kdNodes collection.
         */
        unsigned int find_min(const unsigned int a_root, const uint8_t a_dim) {
            std::vector<unsigned int> minima, to_explore;
            unsigned int node = a_root;
            uint8_t l_cut_dim;
            int child;
            //We avoid recursion by using a stack to track if we have visited all the nodes
            for(to_explore.push_back(a_root); !to_explore.empty(); ){
                node = to_explore.back();
                to_explore.pop_back();
                l_cut_dim = (m_kdNodes[node].depth)%m_dim;
                if(l_cut_dim == a_dim){//we only explore the left subtree
                    child = m_kdNodes[node].l_child;
                    if(child == -1) minima.push_back(node);
                    else to_explore.push_back(child);
                }
                else{//we explore both the subtrees
                    minima.push_back(node);
                    child = m_kdNodes[node].l_child;
                    if(child != -1) to_explore.push_back(child);
                    child = m_kdNodes[node].r_child;
                    if(child != -1) to_explore.push_back(child);
                }
            }
            unsigned int min = minima[0];
            for(auto m : minima){
                if(m_kdNodes[min].point[a_dim] > m_kdNodes[m].point[a_dim]) min = m;
            }
            return min;
        }

        /*!
         * \brief This method rebuilds a balanced k-d-tree because the depth is too large.
         *        At the end of the method m_kdNodes will store the binary tree in a depth
         *        first traversal order.  The assumption being this will lead to more
         *        efficient cache coherency.  The average run time complexity of the median
         *        call is O(n) and worst case is O(n^2).  There are O(log n) calls.  So the
         *        average run time complexity of the construction is O(n logn) and worst
         *        case is O(n^2 log n).  The code would be simpler with recurssion.  We
         *        avoid it to improve run-time.
         */
        void rebalance() {
            unsigned int n_points=0;
            Vector_T tmp(m_treeSize);
            for(unsigned int i=0; i<m_kdNodes.size(); i++){
                if((m_kdNodes[i].parent != -1) || (m_kdNodes[i].l_child != -1) || (m_kdNodes[i].r_child != -1)) tmp[n_points++] = m_kdNodes[i];
            }
            m_kdNodes.resize(m_treeSize);
            for(unsigned int i=0; i<m_treeSize; i++) { m_kdNodes[i] = tmp[i]; m_kdNodes[i].parent = m_kdNodes[i].l_child = m_kdNodes[i].r_child = -1; }
            tmp.clear();
            m_maxDepth = ceil( log2( n_points ) ) + !(n_points & (n_points-1));
            std::vector<unsigned int> loc_kd_subtree(2*n_points);
            uint8_t l_cut_dim = 0;
            uint8_t depth;
            int32_t subtree; //ranges from 0...2^depth-1
            loc_kd_subtree[0] = n_points;
            unsigned int first=0, last=n_points-1, median=n_points/2, left_size, right_size, current_tree_size, parent;
            //The first part builds a balanced (lowest-depth) binary tree.  We do this by setting the median to be first+(size)/2 so that the size of left-sub-tree is always >= right-sub-tree
            //Depth-0 is special since root has no parent
            std::nth_element(&m_kdNodes[first], &m_kdNodes[median], &m_kdNodes[last+1], cmpr_kd_nodes(l_cut_dim));
            m_kdNodes[median].depth = 0;
            swap(0,median); current_tree_size=1; l_cut_dim=1;
            parent = 0;
            left_size = n_points/2; right_size = (n_points-1)/2; /*left_size+right_size==n_points-1*/
            loc_kd_subtree[0] = 1;//start position of left-subtree in m_kdNodes
            loc_kd_subtree[1] = left_size;//end of left-subtree
            loc_kd_subtree[2] = left_size+1;//start of right sub-tree
            loc_kd_subtree[3] = loc_kd_subtree[2] + right_size -1;//end of right-subtree
            for(depth = 1; depth<m_maxDepth-2; depth++){
                //At depth-d there are 2^d nodes one for each subtree and 2^(d-1) parents
                //The parent and subtree locations (indexes to m_kdNodes) are stored in loc_kd_subtree
                for(subtree = 0; subtree < (1<<depth); subtree++){
                    first = loc_kd_subtree[2*subtree];
                    last = loc_kd_subtree[2*subtree+1];
                    median = (1+last+first)/2;
                    std::nth_element(&m_kdNodes[first], &m_kdNodes[median], &m_kdNodes[last+1], cmpr_kd_nodes(l_cut_dim));
                    m_kdNodes[median].depth = depth;
                    swap(first,median);
                    //We update the parent and child pointers of the relevant nodes
                    KD_TREE_SET_PARENT_CHILD
                }
                //Set the sub-tree sizes for the next level
                for(subtree--; subtree>=0; subtree--){
                    //each subtree becomes two subtrees
                    first = loc_kd_subtree[2*subtree];
                    last = loc_kd_subtree[2*subtree+1];
                    left_size = (1+last-first)/2; right_size = (last-first)/2;
                    loc_kd_subtree[4*subtree] = first+1;
                    loc_kd_subtree[4*subtree+1] = first+left_size;
                    loc_kd_subtree[4*subtree+2] = first+left_size+1;
                    loc_kd_subtree[4*subtree+3] = first+left_size+right_size;
                }
                //Update the cutting dimension for the next level
                l_cut_dim = (l_cut_dim+1)%m_dim;
            }
            //The last two levels is where we lose the complete binary tree property, because we can have string of left children
            bool l_lt;
            unsigned int subtree_size;
            for(subtree = 0; subtree < (1<<depth); subtree++){
                //At this point the subtree sizes should be {1,2,3}.  It can't be 0 since it would be make it the last level of the tree rather than the penultimate level
                first = loc_kd_subtree[2*subtree];
                last = loc_kd_subtree[2*subtree+1];
                subtree_size = last+1-first;
                switch(subtree_size){
                    case 1:
                        //last == median == first
                        KD_TREE_SET_PARENT_CHILD
                            m_kdNodes[first].depth=depth;
                        break;
                    case 2:
                        //last == first+1
                        KD_NODE_LT(first,first+1)
                            if(l_lt){//first+1 is the median and first is its left child
                                swap(first,first+1);
                            }
                        //Set the parent of the median and the parent's child index
                        KD_TREE_SET_PARENT_CHILD
                            m_kdNodes[first].depth=depth;
                        m_kdNodes[first+1].depth=depth+1;
                        //Set the parent and child indices for the "other" node
                        m_kdNodes[first+1].parent = first;
                        m_kdNodes[first].l_child = first+1;
                        //update k-d-tree size
                        current_tree_size++;
                        break;
                    case 3:
                        //(first, median, last)
                        std::nth_element(&m_kdNodes[first], &m_kdNodes[first+1], &m_kdNodes[first+3], cmpr_kd_nodes(l_cut_dim));
                        swap(first,first+1);
                        KD_TREE_SET_PARENT_CHILD
                            m_kdNodes[first].depth=depth;
                        m_kdNodes[first+1].depth=depth+1;
                        m_kdNodes[first+2].depth=depth+1;
                        //Note that after the call to std::nth_element the first is the smallest
                        //This first node is the left child of the median
                        //This first+2 node is the right child of the median
                        m_kdNodes[first+1].parent = first;
                        m_kdNodes[first+2].parent = first;
                        m_kdNodes[first].l_child = first+1;
                        m_kdNodes[first].r_child = first+2;
                        current_tree_size+=2;
                        break;
                }
            }
            //Store the indices of the IDs
            for(unsigned int i=0; i<m_treeSize; i++) m_idToIndex[m_kdNodes[i].Id] = i;
        }

        /*!
         * \brief Adds nodes to the end of the tree. It simply inserts it and doesn't
         *        connect it into the tree.
         * \param _point The point to add at
         * \param _id Id of node being added
         * \note This is a utility method, not public
         */
        void append(const std::vector<KD_Point_Type> &_point, const KD_ID &_id) {
            // Puts a point at the end of the list of nodes.
            auto i = m_kdNodes.size();
            m_kdNodes.resize(i + 1);
            for (auto j = 0u; j < m_dim; j++) {
                m_kdNodes[i].point[j] = _point[j];
            }
            for (auto j = m_dim; j < MAX_DIM; j++) {
                m_kdNodes[i].point[j] = 0;
            }
            m_kdNodes[i].Id  = _id;
            m_idToIndex[_id] = i;
        }

        /*!
         * \brief Performs the 'sorting' and sets parent/children appropriately.
         *        Does not balance the tree.
         * Utility method, not public. 
         * \param _index Vector index of node to connect
         */
        void connect(const size_t _index) {
            assert(_index > 0);
            unsigned int node = 0; // the root
            bool         l_lt;
            auto         depth     = 0u;
            auto         l_cut_dim = 0u;
            for (int child; true; l_cut_dim = (l_cut_dim + 1) % m_dim) {
                KD_NODE_LT(_index, node)
                    if (l_lt)
                        child = m_kdNodes[node].l_child;
                    else
                        child = m_kdNodes[node].r_child;
                if (child == -1)
                    break; // End of the tree
                node = child;
            }
            m_kdNodes[_index].parent = node;
            depth                    = m_kdNodes[node].depth + 1;
            m_kdNodes[_index].depth  = depth;
            if (m_maxDepth == depth)
                m_maxDepth++;
            if (l_lt)
                m_kdNodes[node].l_child = _index;
            else
                m_kdNodes[node].r_child = _index;
        }

        /*!
         * \brief Performs a delete and moves nodes up in the tree to fill.
         * \param _index Vector index of node to remove.
         * \return size_t Index of empty spot left by delete after bubbling.
         */
        size_t remove_and_bubble(size_t _index) {
            while (true) {
                int  l_child = m_kdNodes[_index].l_child;
                int  r_child = m_kdNodes[_index].r_child;
                auto parent  = m_kdNodes[_index].parent;
                if (r_child != -1) { // Case I: find the minimum in the _index's cutting dimension in right subtree
                    auto next_node = find_min(r_child, m_kdNodes[_index].depth % m_dim);
                    for (uint8_t j = 0; j < m_dim; j++)
                        m_kdNodes[_index].point[j] = m_kdNodes[next_node].point[j];
                    m_kdNodes[_index].Id              = m_kdNodes[next_node].Id;
                    m_idToIndex[m_kdNodes[_index].Id] = _index;
                    _index                            = next_node;
                } else if (l_child != -1) // Implies r_child IS -1
                {                         // Case II: switch the left subtree to right subtree
                    // TODO: refactor as 'swap left and right and repeat steps for right'
                    auto next_node = find_min(l_child, m_kdNodes[_index].depth % m_dim);
                    for (uint8_t j = 0; j < m_dim; j++)
                        m_kdNodes[_index].point[j] = m_kdNodes[next_node].point[j];
                    m_kdNodes[_index].Id              = m_kdNodes[next_node].Id;
                    m_idToIndex[m_kdNodes[_index].Id] = _index;
                    m_kdNodes[_index].r_child         = l_child;
                    m_kdNodes[_index].l_child         = -1;
                    _index                            = next_node;
                } else { // Case III:  _index is a leaf
                    if (m_kdNodes[parent].l_child == (long)_index)
                        m_kdNodes[parent].l_child = -1;
                    else
                        m_kdNodes[parent].r_child = -1;
                    m_kdNodes[_index].parent = -1;
                    m_kdNodes[_index].Id     = 0;
                    return _index;
                }
            }
            throw remove_error();
        }

    public:
        /*!
         * \brief Default constructor.
         * \param n_points Number of nodes to initialize the k-d tree with.
         * \param a_dim Number of dimensions of the k-d tree; basically, the k in k-d.
         */
        kdTree(int n_points, uint8_t a_dim){
            srand(time(NULL));
            m_treeSize = n_points;
            m_maxDepth = ceil( log2( n_points ) ) + !(n_points & (n_points-1));
            m_kdNodes.resize(n_points);
            m_dim = a_dim;
        }

        /*!
         * \brief Returns the k-d node with the given identifier.
         * \param _id The identifier for the k-d node to return.
         * \return The k-d node with the given identifier.
         */
        std::vector<KD_Point_Type> get_position(const KD_ID& _id){
            std::vector<KD_Point_Type> result;
            auto n = m_idToIndex.find(_id);
            if(n == m_idToIndex.end()) {
                throw std::runtime_error("Id not found");
            }
            auto& node = m_kdNodes[n->second];
            for(auto i=0u;i<MAX_DIM;++i) {
                result.push_back(node.point[i]);
            }
            return result;
        }

        /*!
         * \brief Adjusts the allowed depth of the tree if the requested threshold allows
         *        for the current tree size.
         * \param a_depth The requested allowed depth to set for the tree. We constrain it 
         * to be below 101.  Recall that all the tree operation run-times are linear in tree 
         * depth.  A balanced tree at depth 101 can hold more than 1E30 entries!  So a depth
         * 100 tree is more like a linked list.  We feel the kd-tree depth should not be 
         * allowed to get nowhere close to this and recommend setting it lower.
         * The cost of this is a rebalance() operation with in the average case would run
         * in O(nlogn) time and in the worst case O(n^2logn) in our current implementation
         * We hope to speed up the worst case in a future release  
         */
        void adjust_allowed_depth(uint8_t a_depth){
            assert(a_depth<101);
            uint8_t l_max_depth = ceil( log2( m_treeSize ) ) + !(m_treeSize & (m_treeSize-1));
            if (a_depth < l_max_depth)
              {
                std::stringstream message;
                message << "The tree size: " << m_treeSize << "is too big to fit in the requested depth: " << ((int) a_depth);
                throw std::runtime_error (message.str ());
              }
            m_allowedDepth = a_depth;
        }


        /*!
         * \brief This method constructs a balanced (lowest depth but not necessarily complete
         *        (leaves are filled left to right at max-depth)  kd-tree by repeatedly calling
         *        the std::nth_element function to obtain the median.  The kd-tree order is
         *        left_child(cutting-dimension) < parent <= right_child. At the end of the method
         *        m_kdNodes will store the binary tree in a depth first traversal order.  The
         *        assumption being this will lead to more efficient cache coherency.  The average
         *        run time complexity of the median call is O(n) and worst case is O(n^2).
         *        There are O(log n) calls.  So the average run time complexity of the
         *        construction is O(n logn) and worst case is O(n^2 log n).  The code would be
         *        simpler with recurssion.  We avoid it to improve run-time.
         * \param n_points Number of nodes to initialize with.
         * \param a_dim Number of dimensions to copy into our nodes; remaining dimensions,
         *        if any, are set to zero values.
         * \param points The vector of points to used to initialize.
         * \param ids The vector fo identifiers for the points, which must be same size
         *        as the points vector.
         */
        void build_kdTree_median_nthelement(int n_points, uint8_t a_dim,
                const std::vector<std::vector<KD_Point_Type>>& points,
                const std::vector<KD_ID>& ids) {
            assert(n_points>0); assert(a_dim>1);
            assert(points.size() == ids.size());
            assert(n_points<=points.size());
            initialize(n_points, a_dim, points, ids);
            if(n_points==1) {
                m_kdNodes[0].depth=1;
                return;
            }
            std::vector<unsigned int> loc_kd_subtree(2*n_points);
            uint8_t l_cut_dim = 0;
            /* The depth of the tree is ceil(log2(n)) + 1{n == 2^k} */
            /* If n==2^k then (n)&(n-1)==0 */
            uint8_t depth;
            int32_t subtree; //ranges from 0...2^depth-1
            loc_kd_subtree[0] = n_points;
            unsigned int first=0,
                         last=n_points-1,
                         median=n_points/2,
                         left_size,
                         right_size,
                         current_tree_size,
                         parent;

            /* The first part builds a balanced (lowest-depth) binary tree.  We do this by
             * setting the median to be first+(size)/2 so that the size of left-sub-tree is
             * always >= right-sub-tree
             */
            //Depth-0 is special since root has no parent
            std::nth_element(&m_kdNodes[first], &m_kdNodes[median], &m_kdNodes[last+1], cmpr_kd_nodes(l_cut_dim));
            m_kdNodes[median].depth = 0;
            swap(0,median); current_tree_size=1; l_cut_dim=1;
            left_size = n_points/2; right_size = (n_points-1)/2; /*left_size+right_size==n_points-1*/
            loc_kd_subtree[0] = 1;//start position of left-subtree in m_kdNodes
            loc_kd_subtree[1] = left_size;//end of left-subtree
            loc_kd_subtree[2] = left_size+1;//start of right sub-tree
            loc_kd_subtree[3] = loc_kd_subtree[2] + right_size -1;//end of right-subtree
            for(depth = 1; depth<m_maxDepth-2; depth++){
                //At depth-d there are 2^d nodes one for each subtree and 2^(d-1) parents
                //The parent and subtree locations (indexes to m_kdNodes) are stored in loc_kd_subtree
                for(subtree = 0; subtree < (1<<depth); subtree++){
                    first = loc_kd_subtree[2*subtree];
                    last = loc_kd_subtree[2*subtree+1];
                    median = (1+last+first)/2;
                    std::nth_element(&m_kdNodes[first], &m_kdNodes[median], &m_kdNodes[last+1], cmpr_kd_nodes(l_cut_dim));
                    m_kdNodes[median].depth = depth;
                    swap(first,median);
                    //We update the parent and child pointers of the relevant nodes
                    KD_TREE_SET_PARENT_CHILD
                }
                //Set the sub-tree sizes for the next level
                for(subtree--; subtree>=0; subtree--){
                    //each subtree becomes two subtrees
                    first = loc_kd_subtree[2*subtree];
                    last = loc_kd_subtree[2*subtree+1];
                    left_size = (1+last-first)/2; right_size = (last-first)/2;
                    loc_kd_subtree[4*subtree] = first+1;
                    loc_kd_subtree[4*subtree+1] = first+left_size;
                    loc_kd_subtree[4*subtree+2] = first+left_size+1;
                    loc_kd_subtree[4*subtree+3] = first+left_size+right_size;
                }
                //Update the cutting dimension for the next level
                l_cut_dim = (l_cut_dim+1)%a_dim;
            }
            //The last two levels is where we lose the complete binary tree property, because we can have string of left children
            bool l_lt;
            unsigned int subtree_size;
            for(subtree = 0; subtree < (1<<depth); subtree++){
                //At this point the subtree sizes should be {1,2,3}.  It can't be 0 since it would be make it the last level of the tree rather than the penultimate level
                first = loc_kd_subtree[2*subtree];
                last = loc_kd_subtree[2*subtree+1];
                subtree_size = last+1-first;
                switch(subtree_size){
                    case 1:
                        //last == median == first
                        KD_TREE_SET_PARENT_CHILD
                            m_kdNodes[first].depth=depth;
                        break;
                    case 2:
                        //last == first+1
                        KD_NODE_LT(first,first+1)
                            if(l_lt){//first+1 is the median and first is its left child
                                swap(first,first+1);
                            }
                        //Set the parent of the median and the parent's child index
                        KD_TREE_SET_PARENT_CHILD
                            m_kdNodes[first].depth=depth;
                        m_kdNodes[first+1].depth=depth+1;
                        //Set the parent and child indices for the "other" node
                        m_kdNodes[first+1].parent = first;
                        m_kdNodes[first].l_child = first+1;
                        //update k-d-tree size
                        current_tree_size++;
                        break;
                    case 3:
                        //(first, median, last)
                        std::nth_element(&m_kdNodes[first], &m_kdNodes[first+1], &m_kdNodes[first+3], cmpr_kd_nodes(l_cut_dim));
                        swap(first,first+1);
                        KD_TREE_SET_PARENT_CHILD
                            m_kdNodes[first].depth=depth;
                        m_kdNodes[first+1].depth=depth+1;
                        m_kdNodes[first+2].depth=depth+1;
                        //Note that after the call to std::nth_element the first is the smallest
                        //This first node is the left child of the median
                        //This first+2 node is the right child of the median
                        m_kdNodes[first+1].parent = first;
                        m_kdNodes[first+2].parent = first;
                        m_kdNodes[first].l_child = first+1;
                        m_kdNodes[first].r_child = first+2;
                        current_tree_size+=2;
                        break;
                }
            }
            //Store the indices of the IDs
            for(unsigned int i=0; i<m_treeSize; i++) m_idToIndex[m_kdNodes[i].Id] = i;
        }

        /*!
         * \brief adds a point to the k-d-tree.
         * \param _point Point to index at.
         * \param _id Id to assign.
         */
        void insert(const std::vector<KD_Point_Type> &_point, const KD_ID &_id) {
            /* TODO: Currently we store the new points at the end of m_kdNodes.  We should
             *       be looking to use the slots that have been left vacant by deletion.
             */
            assert(_point.size()==m_dim);
            if(m_idToIndex.end() != m_idToIndex.find(_id)) throw add_error();
            append(_point, _id);
            if(m_kdNodes.size()==1) m_kdNodes[0].depth=0; //Inserting into an empty kd-tree
            if (m_kdNodes.size() > 1) {
                connect(m_kdNodes.size() - 1);
            }
            m_treeSize++;
            if (m_maxDepth > m_allowedDepth)
                rebalance();
        }

        /*!
         * \brief Adds multiple points to the k-d-tree.
         * \param points list of points to index at.
         * \param ids list of ids to assign.
         */
        void insert(const std::vector<std::vector<KD_Point_Type>> &points, const std::vector<KD_ID> &ids) {
            assert(points.size() > 0);
            assert(points.size() == ids.size());
            unsigned int l_size = m_kdNodes.size();
            m_kdNodes.reserve(l_size + points.size());
            append(points[0], ids[0]);
            if(m_kdNodes.size()==1) m_kdNodes[0].depth=0; //Inserting into an empty kd-tree
            for (auto i = 1u; i < points.size(); ++i) {
                append(points[i], ids[i]);
                connect(l_size + i);
            }
            m_treeSize += points.size();
            if (m_maxDepth > m_allowedDepth)
                rebalance();
        }

        /*!
         * \brief Deletes a node based on id.
         * \param _id Id to find and delete.
         */
        void delete_id(const KD_ID &_id) {
            //TODO: keep track of empty entries to reuse
            auto it = m_idToIndex.find(_id);
            if (it == m_idToIndex.end()) throw remove_error();
            remove_and_bubble(it->second);
            m_idToIndex.erase(it);
            m_treeSize--;
        }

        /*!
         * \brief Deletes multiple nodes based on ids.
         * \param ids List of ids to find and delete.
         */
        void delete_ids(const std::vector<KD_ID> &ids) {
            // TODO: likely we can optimize the bubble up if we have a list of things to delete.
            for (auto &id : ids) {
                delete_id(id);
            }
        }

        /*!
         * \brief Find node by id and update is coordinates.
         * \param _point Coordinates to update to.
         * \param _id Id to find.
         */
        void update_id(const std::vector<KD_Point_Type> &_point, const KD_ID &_id) {
            auto it = m_idToIndex.find(_id);
            if (it == m_idToIndex.end()) throw update_error();
            auto node = remove_and_bubble(it->second);

            for (auto j = 0u; j < m_dim; j++) {
                m_kdNodes[node].point[j] = _point[j];
            }
            m_kdNodes[node].Id = _id;
            m_idToIndex[_id]   = node;
            connect(node);
        }

        /*!
         * \brief Updates multiple nodes by id.
         * \param points list of coordinate sets to update to.
         * \param ids list of ids to update.
         */
        void update_ids(const std::vector<std::vector<KD_Point_Type>> &points, const std::vector<KD_ID> &ids) {
            assert(points.size() == ids.size());
            for (auto i = 0u; i < points.size(); i++) {
                update_id(points[i], ids[i]);
            }
            if (m_maxDepth > m_allowedDepth) {
                rebalance();
            }
        }

        /*!
         * \brief Performs a range query of the k-d tree in the (low, high) open interval
         *        and stores the output in the result object.
         * \param low The lower bound of the range query.
         * \param high The upper bound of the range query.
         * \param result The object that stores the output of the query.
         * \todo Update this method so that its query is on the [low, high] closed interval.
         */
        void range_search(KD_Point_Type *low, KD_Point_Type *high, std::vector<KD_ID> &result) {
            //The range is interpretted as an open box, i.e. points on the boundary won't be in the result
            result.clear();
            unsigned int node = 0;
            std::vector<unsigned int> to_explore;
            uint8_t l_cut_dim, l_dim;
            int child;
            bool is_in_box;
            //We avoid recursion by using a stack to perform "binary" search of the tree
            //The logic is similar to find_min
            for(to_explore.push_back(node); !to_explore.empty(); ){
                node = to_explore.back();
                to_explore.pop_back();
                l_cut_dim = (m_kdNodes[node].depth)%m_dim;
                if(m_kdNodes[node].point[l_cut_dim] <= low[l_cut_dim]) {
                    //no need to explore the left subtree
                    child = m_kdNodes[node].r_child;
                    if(child != -1) to_explore.push_back(child);
                }
                else if(m_kdNodes[node].point[l_cut_dim] >= high[l_cut_dim]) {
                    //no need to explore the right subtree
                    child = m_kdNodes[node].l_child;
                    if(child != -1) to_explore.push_back(child);
                }
                else{
                    // have not eliminated any subtrees
                    child = m_kdNodes[node].r_child;
                    if(child != -1) to_explore.push_back(child);
                    child = m_kdNodes[node].l_child;
                    if(child != -1) to_explore.push_back(child);
                }
                //check if the current node is in the box
                is_in_box = true;
                for(l_dim = 0; l_dim <m_dim; l_dim++) {
                    if( (m_kdNodes[node].point[l_dim] < low[l_dim]) || (m_kdNodes[node].point[l_dim] > high[l_dim]) ) {
                        is_in_box = false;
                        break;
                    }
                }
                if(is_in_box) 
                    result.push_back(m_kdNodes[node].Id);
            }
        }

        /*! \brief Sends the k-d tree's information to the standard output stream for displaying. */
        void print(){
            std::cout << "Dim = " << (int)m_dim << std::endl;
            std::cout << "Tree Size = " << (int)m_treeSize << std::endl;
            std::cout << "Tree Depth = " << (int)m_maxDepth << std::endl;
            std::cout << std::fixed;
            std::cout << std::setprecision(2);
            for(unsigned int i=0; i<m_treeSize; i++) {
                std::cout << "Index = " << i << ": Node ID " << m_kdNodes[i].Id << ": ( " << m_kdNodes[i].point[0] << ", " << m_kdNodes[i].point[1] << ", " << m_kdNodes[i].point[2] << ")" << std::endl;
                std::cout << "Parent index = " << m_kdNodes[i].parent << std::endl;
                std::cout << "Left Child index = " << m_kdNodes[i].l_child << "; Right Child index = " << m_kdNodes[i].r_child << std::endl;
                std::cout << "Node Level = " << (int)m_kdNodes[i].depth << std::endl << std::endl;

            }
        }
};

#endif
