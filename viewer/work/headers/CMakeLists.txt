add_library(headers INTERFACE include/helpers/net_common.h include/helpers/net_client.h include/helpers/net_connection.h include/helpers/net_message.h include/helpers/net_server.h include/helpers/net_tsqueue.h)
target_include_directories(headers PUBLIC INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
