// Number of bytes per word
typedef struct {
  string name;
  int bytes_per_word;
} cvita_data_type_t;
localparam cvita_data_type_t U16  = '{name:"U16",  bytes_per_word:2};  // uint16
localparam cvita_data_type_t U32  = '{name:"U32",  bytes_per_word:4};  // uint32
localparam cvita_data_type_t U64  = '{name:"U64",  bytes_per_word:8};  // uint64
localparam cvita_data_type_t U128 = '{name:"U128", bytes_per_word:16}; // uint128
localparam cvita_data_type_t S8   = '{name:"S8",   bytes_per_word:1};  // int8
localparam cvita_data_type_t S16  = '{name:"S16",  bytes_per_word:2};  // int16
localparam cvita_data_type_t S32  = '{name:"S32",  bytes_per_word:4};  // int32
localparam cvita_data_type_t S64  = '{name:"S64",  bytes_per_word:8};  // int64
localparam cvita_data_type_t S128 = '{name:"S128", bytes_per_word:16}; // int128
localparam cvita_data_type_t SC8  = '{name:"SC8",  bytes_per_word:2};  // complex int8
localparam cvita_data_type_t SC12 = '{name:"SC12", bytes_per_word:3};  // complex int12
localparam cvita_data_type_t SC16 = '{name:"SC16", bytes_per_word:4};  // complex int16
localparam cvita_data_type_t SC32 = '{name:"SC32", bytes_per_word:8};  // complex int32
localparam cvita_data_type_t SC64 = '{name:"SC64", bytes_per_word:16}; // complex int64
localparam cvita_data_type_t F32  = '{name:"F32",  bytes_per_word:4};  // single precision float
localparam cvita_data_type_t F64  = '{name:"F64",  bytes_per_word:8};  // double precision float
localparam cvita_data_type_t FC32 = '{name:"FC32", bytes_per_word:8};  // single precision complex float
localparam cvita_data_type_t FC64 = '{name:"FC64", bytes_per_word:16}; // double precision complex float