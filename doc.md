
NOTE: multi_queries + async
    At any given point in time, a connection may only have a single async operation outstanding. This is because the connection uses the underlying Stream object directly, without any locking or queueing. If you perform several async operations concurrently on a single connection without any serialization, the stream may interleave reads and writes from different operations, leading to undefined behavior.
    Async automatically disables multi queries, so if multiple queries are made in an asynchronous connection, the function will fail.