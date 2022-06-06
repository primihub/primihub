#include "src/primihub/protocol/cryptflow2/utils/emp-tool.h"
#include <thread>

#include "src/primihub/protocol/cryptflow2/GC/bit.h"
#include "src/primihub/protocol/cryptflow2/GC/comparable.h"
#include "src/primihub/protocol/cryptflow2/GC/integer.h"
#include "src/primihub/protocol/cryptflow2/GC/number.h"
#include "src/primihub/protocol/cryptflow2/GC/swappable.h"

#include "src/primihub/protocol/cryptflow2/GC/aes_opt.h"
#include "src/primihub/protocol/cryptflow2/GC/f2k.h"
#include "src/primihub/protocol/cryptflow2/GC/mitccrh.h"
#include "src/primihub/protocol/cryptflow2/GC/utils.h"

#include "src/primihub/protocol/cryptflow2/GC/halfgate_eva.h"
#include "src/primihub/protocol/cryptflow2/GC/halfgate_gen.h"

#include "src/primihub/protocol/cryptflow2/GC/circuit_execution.h"
#include "src/primihub/protocol/cryptflow2/GC/protocol_execution.h"
