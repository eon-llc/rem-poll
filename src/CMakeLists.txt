project(pollingremme)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)

add_contract( pollingremme pollingremme pollingremme.cpp )
target_include_directories( pollingremme
    PUBLIC
    ${CMAKE_SOURCE_DIR}/../include
    /Users/serg/Git/remprotocol/contracts/contracts/rem.system/include
)
target_ricardian_directory( pollingremme ${CMAKE_SOURCE_DIR}/../ricardian )