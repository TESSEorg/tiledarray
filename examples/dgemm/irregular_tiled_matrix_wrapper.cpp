#include <TiledArray/tensor/tensor.h>
#include "irregular_tiled_matrix_wrapper.h"

void *tilearray_future_get_tile(void *f)
{
    void *res;
    typedef TiledArray::Tensor<double, Eigen::aligned_allocator<double> > static_tile_type;
    typedef madness::Future<static_tile_type> static_future_type;
    static_future_type *F = reinterpret_cast <static_future_type*>(f);
    static_tile_type T = F->get();
    res = T.data();
    return res;
}
