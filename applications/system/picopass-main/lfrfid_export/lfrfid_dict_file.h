#pragma once
#include <toolbox/protocols/protocol_dict.h>
#include "lfrfid_protocols.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Save protocol from dictionary to file
 * 
 * @param dict 
 * @param protocol 
 * @param filename 
 * @return true 
 * @return false 
 */
bool lfrfid_dict_file_save(ProtocolDict* dict, ProtocolId protocol, const char* filename);

#ifdef __cplusplus
}
#endif
