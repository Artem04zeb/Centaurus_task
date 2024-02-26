add_executable(net_cl net_client.cpp)
target_link_libraries(net_cl headers asio::asio)
