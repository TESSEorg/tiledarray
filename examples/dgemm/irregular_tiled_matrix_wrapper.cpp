#include <TiledArray/tensor/tensor.h>
#include "irregular_tiled_matrix_wrapper.h"

class PaRSECRescheduleCallback : public madness::CallbackInterface {
public:
    parsec_execution_unit_t    *eu;
    parsec_execution_context_t *task;
    
    PaRSECRescheduleCallback(void *_eu, void *_task) :
        CallbackInterface(),
        eu(static_cast<parsec_execution_unit_t*>(_eu)),
        task(static_cast<parsec_execution_context_t*>(_task))
    {
        PARSEC_LIST_ITEM_SINGLETON(this->task);
    }

    void notify() {
        __parsec_schedule(this->eu, this->task, 0);
    }
};

void *tilearray_future_get_tile(void *future, void *eu, void *task)
{
    void *res;
    typedef TiledArray::Tensor<double, Eigen::aligned_allocator<double> > static_tile_type;
    typedef madness::Future<static_tile_type> static_future_type;
    static_future_type *Future = reinterpret_cast <static_future_type*>(future);

    if( Future->probe() ) {
        static_tile_type T = Future->get();
        res = T.data();
        return res;
    } else {
        Future->register_callback(new PaRSECRescheduleCallback(eu, task));
        return NULL;
    }
}
