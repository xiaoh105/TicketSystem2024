#ifndef TICKETSYSTEM_CONFIG_H
#define TICKETSYSTEM_CONFIG_H

#include <cstdint>

using page_id_t = int32_t;
using frame_id_t = int32_t;

static constexpr std::size_t BUSTUB_PAGE_SIZE = 4096;
static constexpr std::size_t LRUK_REPLACER_K = 3;
static constexpr page_id_t INVALID_PAGE_ID = -1;

#endif //TICKETSYSTEM_CONFIG_H
