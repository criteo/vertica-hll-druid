select version();

\set u_libfile '\'/tmp/build/libhlldruid.so\'';
\echo Loading the HLL-Druid library: :u_libfile

CREATE OR REPLACE LIBRARY libhlldruid AS :u_libfile;
CREATE OR REPLACE AGGREGATE FUNCTION HllDruidCreateSynopsis AS LANGUAGE 'C++' NAME 'HllDruidCreateSynopsisFactory' LIBRARY libhlldruid;
CREATE OR REPLACE AGGREGATE FUNCTION HllDruidDistinctCount AS LANGUAGE 'C++' NAME 'HllDruidDistinctCountFactory' LIBRARY libhlldruid;
CREATE OR REPLACE AGGREGATE FUNCTION HllDruidCombine AS LANGUAGE 'C++' NAME 'HllDruidCombineFactory' LIBRARY libhlldruid;
GRANT EXECUTE ON AGGREGATE FUNCTION HllDruidCreateSynopsis(BIGINT) TO PUBLIC;
GRANT EXECUTE ON AGGREGATE FUNCTION HllDruidDistinctCount(VARBINARY) TO PUBLIC;
GRANT EXECUTE ON AGGREGATE FUNCTION HllDruidCombine(VARBINARY) TO PUBLIC;
