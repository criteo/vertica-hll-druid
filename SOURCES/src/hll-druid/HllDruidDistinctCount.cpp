#include "hll-druid/druid_vertica.hpp"


static const std::string BUFFER = "BUFFER";

class HllDruidDistinctCount : public DruidAggregateFunction
{
public:
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
      resWriter.setInt(hll.approximateCountDistinct());
    }
    catch (std::exception &e)
    {
      vt_report_error(0, "Exception while terminating intermediate aggregates: [%s]", e.what());
    }
  }
};

class HllDruidDistinctCountFactory : public DruidAggregateFunctionFactory<HllDruidDistinctCount>
{
  virtual void getIntermediateTypes(ServerInterface &srvInterface,
                                    const SizedColumnTypes &inputTypes,
                                    SizedColumnTypes &intermediateTypeMetaData)
  {
    intermediateTypeMetaData.addVarbinary(NUM_BYTES_FOR_BUCKETS + NUM_HEADER_BYTES, BUFFER);
  }

  virtual void getPrototype(ServerInterface &srvInterface,
                            ColumnTypes &argTypes,
                            ColumnTypes &returnType)
  {
    argTypes.addVarbinary();
    returnType.addInt();
  }

  virtual void getReturnType(ServerInterface &srvInterface,
                             const SizedColumnTypes &inputTypes,
                             SizedColumnTypes &outputTypes)
  {
    outputTypes.addInt();
  }

public:
  HllDruidCreateSynopsisFactory() {vol = IMMUTABLE};

};

RegisterFactory(HllDruidDistinctCountFactory);
RegisterLibrary("Criteo",                                                               // author
                "",                                                                     // lib_build_tag
                "0.1",                                                                  // lib_version
                "7.2.1",                                                                // lib_sdk_version
                "https://github.com/criteo/vertica-hyperloglog",                        // URL
                "Druid HyperLogLog implementation as User Defined Aggregate Functions", // description
                "",                                                                     // licenses required
                ""                                                                      // signature
);
