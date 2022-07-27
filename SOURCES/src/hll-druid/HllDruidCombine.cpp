#include "hll-druid/druid_vertica.hpp"


class HllDruidCombine : public DruidAggregateFunction
{
public:
  virtual void setup(ServerInterface& srvInterface, const SizedColumnTypes& argTypes) {

  }

  virtual void doAggregate(HllDruid &hll, BlockReader &argReader) {
    if (!argReader.getStringRef(0).isNull()) {
      hll.fold(
          reinterpret_cast<const uint8_t *>(argReader.getStringRef(0).data()),
          argReader.getStringRef(0).length());
    }
  }

  virtual void terminate(ServerInterface &srvInterface,
                         BlockWriter &resWriter,
                         IntermediateAggs &aggs)
  {
    try
    {
      HllDruid hll = hllWrap(aggs);
      resWriter.getStringRef().alloc(hll.getSerializedBufferSize());
      size_t length = 0;
      hll.serialize(
        reinterpret_cast<uint8_t *>(resWriter.getStringRef().data()),
        length);
      resWriter.next();
    }
    catch (std::exception &e)
    {
      vt_report_error(0, "Exception while terminating intermediate aggregates: [%s]", e.what());
    }

  }
};


class HllDruidCombineFactory : public DruidAggregateFunctionFactory<HllDruidCombine>
{

  virtual void getIntermediateTypes(ServerInterface &srvInterface,
                                    const SizedColumnTypes &inputTypes,
                                    SizedColumnTypes &intermediateTypeMetaData)
  {
    intermediateTypeMetaData.addVarbinary(NUM_BYTES_FOR_BUCKETS + NUM_HEADER_BYTES);
  }


  virtual void getPrototype(ServerInterface &srvInterface,
                            ColumnTypes &argTypes,
                            ColumnTypes &returnType)
  {
    argTypes.addVarbinary();
    returnType.addVarbinary();
  }

  virtual void getReturnType(ServerInterface &srvInterface,
                             const SizedColumnTypes &inputTypes,
                             SizedColumnTypes &outputTypes)
  {
    outputTypes.addVarbinary(NUM_BYTES_FOR_BUCKETS + NUM_HEADER_BYTES);
  }

public:
  HllDruidCombineFactory() {vol = IMMUTABLE};


};

RegisterFactory(HllDruidCombineFactory);
