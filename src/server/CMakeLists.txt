
aux_source_directory(. SRC_LIST)
aux_source_directory(./db DB_LIST)
aux_source_directory(./model MD_LIST)
add_executable(ChatServer ${SRC_LIST} ${DB_LIST} ${MD_LIST})

target_link_libraries(ChatServer muduo_net muduo_base mysqlclient pthread)


