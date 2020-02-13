#ifndef _DRUID_VERTICA_H_
#define _DRUID_VERTICA_H_

#include <type_traits>

#include "Vertica.h"
#include "hll_druid.hpp"

using namespace druid;
using namespace Vertica;


class DruidAggregateFunction : public AggregateFunction {
protected:
    inline HllDruid hllWrap(IntermediateAggs &aggs) {
        return HllDruid::wrapRawBuffer(
            reinterpret_cast<uint8_t *>(aggs.getStringRef(0).data()),
            aggs.getTypeMetaData().getColumnType(0).getStringLength());
    }

    virtual void doAggregate(HllDruid &hll, BlockReader &argReader) = 0;

public:
    void initAggregate(ServerInterface &srvInterface, IntermediateAggs &aggs) override {
        try {
            aggs.getStringRef(0).alloc(NUM_BYTES_FOR_BUCKETS + NUM_HEADER_BYTES);
            HllDruid hll = hllWrap(aggs);
            hll.reset();
        } catch (std::exception &e) {
            vt_report_error(0, "Exception while initializing intermediate aggregates: [%s]", e.what());
        }
    }

    void aggregate(ServerInterface &srvInterface,
                BlockReader &argReader,
                IntermediateAggs &aggs) {
        try {
            HllDruid hll = hllWrap(aggs);
            do {
                doAggregate(hll, argReader);
            } while (argReader.next());
        } catch (std::exception &e) {
            vt_report_error(0, "Exception while aggregating intermediates: [%s]", e.what());
        }
    }

    void combine(ServerInterface &srvInterface,
                IntermediateAggs &aggs,
                MultipleIntermediateAggs &aggsOther) override {
        try {
            HllDruid hll = hllWrap(aggs);
            do {
                hll.fold(
                    reinterpret_cast<const uint8_t *>(aggsOther.getStringRef(0).data()),
                    aggsOther.getStringRef(0).length());
            } while (aggsOther.next());
        } catch (std::exception &e) {
            vt_report_error(0, "Exception while combining intermediates: [%s]", e.what());
        }
    }

    InlineAggregate()
};

template <typename F>
class DruidAggregateFunctionFactory : public AggregateFunctionFactory {
    static_assert(std::is_base_of<AggregateFunction, F>::value,
        "F must be a subclass of AggregateFunction");

  AggregateFunction *createAggregateFunction(ServerInterface &srvInterface) override {
    return vt_createFuncObject<F>(srvInterface.allocator);
  }

  void getParameterType(ServerInterface &srvInterface, SizedColumnTypes &parameterTypes) override {
  }
};

#endif
