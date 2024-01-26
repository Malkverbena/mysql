
NOTE: multi_queries + async
    At any given point in time, a connection may only have a single async operation outstanding. This is because the connection uses the underlying Stream object directly, without any locking or queueing. If you perform several async operations concurrently on a single connection without any serialization, the stream may interleave reads and writes from different operations, leading to undefined behavior.
    Async automatically disables multi queries, so if multiple queries are made in an asynchronous connection, the function will fail

# You don't have to compile third-party libraries each time you compile the module.
# With the "recompile" option you can recompile just Openssl or just Boost, both of them or none of them.
# all: Will compile Openssl and Boost.
# openssl: Will compile only Openssl.
# boost: Will compile only Boost.
# none: It will not compile Boost or Openssl.
