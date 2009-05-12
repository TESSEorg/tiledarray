#ifndef ARRAY_H__INCLUDED
#define ARRAY_H__INCLUDED

#include <cassert>
#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

#include <coordinates.h>
#include <permutation.h>
#include <range.h>
#include <shape.h>
#include <tile.h>

namespace TiledArray {

  /// The main player: Array
  /// Serves as base to various implementations (local, replicated, distributed)
  ///
  template <typename T, unsigned int DIM, typename CS = CoordinateSystem<DIM> >
  class Array : private boost::noncopyable // we don't allow copy constructors, et al. May need in the future.
  {
  public:
    typedef Array<T,DIM,CS> Array_;
    typedef Range<DIM, CS> range_type;
    typedef typename range_type::tile_iterator range_iterator;
    typedef Shape<DIM, CS> shape_type;
    typedef typename shape_type::iterator shape_iterator;
    typedef typename range_type::ordinal_index ordinal_index;
    typedef typename range_type::tile_index tile_index;
    typedef typename range_type::element_index element_index;
    typedef T value_type;
    typedef CS coordinate_system;

    /// Tile is implemented in terms of boost::multi_array
    /// it provides reshaping, iterators, etc., and supports direct access to the raw pointer.
    /// array layout must match that given by CoordinateSystem (i.e. both C, or both Fortran)
    typedef Tile<value_type, DIM, element_index, coordinate_system> tile;

	typedef detail::ElementIterator<value_type, element_index, Array_ > iterator;
	typedef detail::ElementIterator<value_type const, element_index, Array_ > const_iterator;
    ELEMENT_ITERATOR_FRIENDSHIP( value_type, element_index, Array_ );
    ELEMENT_ITERATOR_FRIENDSHIP( value_type const, element_index, Array_ );

    iterator begin() const {
      return iterator( boost::shared_ptr<shape_iterator>( shape_->begin() ) );
    }

    iterator end() const {
      return iterator( boost::shared_ptr<shape_iterator>( shape_->end() ) );
    }

    /// array is defined by its shape
    Array(const boost::shared_ptr<shape_type>& shp) : shape_(shp) {}

    virtual ~Array() {}

    /// Access array shape.
    const boost::shared_ptr<shape_type>& shape() const { return shape_; }

    /// Access array range.
    const boost::shared_ptr<range_type>& range() const { return shape_->range(); }

    /// Returns the number of dimensions in the array.
    unsigned int dim() const { return DIM; }

    /// Returns an element index that contains the lower limit of each dimension.
    const element_index& origin() const { return shape_->range()->start_element(); }

    // Array virtual functions

	/// Clone array
    virtual boost::shared_ptr<Array_> clone() const =0;

    /// assign each element to a
    virtual void assign(const value_type& val) =0;

    /// where is tile k
    virtual unsigned int proc(const tile_index& k) const =0;
    virtual bool local(const tile_index& k) const =0;


    /// Low-level interface will only allow permutations and efficient direct contractions
    /// it should be sufficient to use with an optimizing array expression compiler

    /// make new Array by applying permutation P
//    virtual Array transpose(const Permutation<DIM>& P) =0;

    /// Higher-level interface will be be easier to use but necessarily less efficient
    /// since it will allow more complex operations implemented in terms of permutations
    /// and contractions by a runtime

    /// bind a string to this array to make operations look normal
    /// e.g. R("ijcd") += T2("ijab") . V("abcd") or T2new("iajb") = T2("ijab")
    /// runtime then can figure out how to implement operations
    // ArrayExpression operator()(const char*) const;

  protected:

    /// Returns the tile index that contains the element index e_idx.
    tile_index get_tile_index(const element_index& e_idx) const {
      assert(includes(e_idx));
      return * this->shape_->range()->find(e_idx);
    }

    /// Returns true when the tile is included in the shape.
	bool includes(const tile_index& t_idx) const {
      return this->shape_->includes(t_idx);
	}

	/// Returns true when the element is included in the range.
	/// It may or may not be included in the shape.
	bool includes(const element_index& e_idx) const {
      return this->shape_->range()->includes(e_idx);
	}

  private:
    /// Shape pointer to a shape object.
    boost::shared_ptr<shape_type> shape_;

  };

};

#endif // TILEDARRAY_H__INCLUDED
