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

# Updated build info

## Install proper GCC/G++ for Vertica

Vertica needs (as of 24.4) G++ version 8. You can't get that decently well on Ubuntu Jammy+.

So, we're going to build using Docker! This section will not help you install old GCC on your recent machine, it's not reliable.


## Get the Vertica SDK from a Vertica instance or a Docker image.

If you have a Vertica deployment, just get the SDK from `/opt/vertica/sdk`

If you don't have one handy, use:

`docker run --rm opentext/vertica-k8s:24.4.0-0 tar -C /opt/vertica -c -v sdk > /tmp/vertica-sdk.tar`

## Install the Vertica SDK somewhere useful

On your dev machine (don't do that on a Vertica server, you'll mess things up)

```bash
# load the SDK to a dev directory in your home
mkdir -p ~/dev/vertica-sdks
cd ~/dev/vertica-sdks
tar xf /tmp/vertica-sdk.tar
mv sdk 24.4.0
# link the expected SDK location to your local SDK copy
sudo mkdir -p /opt/vertica && sudo chown -R $(id -u) /opt/vertica
ln -sf ~/dev/vertica-sdks/24.4.0 /opt/vertica/sdk
```

## Run the build environment

You need Docker and the SDK properly ready in /opt/vertica/sdk. Run `./start-build-env.sh` and then drop to the shell: `docker exec -ti vbuilder-hlldruid bash`

You may then try to build using (inside the container)

```bash
cd /build
cmake /sources
make
```
