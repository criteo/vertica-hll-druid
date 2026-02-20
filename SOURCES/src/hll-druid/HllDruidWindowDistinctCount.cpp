#include "hll-druid/druid_vertica.hpp"

using namespace Vertica;
using namespace druid;

class HllDruidWindowDistinctCount : public AnalyticFunction
{
public:
    virtual void processPartition(ServerInterface &srvInterface,
                                  AnalyticPartitionReader &inputReader,
                                  AnalyticPartitionWriter &outputWriter)
    {
        try {
            std::vector<size_t> argCols;
            inputReader.getTypeMetaData().getArgumentColumns(argCols);
            if (argCols.size() != 1)
                vt_report_error(0, "HllDruidWindowDistinctCount expects exactly 1 argument");

            const size_t sketchIdx = argCols[0];

            uint8_t buffer[NUM_BYTES_FOR_BUCKETS + NUM_HEADER_BYTES] = {0};

            HllDruid hll = HllDruid::wrapRawBuffer(buffer, sizeof(buffer));
            hll.reset();

            do {
                const VString &sketch = inputReader.getStringRef(sketchIdx);
                if (!sketch.isNull() && sketch.length() > 0) {
                    hll.fold(
                        reinterpret_cast<const uint8_t *>(sketch.data()),
                        sketch.length());
                }
            } while (inputReader.next());

            vint count = static_cast<vint>(hll.approximateCountDistinct());

            do {
                outputWriter.setInt(0, count);
            } while (outputWriter.next());

        } catch (std::exception &e) {
            vt_report_error(0, "Exception while processing partition: [%s]", e.what());
        }
    }
};

class HllDruidWindowDistinctCountFactory : public AnalyticFunctionFactory
{
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

    virtual AnalyticFunction *createAnalyticFunction(ServerInterface &srvInterface)
    {
        return vt_createFuncObject<HllDruidWindowDistinctCount>(srvInterface.allocator);
    }
};

RegisterFactory(HllDruidWindowDistinctCountFactory);
