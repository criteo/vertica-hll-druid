# Druid Hyperloglog
The repository contains three C++ UDAFS using a variant of HyperLogLog compatible with the Druid implementation, allowing to consume/create HyperLogLog from/to Druid.

# Build
```
mkdir build
cd build
cmake ../SOURCES
make
```

# Install on Vertica
SQL statements to install the library and register the functions can be found in `SOURCES/install.sql`
