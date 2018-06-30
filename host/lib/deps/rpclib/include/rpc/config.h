#pragma once

#ifndef CONFIG_H_L7IVDSPZ
#define CONFIG_H_L7IVDSPZ

#include <cstddef>
#include <cstdint>

#include "rpc/compatibility.h"

namespace rpc
{

//! \brief
struct constants RPCLIB_FINAL {
    static RPCLIB_CONSTEXPR std::size_t DEFAULT_BUFFER_SIZE = 1024 << 10;
    static RPCLIB_CONSTEXPR std::uint16_t DEFAULT_PORT = 8080;
};

} /* rpc */

// This define allows the end user to replace the msgpack dependency.
// To do so, one has to delete the msgpack headers that are
// in the rpclib directory. The other messagepack headers don't
// need to be stored in place of the others. Finally, the RPCLIB_MSGPACK
// macro has to be changed to the namespace name that this new
// msgpack uses (usually "msgpack", unless it is changed manually)
#ifndef RPCLIB_MSGPACK
#define RPCLIB_MSGPACK clmdep_msgpack
#endif /* ifndef RPCLIB_MSGPACK */

#define RPCLIB_CXX_STANDARD 11

#endif /* end of include guard: CONFIG_H_L7IVDSPZ */
